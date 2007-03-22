/************************************************************
 * up_serial.c
 *
 *   Copyright (C) 2007 Gregory Nutt. All rights reserved.
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
 * 3. Neither the name Gregory Nutt nor the names of its contributors may be
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
 ************************************************************/

/************************************************************
 * Included Files
 ************************************************************/

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
#include "dm320.h"
#include "os_internal.h"
#include "up_internal.h"

/************************************************************
 * Definitions
 ************************************************************/

#define BASE_BAUD 115200

/************************************************************
 * Private Types
 ************************************************************/

struct up_dev_s
{
  uint32               uartbase;	/* Base address of UART
					 * registers */
  uint32               baud;		/* Configured baud */
  uint16               msr;		/* Saved MSR value */
  ubyte                irq;		/* IRQ associated with
					 * this UART */
  ubyte                parity;		/* 0=none, 1=odd, 2=even */
  ubyte                bits;		/* Number of bits (7 or 8) */
  boolean              stopbits2;	/* TRUE: Configure with 2
					 * stop bits instead of 1 */
};

/************************************************************
 * Private Function Prototypes
 ************************************************************/

static int     up_setup(struct uart_dev_s *dev);
static void    up_shutdown(struct uart_dev_s *dev);
static int     up_interrupt(int irq, void *context);
static int     up_ioctl(struct file *filep, int cmd, unsigned long arg);
static int     up_receive(struct uart_dev_s *dev, uint32 *status);
static void    up_rxint(struct uart_dev_s *dev, boolean enable);
static boolean up_rxfifonotempty(struct uart_dev_s *dev);
static void    up_send(struct uart_dev_s *dev, int ch);
static void    up_txint(struct uart_dev_s *dev, boolean enable);
static boolean up_txfifonotfull(struct uart_dev_s *dev);
static boolean up_txfifoempty(struct uart_dev_s *dev);

/************************************************************
 * Private Variables
 ************************************************************/

struct uart_ops_s g_uart_ops =
{
  .setup          = up_setup,
  .shutdown       = up_shutdown,
  .handler        = up_interrupt,
  .ioctl          = up_ioctl,
  .receive        = up_receive,
  .rxint          = up_rxint,
  .rxfifonotempty = up_rxfifonotempty,
  .send           = up_send,
  .txint          = up_txint,
  .txfifonotfull  = up_txfifonotfull,
  .txfifoempty    = up_txfifoempty,
};

/* I/O buffers */

static char g_uart0rxbuffer[CONFIG_UART0_RXBUFSIZE];
static char g_uart0txbuffer[CONFIG_UART0_TXBUFSIZE];
static char g_uart1rxbuffer[CONFIG_UART1_RXBUFSIZE];
static char g_uart1txbuffer[CONFIG_UART1_TXBUFSIZE];

/* This describes the state of the DM320 uart0 port. */

static struct up_dev_s g_uart0priv =
{
  .uartbase       = DM320_UART0_REGISTER_BASE,
  .baud           = CONFIG_UART0_BAUD,
  .parity         = CONFIG_UART0_PARITY,
  .bits           = CONFIG_UART0_BITS,
  .stopbits2      = CONFIG_UART0_2STOP,
};

