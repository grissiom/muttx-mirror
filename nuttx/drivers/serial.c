/************************************************************************************
 * drivers/serial.c
 *
 *   Copyright (C) 2007, 2008 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <spudmonkey@racsa.co.cr>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ************************************************************************************/

/************************************************************************************
 * Included Files
 ************************************************************************************/

#include <nuttx/config.h>

#include <sys/types.h>
#include <unistd.h>
#include <semaphore.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <debug.h>

#include <nuttx/irq.h>
#include <nuttx/arch.h>
#include <nuttx/fs.h>
#include <nuttx/serial.h>

/************************************************************************************
 * Definitions
 ************************************************************************************/

/* The architecture must provide up_putc for this driver */

#ifndef CONFIG_ARCH_LOWPUTC
#  error "Architecture must provide up_putc() for this driver"
#endif

#define uart_putc(ch) up_putc(ch)

/************************************************************************************
 * Private Types
 ************************************************************************************/

/************************************************************************************
 * Private Function Prototypes
 ************************************************************************************/

static int     uart_open(FAR struct file *filep);
static int     uart_close(FAR struct file *filep);
static ssize_t uart_read(FAR struct file *filep, FAR char *buffer, size_t buflen);
static ssize_t uart_write(FAR struct file *filep, FAR const char *buffer, size_t buflen);
static int     uart_ioctl(FAR struct file *filep, int cmd, unsigned long arg);

/************************************************************************************
 * Private Variables
 ************************************************************************************/

struct file_operations g_serialops =
{
  uart_open,  /* open */
  uart_close, /* close */
  uart_read,  /* read */
  uart_write, /* write */
  0,          /* seek */
  uart_ioctl  /* ioctl */
};

/************************************************************************************
 * Private Functions
 ************************************************************************************/

/************************************************************************************
 * Name: uart_takesem
 ************************************************************************************/

static void uart_takesem(FAR sem_t *sem)
{
  while (sem_wait(sem) != 0)
    {
      /* The only case that an error should occur here is if
       * the wait was awakened by a signal.
       */

      ASSERT(*get_errno_ptr() == EINTR);
    }
}

/************************************************************************************
 * Name: uart_givesem
 ************************************************************************************/

#define uart_givesem(sem) (void)sem_post(sem)

/************************************************************************************
 * Name: uart_putxmitchar
 ************************************************************************************/

static void uart_putxmitchar(FAR uart_dev_t *dev, int ch)
{
  int nexthead = dev->xmit.head + 1;
  if (nexthead >= dev->xmit.size)
    {
      nexthead = 0;
    }

  for(;;)
    {
      if (nexthead != dev->xmit.tail)
        {
          dev->xmit.buffer[dev->xmit.head] = ch;
          dev->xmit.head = nexthead;
          return;
        }
      else
        {
          /* Inform the interrupt level logic that we are waiting */

          dev->xmitwaiting = TRUE;

          /* Wait for some characters to be sent from the buffer
           * with the TX interrupt enabled.  When the TX interrupt
           * is enabled, uart_xmitchars should execute and remove
           * some of the data from the TX buffer.
           */

          uart_enabletxint(dev);
          uart_takesem(&dev->xmitsem);
          uart_disabletxint(dev);
        }
    }
}

/************************************************************************************
 * Name: uart_irqwrite
 ************************************************************************************/

static ssize_t uart_irqwrite(FAR uart_dev_t *dev, FAR const char *buffer, size_t buflen)
{
  ssize_t ret = buflen;

  /* Force each character through the low level interface */

  for (; buflen; buflen--)
    {
      int ch = *buffer++;
      uart_putc(ch);

     /* If this is the console, then we should replace LF with LF-CR */

      if (ch == '\n')
        {
          uart_putc('\r');
        }
    }

  return ret;
}

/************************************************************************************
 * Name: uart_write
 ************************************************************************************/

