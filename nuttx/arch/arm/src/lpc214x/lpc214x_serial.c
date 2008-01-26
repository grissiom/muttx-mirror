/****************************************************************************
 * lpc214x/lpc214x_serial.c
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
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <sys/types.h>
#include <unistd.h>
#include <semaphore.h>
#include <string.h>
#include <errno.h>
#include <debug.h>
#include <nuttx/irq.h>
#include <nuttx/arch.h>
#include <nuttx/serial.h>
#include <arch/serial.h>

#include "up_arch.h"
#include "os_internal.h"
#include "up_internal.h"

#include "lpc214x_uart.h"

/****************************************************************************
 * Definitions
 ****************************************************************************/

#define BASE_BAUD 115200

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct up_dev_s
{
  uint32  uartbase;  /* Base address of UART registers */
  uint32  baud;      /* Configured baud */
  ubyte   ier;       /* Saved IER value */
  ubyte   irq;       /* IRQ associated with this UART */
  ubyte   parity;    /* 0=none, 1=odd, 2=even */
  ubyte   bits;      /* Number of bits (7 or 8) */
  boolean stopbits2; /* TRUE: Configure with 2 stop bits instead of 1 */
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int     up_setup(struct uart_dev_s *dev);
static void    up_shutdown(struct uart_dev_s *dev);
static int     up_attach(struct uart_dev_s *dev);
static void    up_detach(struct uart_dev_s *dev);
static int     up_interrupt(int irq, void *context);
static int     up_ioctl(struct file *filep, int cmd, unsigned long arg);
static int     up_receive(struct uart_dev_s *dev, uint32 *status);
static void    up_rxint(struct uart_dev_s *dev, boolean enable);
static boolean up_rxavailable(struct uart_dev_s *dev);
static void    up_send(struct uart_dev_s *dev, int ch);
static void    up_txint(struct uart_dev_s *dev, boolean enable);
static boolean up_txready(struct uart_dev_s *dev);
static boolean up_txempty(struct uart_dev_s *dev);

/****************************************************************************
 * Private Variables
 ****************************************************************************/

struct uart_ops_s g_uart_ops =
{
  .setup          = up_setup,
  .shutdown       = up_shutdown,
  .attach         = up_attach,
  .detach         = up_detach,
  .ioctl          = up_ioctl,
  .receive        = up_receive,
  .rxint          = up_rxint,
  .rxavailable    = up_rxavailable,
  .send           = up_send,
  .txint          = up_txint,
  .txready        = up_txready,
  .txempty    = up_txempty,
};

/* I/O buffers */

static char g_uart0rxbuffer[CONFIG_UART0_RXBUFSIZE];
static char g_uart0txbuffer[CONFIG_UART0_TXBUFSIZE];
static char g_uart1rxbuffer[CONFIG_UART1_RXBUFSIZE];
static char g_uart1txbuffer[CONFIG_UART1_TXBUFSIZE];

/* This describes the state of the LPC214X uart0 port. */

static struct up_dev_s g_uart0priv =
{
  .uartbase       = LPC214X_UART0_BASE,
  .baud           = CONFIG_UART0_BAUD,
  .irq            = LPC214X_UART0_IRQ,
  .parity         = CONFIG_UART0_PARITY,
  .bits           = CONFIG_UART0_BITS,
  .stopbits2      = CONFIG_UART0_2STOP,
};

static uart_dev_t g_uart0port =
{
  .recv     =
  {
    .size   = CONFIG_UART0_RXBUFSIZE,
    .buffer = g_uart0rxbuffer,
  },
  .xmit     =
  {
    .size   = CONFIG_UART0_TXBUFSIZE,
    .buffer = g_uart0txbuffer,
  },
  .ops      = &g_uart_ops,
  .priv     = &g_uart0priv,
};

/* This describes the state of the LPC214X uart1 port. */

static struct up_dev_s g_uart1priv =
{
  .uartbase       = LPC214X_UART1_BASE,
  .baud           = CONFIG_UART1_BAUD,
  .irq            = LPC214X_UART1_IRQ,
  .parity         = CONFIG_UART1_PARITY,
  .bits           = CONFIG_UART1_BITS,
  .stopbits2      = CONFIG_UART1_2STOP,
};

static uart_dev_t g_uart1port =
{
  .recv     =
  {
    .size   = CONFIG_UART1_RXBUFSIZE,
    .buffer = g_uart1rxbuffer,
  },
  .xmit     =
  {
    .size   = CONFIG_UART1_TXBUFSIZE,
    .buffer = g_uart1txbuffer,
   },
  .ops      = &g_uart_ops,
  .priv     = &g_uart1priv,
};

/* Now, which one with be tty0/console and which tty1? */

#ifdef CONFIG_SERIAL_IRDA_CONSOLE
# define CONSOLE_DEV     g_uart1port
# define TTYS0_DEV       g_uart1port
# define TTYS1_DEV       g_uart0port
#else
# define CONSOLE_DEV     g_uart0port
# define TTYS0_DEV       g_uart0port
# define TTYS1_DEV       g_uart1port
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: up_serialin
 ****************************************************************************/

static inline ubyte up_serialin(struct up_dev_s *priv, int offset)
{
  return getreg16(priv->uartbase + offset);
}

/****************************************************************************
 * Name: up_serialout
 ****************************************************************************/

static inline void up_serialout(struct up_dev_s *priv, int offset, ubyte value)
{
  putreg16(value, priv->uartbase + offset);
}

/****************************************************************************
 * Name: up_disableuartint
 ****************************************************************************/

static inline void up_disableuartint(struct up_dev_s *priv, ubyte *ier)
{
  if (ier)
    {
      *ier = priv->ier & LPC214X_IER_ALLIE;
    }

  priv->ier &= ~LPC214X_IER_ALLIE;
  up_serialout(priv, LPC214X_UART_IER_OFFSET, priv->ier);
}

/****************************************************************************
 * Name: up_restoreuartint
 ****************************************************************************/

static inline void up_restoreuartint(struct up_dev_s *priv, ubyte ier)
{
  priv->ier |= ier & LPC214X_IER_ALLIE;
  up_serialout(priv, LPC214X_UART_IER_OFFSET, priv->ier);
}

/****************************************************************************
 * Name: up_waittxready
 ****************************************************************************/

static inline void up_waittxready(struct up_dev_s *priv)
{
  int tmp;

  /* Limit how long we will wait for the TX available condition */
  for (tmp = 1000 ; tmp > 0 ; tmp--)
    {
      /* Check if the tranmitter holding register (THR) is empty */
      if ((up_serialin(priv, LPC214X_UART_LSR_OFFSET) & LPC214X_LSR_THRE) != 0)
        {
          /* The THR is empty, return */
          break;
        }
    }
}

/****************************************************************************
 * Name: up_enablebreaks
 ****************************************************************************/

static inline void up_enablebreaks(struct up_dev_s *priv, boolean enable)
{
  ubyte lcr = up_serialin(priv, LPC214X_UART_LCR_OFFSET);
  if (enable)
    {
      lcr |= LPC214X_LCR_BREAK_ENABLE;
    }
  else
    {
      lcr &= ~LPC214X_LCR_BREAK_ENABLE;
    }
  up_serialout(priv, LPC214X_UART_LCR_OFFSET, lcr);
}

/****************************************************************************
 * Name: up_setup
 *
 * Description:
 *   Configure the UART baud, bits, parity, fifos, etc. This
 *   method is called the first time that the serial port is
 *   opened.
 *
 ****************************************************************************/

static int up_setup(struct uart_dev_s *dev)
{
#ifdef CONFIG_SUPPRESS_LPC214X_UART_CONFIG
  struct up_dev_s *priv = (struct up_dev_s*)dev->priv;
  uint16 baud;
  ubyte lcr;

  /* Clear fifos */

  up_serialout(priv, LPC214X_UART_FCR_OFFSET, (LPC214X_FCR_RX_FIFO_RESET|LPC214X_FCR_TX_FIFO_RESET));
 
  /* Set trigger */

  up_serialout(priv, LPC214X_UART_FCR_OFFSET, (LPC214X_FCR_FIFO_ENABLE|LPC214X_FCR_FIFO_TRIG14));

  /* Set up the IER */

  priv->ier = up_serialin(priv, LPC214X_UART_IER_OFFSET);
  
  /* Set up the LCR */
  
  lcr = 0;
  
  if (priv->bits == 7)
    {
      lcr |= LPC214X_LCR_CHAR_7;
    }
  else
    {
      lcr |= LPC214X_LCR_CHAR_8;
    }

  if (priv->stopbits2)
    {
      lcr |= LPC214X_LCR_STOP_2;
    }
 
  if (priv->parity == 1)
    {
      lcr |= LPC214X_LCR_PAR_ODD;
    }
  else if (priv->parity == 2)
    {
      lcr |= LPC214X_LCR_PAR_EVEN;
    }

  /* Enter DLAB=1 */
    
  up_serialout(priv, LPC214X_UART_LCR_OFFSET, (lcr | LPC214X_LCR_DLAB_ENABLE));
 
  /* Set the BAUD divisor */

  baud = UART_BAUD(priv->baud);
  up_serialout(priv, LPC214X_UART_DLM_OFFSET, baud >> 8);
  up_serialout(priv, LPC214X_UART_DLL_OFFSET, baud & 0xff);
 
  /* Clear DLAB */

 up_serialout(priv, LPC214X_UART_LCR_OFFSET, lcr);
#endif
  return OK;
}

/****************************************************************************
 * Name: up_shutdown
 *
 * Description:
 *   Disable the UART.  This method is called when the serial
 *   port is closed
 *
 ****************************************************************************/

static void up_shutdown(struct uart_dev_s *dev)
{
  struct up_dev_s *priv = (struct up_dev_s*)dev->priv;
  up_disableuartint(priv, NULL);
}

/****************************************************************************
 * Name: up_attach
 *
 * Description:
 *   Configure the UART to operation in interrupt driven mode.  This method is
 *   called when the serial port is opened.  Normally, this is just after the
 *   the setup() method is called, however, the serial console may operate in
 *   a non-interrupt driven mode during the boot phase.
 *
 *   RX and TX interrupts are not enabled when by the attach method (unless the
 *   hardware supports multiple levels of interrupt enabling).  The RX and TX
 *   interrupts are not enabled until the txint() and rxint() methods are called.
 *
 ****************************************************************************/

static int up_attach(struct uart_dev_s *dev)
{
  struct up_dev_s *priv = (struct up_dev_s*)dev->priv;
  int ret;

  /* Attach and enable the IRQ */

  ret = irq_attach(priv->irq, up_interrupt);
  if (ret == OK)
    {
       /* Enable the interrupt (RX and TX interrupts are still disabled
        * in the UART
        */

       up_enable_irq(priv->irq);
    }
  return ret;
}

/****************************************************************************
 * Name: up_detach
 *
 * Description:
 *   Detach UART interrupts.  This method is called when the serial port is
 *   closed normally just before the shutdown method is called.  The exception is
 *   the serial console which is never shutdown.
 *
 ****************************************************************************/

static void up_detach(struct uart_dev_s *dev)
{
  struct up_dev_s *priv = (struct up_dev_s*)dev->priv;
  up_disable_irq(priv->irq);
  irq_detach(priv->irq);
}

/****************************************************************************
 * Name: up_interrupt
 *
 * Description:
 *   This is the UART interrupt handler.  It will be invoked
 *   when an interrupt received on the 'irq'  It should call
 *   uart_transmitchars or uart_receivechar to perform the
 *   appropriate data transfers.  The interrupt handling logic\
 *   must be able to map the 'irq' number into the approprite
 *   uart_dev_s structure in order to call these functions.
 *
 ****************************************************************************/

static int up_interrupt(int irq, void *context)
{
  struct uart_dev_s *dev = NULL;
  struct up_dev_s   *priv;
  ubyte              status;
  int                passes;

  if (g_uart1priv.irq == irq)
    {
      dev = &g_uart1port;
    }
  else if (g_uart0priv.irq == irq)
    {
      dev = &g_uart0port;
    }
  else
    {
      PANIC(OSERR_INTERNAL);
    }
  priv = (struct up_dev_s*)dev->priv;

  /* Loop until there are no characters to be transferred or,
   * until we have been looping for a long time.
   */

  for( passes = 0; passes < 256; passes++)
    {
      /* Get the current UART status and check for loop
       * termination conditions
       */

       status  = up_serialin(priv, LPC214X_UART_IIR_OFFSET);
      
      /* The NO INTERRUPT should be zero */
      
      if (status  !=  LPC214X_IIR_NO_INT)
        {
          /* Handline incoming, receive bytes (with or without timeout) */

          if (status == LPC214X_IIR_RDA_INT || status == LPC214X_IIR_CTI_INT)
            {
              uart_recvchars(dev);
            }

          /* Handle outgoing, transmit bytes */

          else if (status == LPC214X_IIR_THRE_INT)
            {
              uart_xmitchars(dev);
            }

          /* Just clear modem status interrupts */
        
          else if (status == LPC214X_IIR_MS_INT)
           {
             /* Read the modem status regisgter (MSR) to clear */
            
             (void)up_serialin(priv, LPC214X_UART_MSR_OFFSET);
           }

          /* Just clear any line status interrupts */
        
          else if (status == LPC214X_IIR_RLS_INT)
            {
              /* Read the line status register (LSR) to clear */
            
              (void)up_serialin(priv, LPC214X_UART_LSR_OFFSET);
            }
        }
    }
    return OK;
}

/****************************************************************************
 * Name: up_ioctl
 *
 * Description:
 *   All ioctl calls will be routed through this method
 *
 ****************************************************************************/

static int up_ioctl(struct file *filep, int cmd, unsigned long arg)
{
  struct inode      *inode = filep->f_inode;
  struct uart_dev_s *dev   = inode->i_private;
  struct up_dev_s   *priv  = (struct up_dev_s*)dev->priv;
  int                ret    = OK;

  switch (cmd)
    {
    case TIOCSERGSTRUCT:
      {
         struct up_dev_s *user = (struct up_dev_s*)arg;
         if (!user)
           {
             *get_errno_ptr() = EINVAL;
             ret = ERROR;
           }
         else
           {
             memcpy(user, dev, sizeof(struct up_dev_s));
           }
       }
       break;

    case TIOCSBRK:  /* BSD compatibility: Turn break on, unconditionally */
      {
        irqstate_t flags = irqsave();
        up_enablebreaks(priv, TRUE);
        irqrestore(flags);
      }
      break;

    case TIOCCBRK:  /* BSD compatibility: Turn break off, unconditionally */
      {
        irqstate_t flags;
        flags = irqsave();
        up_enablebreaks(priv, FALSE);
        irqrestore(flags);
      }
      break;

    default:
      *get_errno_ptr() = ENOTTY;
      ret = ERROR;
      break;
    }

  return ret;
}

/****************************************************************************
 * Name: up_receive
 *
 * Description:
 *   Called (usually) from the interrupt level to receive one
 *   character from the UART.  Error bits associated with the
 *   receipt are provided in the the return 'status'.
 *
 ****************************************************************************/

static int up_receive(struct uart_dev_s *dev, uint32 *status)
{
  struct up_dev_s *priv = (struct up_dev_s*)dev->priv;
  ubyte rbr;

  rbr     = up_serialin(priv, LPC214X_UART_RBR_OFFSET);
  *status = up_serialin(priv, LPC214X_UART_RBR_OFFSET);
  return rbr;
}

/****************************************************************************
 * Name: up_rxint
 *
 * Description:
 *   Call to enable or disable RX interrupts
 *
 ****************************************************************************/

static void up_rxint(struct uart_dev_s *dev, boolean enable)
{
  struct up_dev_s *priv = (struct up_dev_s*)dev->priv;
  if (enable)
    {
#ifndef CONFIG_SUPPRESS_SERIAL_INTS
      priv->ier |= LPC214X_IER_ERBFI;
#endif
    }
  else
    {
      priv->ier &= ~LPC214X_IER_ERBFI;
    }
  up_serialout(priv, LPC214X_UART_IER_OFFSET, priv->ier);
}

/****************************************************************************
 * Name: up_rxavailable
 *
 * Description:
 *   Return TRUE if the receive fifo is not empty
 *
 ****************************************************************************/

static boolean up_rxavailable(struct uart_dev_s *dev)
{
  struct up_dev_s *priv = (struct up_dev_s*)dev->priv;
  return ((up_serialin(priv, LPC214X_UART_LSR_OFFSET) & LPC214X_LSR_RDR) != 0);
}

/****************************************************************************
 * Name: up_send
 *
 * Description:
 *   This method will send one byte on the UART
 *
 ****************************************************************************/

static void up_send(struct uart_dev_s *dev, int ch)
{
  struct up_dev_s *priv = (struct up_dev_s*)dev->priv;
  up_serialout(priv, LPC214X_UART_THR_OFFSET, (ubyte)ch);
}

/****************************************************************************
 * Name: up_txint
 *
 * Description:
 *   Call to enable or disable TX interrupts
 *
 ****************************************************************************/

static void up_txint(struct uart_dev_s *dev, boolean enable)
{
  struct up_dev_s *priv = (struct up_dev_s*)dev->priv;
  if (enable)
    {
#ifndef CONFIG_SUPPRESS_SERIAL_INTS
      priv->ier |= LPC214X_IER_ETBEI;
#endif
    }
  else
    {
      priv->ier &= ~LPC214X_IER_ETBEI;
    }
  up_serialout(priv, LPC214X_UART_IER_OFFSET, priv->ier);
}

/****************************************************************************
 * Name: up_txready
 *
 * Description:
 *   Return TRUE if the tranmsit fifo is not full
 *
 ****************************************************************************/

static boolean up_txready(struct uart_dev_s *dev)
{
  struct up_dev_s *priv = (struct up_dev_s*)dev->priv;
  return ((up_serialin(priv, LPC214X_UART_LSR_OFFSET) & LPC214X_LSR_THRE) != 0);
}

/****************************************************************************
 * Name: up_txempty
 *
 * Description:
 *   Return TRUE if the transmit fifo is empty
 *
 ****************************************************************************/

static boolean up_txempty(struct uart_dev_s *dev)
{
  struct up_dev_s *priv = (struct up_dev_s*)dev->priv;
  return ((up_serialin(priv, LPC214X_UART_LSR_OFFSET) & LPC214X_LSR_THRE) != 0);
}

/****************************************************************************
 * Public Funtions
 ****************************************************************************/

/****************************************************************************
 * Name: up_serialinit
 *
 * Description:
 *   Performs the low level UART initialization early in 
 *   debug so that the serial console will be available
 *   during bootup.  This must be called before up_serialinit.
 *
 ****************************************************************************/

void up_earlyserialinit(void)
{
  /* Enable UART0 and 1 */
  
  uint32 pinsel = getreg32(LPC214X_PINSEL0);
  pinsel &= ~(LPC214X_UART0_PINMASK|LPC214X_UART1_PINMASK);
  pinsel |= (LPC214X_UART0_PINSEL|LPC214X_UART1_PINSEL);
  putreg32(pinsel, LPC214X_PINSEL0);

  /* Disable both UARTS */
  
  up_disableuartint(TTYS0_DEV.priv, NULL);
  up_disableuartint(TTYS1_DEV.priv, NULL);

  /* Configuration whichever on is the console */
  
  CONSOLE_DEV.isconsole = TRUE;
  up_setup(&CONSOLE_DEV);
}

/****************************************************************************
 * Name: up_serialinit
 *
 * Description:
 *   Register serial console and serial ports.  This assumes
 *   that up_earlyserialinit was called previously.
 *
 ****************************************************************************/

void up_serialinit(void)
{
  (void)uart_register("/dev/console", &CONSOLE_DEV);
  (void)uart_register("/dev/ttyS0", &TTYS0_DEV);
  (void)uart_register("/dev/ttyS1", &TTYS1_DEV);
}

/****************************************************************************
 * Name: up_putc
 *
 * Description:
 *   Provide priority, low-level access to support OS debug
 *   writes
 *
 ****************************************************************************/

int up_putc(int ch)
{
  struct up_dev_s *priv = (struct up_dev_s*)CONSOLE_DEV.priv;
  ubyte  ier;

  up_disableuartint(priv, &ier);
  up_waittxready(priv);
  up_serialout(priv, LPC214X_UART_THR_OFFSET, (ubyte)ch);

  /* Check for LF */

  if (ch == '\n')
    {
      /* Add CR */

      up_waittxready(priv);
      up_serialout(priv, LPC214X_UART_THR_OFFSET, '\r');
    }

  up_waittxready(priv);
  up_restoreuartint(priv, ier);
  return ch;
}