static uart_dev_t g_uart0port =
{
  .irq      = DM320_IRQ_UART0,
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

/* This describes the state of the DM320 uart1 port. */

static struct up_dev_s g_uart1priv =
{
  .uartbase       = DM320_UART1_REGISTER_BASE,
  .baud           = CONFIG_UART1_BAUD,
  .parity         = CONFIG_UART1_PARITY,
  .bits           = CONFIG_UART1_BITS,
  .stopbits2      = CONFIG_UART1_2STOP,
};

static uart_dev_t g_uart1port =
{
  .irq      = DM320_IRQ_UART1,
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

/************************************************************
 * Private Functions
 ************************************************************/

/************************************************************
 * Name: up_serialin
 ************************************************************/

static inline uint16 up_serialin(struct up_dev_s *priv, uint32 offset)
{
  return getreg16(priv->uartbase + offset);
}

/************************************************************
 * Name: up_serialout
 ************************************************************/

static inline void up_serialout(struct up_dev_s *priv, uint32 offset, uint16 value)
{
  putreg16(value, priv->uartbase + offset);
}

/************************************************************
 * Name: up_disableuartint
 ************************************************************/

static inline void up_disableuartint(struct up_dev_s *priv, uint16 *msr)
{
  if (msr)
    {
      *msr = priv->msr & UART_MSR_ALLIE;
    }

  priv->msr &= ~UART_MSR_ALLIE;
  up_serialout(priv, UART_MSR, priv->msr);
}

/************************************************************
 * Name: up_restoreuartint
 ************************************************************/

static inline void up_restoreuartint(struct up_dev_s *priv, uint16 msr)
{
  priv->msr |= msr & UART_MSR_ALLIE;
  up_serialout(priv, UART_MSR, priv->msr);
}

/************************************************************
 * Name: up_waittxfifonotfull
 ************************************************************/

static inline void up_waittxfifonotfull(struct up_dev_s *priv)
{
  int tmp;

  for (tmp = 1000 ; tmp > 0 ; tmp--)
    {
      if ((up_serialin(priv, UART_SR) & UART_SR_TFTI) != 0)
        {
          break;
        }
    }
}

/************************************************************
 * Name: up_enablebreaks
 ************************************************************/

static inline void up_enablebreaks(struct up_dev_s *priv, boolean enable)
{
  uint16 lcr = up_serialin(priv, UART_LCR);
  if (enable)
    {
      lcr |= UART_LCR_BOC;
    }
  else
    {
      lcr &= ~UART_LCR_BOC;
    }
  up_serialout(priv, UART_LCR, lcr);
}

/************************************************************
 * Name: up_setup
 *
 * Description:
 *   Configure the UART baud, bits, parity, fifos, etc. This
 *   method is called the first time that the serial port is
 *   opened.
 *
 ************************************************************/

static int up_setup(struct uart_dev_s *dev)
{
#ifdef CONFIG_SUPPRESS_UART_CONFIG
  struct up_dev_s *priv = (struct up_dev_s*)dev->priv;
  uint16 brsr;

  /* Clear fifos */

  up_serialout(priv, UART_DM320_RFCR, 0x8000);
  up_serialout(priv, UART_DM320_TFCR, 0x8000);

  /* Set rx and tx triggers */

  up_serialout(priv, UART_DM320_RFCR, UART_RFCR_RTL_1);
  up_serialout(priv, UART_DM320_TFCR, UART_TFCR_TTL_16);

  /* Set up the MSR */

  priv->msr = up_serialin(priv, UART_MSR);
  if (priv->bits == 7)
    {
      priv->msr |= UART_DATABIT_7;
    }
  else
    {
      priv->msr &= ~UART_MSR_CLS;
    }

  if (priv->stopbits2)
    {
      priv->msr |= UART_STOPBIT_2;
    }
  else
    {
      priv->msr &= ~UART_MSR_SBLS;
    }

  if (priv->parity == 1)
    {
      priv->msr |= UART_ODDPARITY;
    }
  else if (priv->parity == 2)
    {
      priv->msr |= UART_EVENPARITY;
    }
  else
    {
      priv->msr &= ~(UART_MSR_PSB|UART_MSR_PEB);
    }

  /* Set up the BRSR */

  switch (dev->baud)
    {
      case 2400:
        brsr = UART_BAUD_2400;
        break;
      case 4800:
        brsr = UART_BAUD_4800;
        break;
      default:
      case 9600:
        brsr = UART_BAUD_9600;
        break;
      case 14400:
        brsr = UART_BAUD_14400;
        break;
      case 19200:
        brsr = UART_BAUD_19200;
        break;
      case 28800:
        brsr = UART_BAUD_28800;
        break;
      case 3840:
        brsr = UART_BAUD_38400;
        break;
      case 57600:
        brsr = UART_BAUD_57600;
        break;
      case 115200:
        brsr = UART_BAUD_115200;
        break;
      case 230400:
        brsr = UART_BAUD_230400;
        break;
      case 460800:
        brsr = UART_BAUD_460800;
        break;
      case 921600:
        brsr = UART_BAUD_921600;
        break;
    }

  /* Setup the new UART configuration */

  up_serialout(priv,UART_MSR, priv->msr);
  up_serialout(priv, UART_DM320_BRSR, brsr);
  up_enablebreaks(priv, FALSE);
#endif
  return OK;
}

/************************************************************
 * Name: up_shutdown
 *
 * Description:
 *   Disable the UART.  This method is called when the serial
 *   port is closed
 *
 ************************************************************/

static void up_shutdown(struct uart_dev_s *dev)
{
  struct up_dev_s *priv = (struct up_dev_s*)dev->priv;
  up_disableuartint(priv, NULL);
}

/************************************************************
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
 ************************************************************/

static int up_interrupt(int irq, void *context)
{
  struct uart_dev_s *dev = NULL;
  struct up_dev_s   *priv;
  uint16             status;
  int                passes = 0;

  if (g_uart1port.irq == irq)
    {
      dev = &g_uart1port;
    }
  else if (g_uart0port.irq == irq)
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

  for(;;)
    {
      /* Get the current UART status and check for loop
       * termination conditions
       */

      status  = up_serialin(priv, UART_SR);
      status &= (UART_SR_RFTI | UART_SR_TFTI);

      if (status == 0 || passes > 256) 
        {
          return OK;
        }

      /* Handline incoming, receive bytes */

      if (status & UART_SR_RFTI)
        {
          uart_recvchars(dev);
        }

      /* Handle outgoing, transmit bytes */

      if (status & UART_SR_TFTI)
        {
          uart_xmitchars(dev);
        }

      /* Keep track of how many times we do this in case there
       * is some hardware failure condition.
       */

      passes++;
    }
}

/************************************************************
 * Name: up_ioctl
 *
 * Description:
 *   All ioctl calls will be routed through this method
 *
 ************************************************************/

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

/************************************************************
 * Name: up_receive
 *
 * Description:
 *   Called (usually) from the interrupt level to receive one
 *   character from the UART.  Error bits associated with the
 *   receipt are provided in the the return 'status'.
 *
 ************************************************************/

static int up_receive(struct uart_dev_s *dev, uint32 *status)
{
  struct up_dev_s *priv = (struct up_dev_s*)dev->priv;
  uint16 dtrr;

  dtrr    = up_serialin(priv, UART_DTRR);
  *status = dtrr;
  return dtrr & UART_DTRR_DTR_MASK;
}

/************************************************************
 * Name: up_rxint
 *
 * Description:
 *   Call to enable or disable RX interrupts
 *
 ************************************************************/

static void up_rxint(struct uart_dev_s *dev, boolean enable)
{
  struct up_dev_s *priv = (struct up_dev_s*)dev->priv;
  if (enable)
    {
#ifndef CONFIG_SUPPRESS_SERIAL_INTS
      priv->msr |= UART_MSR_RFTIE;
#endif
    }
  else
    {
      priv->msr &= ~UART_MSR_RFTIE;
    }
  up_serialout(priv, UART_MSR, priv->msr);
}

/************************************************************
 * Name: up_rxfifonotempty
 *
 * Description:
 *   Return TRUE if the receive fifo is not empty
 *
 ************************************************************/

static boolean up_rxfifonotempty(struct uart_dev_s *dev)
{
  struct up_dev_s *priv = (struct up_dev_s*)dev->priv;
  return ((up_serialin(priv, UART_SR) & UART_SR_RFNEF) != 0);
}

/************************************************************
 * Name: up_send
 *
 * Description:
 *   This method will send one byte on the UART
 *
 ************************************************************/

static void up_send(struct uart_dev_s *dev, int ch)
{
  struct up_dev_s *priv = (struct up_dev_s*)dev->priv;
  up_serialout(priv, UART_DTRR, (uint16)ch);
}

/************************************************************
 * Name: up_txint
 *
 * Description:
 *   Call to enable or disable TX interrupts
 *
 ************************************************************/

static void up_txint(struct uart_dev_s *dev, boolean enable)
{
  struct up_dev_s *priv = (struct up_dev_s*)dev->priv;
  if (enable)
    {
#ifndef CONFIG_SUPPRESS_SERIAL_INTS
      priv->msr |= UART_MSR_TFTIE;
#endif
    }
  else
    {
      priv->msr &= ~UART_MSR_TFTIE;
    }
  up_serialout(priv, UART_MSR, priv->msr);
}

/************************************************************
 * Name: up_txfifonotfull
 *
 * Description:
 *   Return TRUE if the tranmsit fifo is not full
 *
 ************************************************************/

static boolean up_txfifonotfull(struct uart_dev_s *dev)
{
  struct up_dev_s *priv = (struct up_dev_s*)dev->priv;
  return ((up_serialin(priv, UART_SR) & UART_SR_TFTI) != 0);
}

/************************************************************
 * Name: up_txfifoempty
 *
 * Description:
 *   Return TRUE if the transmit fifo is empty
 *
 ************************************************************/

static boolean up_txfifoempty(struct uart_dev_s *dev)
{
  struct up_dev_s *priv = (struct up_dev_s*)dev->priv;
  return ((up_serialin(priv, UART_SR) & UART_SR_TREF) == 0);
}

/************************************************************
 * Public Funtions
 ************************************************************/

/************************************************************
 * Name: up_serialinit
 *
 * Description:
 *   Performs the low level UART initialization early in 
 *   debug so that the serial console will be available
 *   during bootup.  This must be called before up_serialinit.
 *
 ************************************************************/

void up_earlyserialinit(void)
{
  up_disableuartint(TTYS0_DEV.priv, NULL);
  up_disableuartint(TTYS1_DEV.priv, NULL);

  CONSOLE_DEV.isconsole = TRUE;
  up_setup(&CONSOLE_DEV);
}

/************************************************************
 * Name: up_serialinit
 *
 * Description:
 *   Register serial console and serial ports.  This assumes
 *   that up_earlyserialinit was called previously.
 *
 ************************************************************/

void up_serialinit(void)
{
  (void)uart_register("/dev/console", &CONSOLE_DEV);
  (void)uart_register("/dev/ttyS0", &TTYS0_DEV);
  (void)uart_register("/dev/ttyS1", &TTYS1_DEV);
}

/************************************************************
 * Name: up_putc
 *
 * Description:
 *   Provide priority, low-level access to support OS debug
 *   writes
 *
 ************************************************************/

int up_putc(int ch)
{
  struct up_dev_s *priv = (struct up_dev_s*)CONSOLE_DEV.priv;
  uint16  ier;

  up_disableuartint(priv, &ier);
  up_waittxfifonotfull(priv);
  up_serialout(priv, UART_DTRR, (uint16)ch);

  /* Check for LF */

  if (ch == '\n')
    {
      /* Add CR */

      up_waittxfifonotfull(priv);
      up_serialout(priv, UART_DTRR, '\r');
    }

  up_waittxfifonotfull(priv);
  up_restoreuartint(priv, ier);
  return ch;
}