static ssize_t uart_write(FAR struct file *filep, FAR const char *buffer, size_t buflen)
{
  FAR struct inode *inode = filep->f_inode;
  FAR uart_dev_t   *dev   = inode->i_private;
  ssize_t           ret   = buflen;

  /* We may receive console writes through this path from
   * interrupt handlers and from debug output in the IDLE task!
   * In these cases, we will need to do things a little
   * differently.
   */

  if (up_interrupt_context() || getpid() == 0)
    {
      if (dev->isconsole)
        {
          irqstate_t flags = irqsave();
          ret = uart_irqwrite(dev, buffer, buflen);
          irqrestore(flags);
          return ret;
        }
      else
        {
          return ERROR;
        }
    }

  /* Only one user can be accessing dev->xmit.head at once */

  uart_takesem(&dev->xmit.sem);

  /* Loop while we still have data to copy to the transmit buffer.
   * we add data to the head of the buffer; uart_xmitchars takes the
   * data from the end of the buffer.
   */

  uart_disabletxint(dev);
  for (; buflen; buflen--)
    {
      int ch = *buffer++;

      /* Put the character into the transmit buffer */

      uart_putxmitchar(dev, ch);

     /* If this is the console, then we should replace LF with LF-CR */

      if (dev->isconsole && ch == '\n')
        {
          uart_putxmitchar(dev, '\r');
        }
    }

  if (dev->xmit.head != dev->xmit.tail)
    {
      uart_enabletxint(dev);
    }

  uart_givesem(&dev->xmit.sem);
  return ret;
}

/************************************************************************************
 * Name: uart_read
 ************************************************************************************/

static ssize_t uart_read(FAR struct file *filep, FAR char *buffer, size_t buflen)
{
  FAR struct inode *inode = filep->f_inode;
  FAR uart_dev_t   *dev   = inode->i_private;
  ssize_t           recvd = 0;

  /* Only one user can be accessing dev->recv.tail at once */

  uart_takesem(&dev->recv.sem);

  /* Loop while we still have data to copy to the receive buffer.
   * we add data to the head of the buffer; uart_xmitchars takes the
   * data from the end of the buffer.
   */

  uart_disablerxint(dev);
  while (recvd < buflen)
    {
      /* Check if there is more data to return in the circular buffer */

      if (dev->recv.head != dev->recv.tail)
        {
          *buffer++ = dev->recv.buffer[dev->recv.tail];
          recvd++;

          if (++(dev->recv.tail) >= dev->recv.size)
            {
              dev->recv.tail = 0;
            }
        }

      /* No... then we would have to wait to get receive more data.
       * If the user has specified the O_NONBLOCK option, then do not
       * return what we have.
       */

      else if (filep->f_oflags & O_NONBLOCK)
        {
          /* If nothing was transferred, then return the -EAGAIN
           * error (not zero which means end of file).
           */

          if (recvd < 1)
            {
              recvd = -EAGAIN;
            }

          /* Break out of the loop and return the number of bytes
           * received up to the wait condition.
           */

          break;
        }

      /* Otherwise we are going to wait */

      else
        {
          /* Wait for some characters to be sent from the buffer
           * with the TX interrupt re-enabled.
           */

          dev->recvwaiting = TRUE;
          uart_enablerxint(dev);
          uart_takesem(&dev->recvsem);
          uart_disablerxint(dev);
        }
    }

  uart_enablerxint(dev);
  uart_givesem(&dev->recv.sem);
  return recvd;
}

/************************************************************************************
 * Name: uart_ioctl
 ************************************************************************************/

static int uart_ioctl(FAR struct file *filep, int cmd, unsigned long arg)
{
  FAR struct inode *inode = filep->f_inode;
  FAR uart_dev_t   *dev   = inode->i_private;

  return dev->ops->ioctl(filep, cmd, arg);
}

/************************************************************************************
 * Name: uart_close
 *
 * Description:
 *   This routine is called when the serial port gets closed.
 *   It waits for the last remaining data to be sent.
 *
 ************************************************************************************/

static int uart_close(FAR struct file *filep)
{
  FAR struct inode *inode = filep->f_inode;
  FAR uart_dev_t   *dev   = inode->i_private;
  irqstate_t        flags;

  uart_takesem(&dev->closesem);
  if (dev->open_count > 1)
    {
      dev->open_count--;
      uart_givesem(&dev->closesem);
      return OK;
    }

  /* There are no more references to the port */

  dev->open_count = 0;

  /* Stop accepting input */

  uart_disablerxint(dev);

  /* Now we wait for the transmit buffer to clear */

  while (dev->xmit.head != dev->xmit.tail)
    {
#ifndef CONFIG_DISABLE_SIGNALS
      usleep(500*1000);
#else
      up_mdelay(500);
#endif
    }

  /* And wait for the TX fifo to drain */

  while (!uart_txempty(dev))
    {
#ifndef CONFIG_DISABLE_SIGNALS
      usleep(500*1000);
#else
      up_mdelay(500);
#endif
    }

  /* Free the IRQ and disable the UART */

  flags = irqsave();       /* Disable interrupts */
  uart_detach(dev);        /* Detach interrupts */
  if (!dev->isconsole)     /* Check for the serial console UART */
    {
      uart_shutdown(dev);  /* Disable the UART */
    }
  irqrestore(flags);

  uart_givesem(&dev->closesem);
  return OK;
 }

/************************************************************************************
 * Name: uart_open
 *
 * Description:
 *   This routine is called whenever a serial port is opened.
 *
 ************************************************************************************/

static int uart_open(FAR struct file *filep)
{
  struct inode *inode = filep->f_inode;
  uart_dev_t   *dev   = inode->i_private;
  ubyte         tmp;
  int           ret   = OK;

  /* If the port is the middle of closing, wait until the close is finished */

  uart_takesem(&dev->closesem);

  /* Start up serial port */
  /* Increment the count of references to the device. */

  tmp = dev->open_count + 1;
  if (tmp == 0)
    {
      /* More than 255 opens; ubyte overflows to zero */

      ret = -EMFILE;
      goto errout_with_sem;
    }

  /* Check if this is the first time that the driver has been opened. */

  if (tmp == 1)
    {
      irqstate_t flags = irqsave();

      /* If this is the console, then the UART has already been initialized. */

      if (!dev->isconsole)
        {
          /* Perform one time hardware initialization */

          ret = uart_setup(dev);
          if (ret < 0)
            {
              irqrestore(flags);
              goto errout_with_sem;
            }
        }

      /* In any event, we do have to configure for interrupt driven mode of
       * operation.  Attach the hardware IRQ(s). Hmm.. should shutdown() the
       * the device in the rare case that uart_attach() fails, tmp==1, and
       * this is not the console.
       */

      ret = uart_attach(dev);
      if (ret < 0)
        {
           uart_shutdown(dev);
           irqrestore(flags);
           goto errout_with_sem;
        }

      /* Mark the io buffers empty */

      dev->xmit.head = 0;
      dev->xmit.tail = 0;
      dev->recv.head = 0;
      dev->recv.tail = 0;

      /* Enable the RX interrupt */

      uart_enablerxint(dev);
      irqrestore(flags);
    }

  /* Save the new open count on success */

  dev->open_count = tmp;

errout_with_sem:
  uart_givesem(&dev->closesem);
  return ret;
}

/************************************************************************************
 * Public Functions
 ************************************************************************************/

/************************************************************************************
 * Name: uart_register
 *
 * Description:
 *   Register serial console and serial ports.
 *
 ************************************************************************************/

int uart_register(FAR const char *path, FAR uart_dev_t *dev)
{
  sem_init(&dev->xmit.sem, 0, 1);
  sem_init(&dev->recv.sem, 0, 1);
  sem_init(&dev->closesem, 0, 1);
  sem_init(&dev->xmitsem,  0, 0);
  sem_init(&dev->recvsem,  0, 0);

  dbg("Registering %s\n", path);
  return register_driver(path, &g_serialops, 0666, dev);
}
