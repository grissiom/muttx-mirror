/****************************************************************************
 * drivers/net/dm9x.c
 *
 *   Copyright (C) 2007 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <spudmonkey@racsa.co.cr>
 *
 * References: Davicom data sheets (DM9000-DS-F03-041906.pdf,
 *   DM9010-DS-F01-103006.pdf) and looking at lots of other DM90x0
 *   drivers.
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
#if defined(CONFIG_NET) && defined(CONFIG_NET_DM90x0)

/* Force debug on for this file */

#undef  CONFIG_DEBUG
#define CONFIG_DEBUG 1
#undef  CONFIG_DEBUG_VERBOSE
#define CONFIG_DEBUG_VERBOSE 1

/* Only one hardware interface supported at present (althought there are
 * hooks throughout the design to that extending the support to multiple
 * interfaces should not be that difficult)
 */

#undef  CONFIG_DM9X_NINTERFACES
#define CONFIG_DM9X_NINTERFACES 1

#include <time.h>
#include <string.h>
#include <debug.h>
#include <wdog.h>
#include <errno.h>

#include <nuttx/irq.h>
#include <nuttx/arch.h>

#include <net/uip/uip.h>
#include <net/uip/uip-arp.h>
#include <net/uip/uip-arch.h>

/****************************************************************************
 * Definitions
 ****************************************************************************/

/* DM90000 and DM9010 register offets */

#define DM9X_NETC          0x00 /* Network control register */
#define DM9X_NETS          0x01 /* Network Status register */
#define DM9X_TXC           0x02 /* TX control register */
#define DM9X_TXS1          0x03 /* TX status register 1 */
#define DM9X_TXS2          0x03 /* TX status register 2 */
#define DM9X_RXC           0x05 /* RX control register */
#define DM9X_RXS           0x06 /* RX status register */
#define DM9X_RXOVF         0x07 /* Receive overflow counter register */
#define DM9X_BPTHRES       0x08 /* Back pressure threshold register */
#define DM9X_FCTHRES       0x09 /* Flow control threshold register */
#define DM9X_FC            0x0a /* RX/TX flow control register */
#define DM9X_EEPHYC        0x0b /* EEPROM & PHY control register */
#define DM9X_EEPHYA        0x0c /* EEPROM & PHY address register */
#define DM9X_EEPHYDL       0x0d /* EEPROM & PHY data register (lo) */
#define DM9X_EEPHYDH       0x0e /* EEPROM & PHY data register (hi) */
#define DM9X_WAKEUP        0x0f /* Wake-up control register */
#define DM9X_PAB0          0x10 /* Physical address register (byte 0) */
#define DM9X_PAB1          0x11 /* Physical address register (byte 1) */
#define DM9X_PAB2          0x12 /* Physical address register (byte 2) */
#define DM9X_PAB3          0x13 /* Physical address register (byte 3) */
#define DM9X_PAB4          0x14 /* Physical address register (byte 4) */
#define DM9X_PAB5          0x15 /* Physical address register (byte 5) */
#define DM9X_MAB0          0x16 /* Multicast address register (byte 0) */
#define DM9X_MAB1          0x17 /* Multicast address register (byte 1) */
#define DM9X_MAB2          0x18 /* Multicast address register (byte 2) */
#define DM9X_MAB3          0x19 /* Multicast address register (byte 3) */
#define DM9X_MAB4          0x1a /* Multicast address register (byte 4) */
#define DM9X_MAB5          0x1b /* Multicast address register (byte 5) */
#define DM9X_MAB6          0x1c /* Multicast address register (byte 6) */
#define DM9X_MAB7          0x1d /* Multicast address register (byte 7) */
#define DM9X_GPC           0x1e /* General purpose control register */
#define DM9X_GPD           0x1f /* General purpose register */

#define DM9X_TRPAL         0x22 /* TX read pointer address (lo) */
#define DM9X_TRPAH         0x23 /* TX read pointer address (hi) */
#define DM9X_RWPAL         0x24 /* RX write pointer address (lo) */
#define DM9X_RWPAH         0x25 /* RX write pointer address (hi) */

#define DM9X_VIDL          0x28 /* Vendor ID (lo) */
#define DM9X_VIDH          0x29 /* Vendor ID (hi) */
#define DM9X_PIDL          0x2a /* Product ID (lo) */
#define DM9X_PIDH          0x2b /* Product ID (hi) */
#define DM9X_CHIPR         0x2c /* Product ID (lo) */
#define DM9X_TXC2          0x2d /* Transmit control register 2 (dm9010) */
#define DM9X_OTC           0x2e /* Operation test control register (dm9010) */
#define DM9X_SMODEC        0x2f /* Special mode control register */
#define DM9X_ETXCSR        0x30 /* Early transmit control/status register (dm9010) */
#define DM9X_TCCR          0x31 /* Transmit checksum control register (dm9010) */
#define DM9X_RCSR          0x32 /* Receive checksum control/status register (dm9010) */
#define DM9X_EPHYA         0x33 /* External PHY address register (dm9010) */
#define DM9X_GPC2          0x34 /* General purpose control register 2 (dm9010) */
#define DM9X_GPD2          0x35 /* General purpose register 2 */
#define DM9X_GPC3          0x36 /* General purpose control register 3 (dm9010) */
#define DM9X_GPD3          0x37 /* General purpose register 3 */
#define DM9X_PBUSC         0x38 /* Processor bus control register (dm9010) */
#define DM9X_IPINC         0x39 /* INT pin control register (dm9010) */

#define DM9X_MON1          0x40 /* Monitor register 1 (dm9010) */
#define DM9X_MON2          0x41 /* Monitor register 2 (dm9010) */

#define DM9X_SCLKC         0x50 /* System clock turn ON control register (dm9010) */
#define DM9X_SCLKR         0x51 /* Resume system clock control register (dm9010) */

#define DM9X_MRCMDX        0xf0 /* Memory data pre-fetch read command without address increment */
#define DM9X_MRCMDX1       0xf1 /* memory data read command without address increment (dm9010) */
#define DM9X_MRCMD         0xf2 /* Memory data read command with address increment */
#define DM9X_MDRAL         0xf4 /* Memory data read address register (lo) */
#define DM9X_MDRAH         0xf5 /* Memory data read address register (hi) */
#define DM9X_MWCMDX        0xf6 /* Memory data write command without address increment */
#define DM9X_MWCMD         0xf8 /* Memory data write command with address increment */
#define DM9X_MDWAL         0xfa /* Memory data write address register (lo) */
#define DM9X_MDWAH         0xfb /* Memory data write address register (lo) */
#define DM9X_TXPLL         0xfc /* Memory data write address register (lo) */
#define DM9X_TXPLH         0xfd /* Memory data write address register (hi) */
#define DM9X_ISR           0xfe /* Interrupt status register */
#define DM9X_IMR           0xff /* Interrupt mask register */

/* Network control register bit definitions */

#define DM9X_NETC_RST      (1 << 0) /* Software reset */
#define DM9X_NETC_LBKM     (3 << 1) /* Loopback mode mask */
#define DM9X_NETC_LBK0     (0 << 1) /*   0: Normal */
#define DM9X_NETC_LBK1     (1 << 1) /*   1: MAC internal loopback */
#define DM9X_NETC_LBK2     (2 << 1) /*   2: Internal PHY 100M mode loopback */
#define DM9X_NETC_FDX      (1 << 3) /* Full dupliex mode */
#define DM9X_NETC_FCOL     (1 << 4) /* Force collision mode */
#define DM9X_NETC_WAKEEN   (1 << 6) /* Wakeup event enable */
#define DM9X_NETC_EXTPHY   (1 << 7) /* Select external PHY */

/* Network status bit definitions */

#define DM9X_NETS_RXOV     (1 << 1) /* RX Fifo overflow */
#define DM9X_NETS_TX1END   (1 << 2) /* TX packet 1 complete status */
#define DM9X_NETS_TX2END   (1 << 3) /* TX packet 2 complete status */
#define DM9X_NETS_WAKEST   (1 << 5) /* Wakeup event status */
#define DM9X_NETS_LINKST   (1 << 6) /* Link status */
#define DM9X_NETS_SPEED    (1 << 7) /* Media speed */

/* IMR/ISR bit definitions */

#define DM9X_INT_PR        (1 << 0) /* Packet received interrupt */
#define DM9X_INT_PT        (1 << 1) /* Packet transmitted interrupt */
#define DM9X_INT_RO        (1 << 2) /* Receive overflow interrupt */
#define DM9X_INT_ROO       (1 << 3) /* Receive overflow counter overflow int */
#define DM9X_INT_UDRUN     (1 << 4) /* Transmit underrun interrupt */
#define DM9X_INT_LNKCHG    (1 << 5) /* Link status change interrupt */
#define DM9X_INT_ALL       (0x3f)

#define DM9X_IMR_UNUSED    (1 << 6) /* (not used) */
#define DM9X_IMR_PAR       (1 << 7) /* Enable auto R/W pointer reset */

#define DM9X_ISR_IOMODEM   (3 << 6) /* IO mode mask */
#define DM9X_ISR_IOMODE8   (2 << 6) /*   IO mode = 8 bit */
#define DM9X_ISR_IOMODE16  (0 << 6) /*   IO mode = 16 bit */
#define DM9X_ISR_IOMODE32  (1 << 6) /*   IO mode = 32 bit */

#define DM9X_IMRENABLE     (DM9X_INT_PR|DM9X_INT_PT|DM9X_INT_LNKCHG|DM9X_IMR_PAR)
#define DM9X_IMRRXDISABLE  (DM9X_INT_PT|DM9X_INT_LNKCHG|DM9X_IMR_PAR)
#define DM9X_IMRDISABLE    (DM9X_IMR_PAR)

/* EEPROM/PHY control regiser bits */

#define DM9X_EEPHYC_ERRE   (1 << 0) /* EEPROM (vs PHY) access status */
#define DM9X_EEPHYC_ERPRW  (1 << 1) /* EEPROM/PHY write access */
#define DM9X_EEPHYC_ERPRR  (1 << 2) /* EEPROM/PHY read access */
#define DM9X_EEPHYC_EPOS   (1 << 3) /* EEPROM/PHY operation select */
#define DM9X_EEPHYC_WEP    (1 << 4) /* Write EEPROM enable */
#define DM9X_EEPHYC_REEP   (1 << 5) /* Reload EEPROM */

/* Supported values from the vendor and product ID register */

#define DM9X_DAVICOMVID    0x0a46
#define DM9X_DM9000PID     0x9000
#define DM9X_DM9010PID     0x9010

/* RX control register bit settings */

#define DM9X_RXC_RXEN      (1 << 0) /* RX enable */
#define DM9X_RXC_PRMSC     (1 << 1) /* Promiscuous mode */
#define DM9X_RXC_RUNT      (1 << 2) /* Pass runt packet */
#define DM9X_RXC_ALL       (1 << 3) /* Pass all multicast */
#define DM9X_RXC_DISCRC    (1 << 4) /* Discard CRC error packets */
#define DM9X_RXC_DISLONG   (1 << 5) /* Discard long packets */
#define DM9X_RXC_WTDIS     (1 << 6) /* Disable watchdog timer */
#define DM9X_RXC_HASHALL   (1 << 7) /* Filter all addresses in hash table */

#define DM9X_RXCSETUP      (DM9X_RXC_DISCRC|DM9X_RXC_DISLONG)

/* EEPHY bit settings */

#define DM9X_EEPHYA_EROA   0x40 /* PHY register address 0x01 */

#define DM9X_PKTRDY        0x01 /* Packet ready to receive */

/* The RX interrupt will be disabled if more than the following RX
 * interrupts are received back-to-back.
 */

#define DM9X_CRXTHRES 10

/* All access is via an index register and a data regist.  Select accecss
 * according to user supplied base address and bus width.
 */

#if defined(CONFIG_DM9X_BUSWIDTH8)
#  define DM9X_INDEX *(volatile uint8*)(CONFIG_DM9X_BASE)
#  define DM9X_DATA  *(volatile uint8*)(CONFIG_DM9X_BASE + 2)
#elif defined(CONFIG_DM9X_BUSWIDTH16)
#  define DM9X_INDEX *(volatile uint16*)(CONFIG_DM9X_BASE)
#  define DM9X_DATA  *(volatile uint16*)(CONFIG_DM9X_BASE + 2)
#elif defined(CONFIG_DM9X_BUSWIDTH32)
#  define DM9X_INDEX *(volatile uint32*)(CONFIG_DM9X_BASE)
#  define DM9X_DATA  *(volatile uint32*)(CONFIG_DM9X_BASE + 2)
#endif

/* Phy operating mode.  Default is AUTO, but this setting can be overridden
 * in the NuttX configuration file.
 */

#define DM9X_MODE_AUTO    0
#define DM9X_MODE_10MHD   1
#define DM9X_MODE_100MHD  2
#define DM9X_MODE_10MFD   3
#define DM9X_MODE_100MFD  4

#ifndef CONFIG_DM9X_MODE
# define CONFIG_DM9X_MODE DM9X_MODE_AUTO
#endif

// /* TX poll deley = 5 seconds. CLK_TCK is the number of clock ticks per second */

#define DM6X_WDDELAY (5*CLK_TCK)

/* TX timeout = 1 minute */

#define DM6X_TXTIMEOUT (60*CLK_TCK)

/* This is a helper pointer for accessing the contents of the Ethernet header */

#define BUF ((struct uip_eth_hdr *)dm9x->dev.d_buf)

/****************************************************************************
 * Private Types
 ****************************************************************************/

union rx_desc_u
{
  uint8 buf[4];
  struct
  {
    uint8  rxbyte;
    uint8  status;
    uint16 length;
  } desc;
};

/* The dm9x_driver_s encapsulates all DM90x0 state information for a single
 * DM90x0 hardware interface
 */

struct dm9x_driver_s
{
  boolean b100M;            /* TRUE:speed == 100M; FALSE:speed == 10M */
  WDOG_ID txpoll;           /* TX poll timer */
  WDOG_ID txtimeout;        /* TX timeout timer */
  uint8   ntxpending;       /* Count of packets pending transmission */
  uint8   ncrxpackets;      /* Number of continuous rx packets  */

  /* Mode-dependent function to move data in 8/16/32 I/O modes */

  void (*read)(uint8 *ptr, int len);
  void (*write)(const uint8 *ptr, int len);
  void (*discard)(int len);

#if defined(CONFIG_DM9X_STATS) || defined(CONFIG_DEBUG)
  uint32 ntxpackets;        /* Count of packets sent */
  uint32 ntxbytes;          /* Count of bytes sent */
  uint32 ntxerrors;         /* Count of TX errors */
  uint32 nrxpackets;        /* Count of packets received */
  uint32 nrxbytes;          /* Count of bytes received */
  uint32 nrxfifoerrors;     /* Count of RX FIFO overflow errors */
  uint32 nrxcrcerrors;      /* Count of RX CRC errors */
  uint32 nrxlengtherrors;   /* Count of RX length errors */
  uint32 nphyserrors;       /* Count of physical layer errors */
  uint32 nresets;           /* Counts number of resets */
  uint32 ntxtimeouts;       /* Counts resets caused by TX timeouts */
#endif

  /* This holds the information visible to uIP/NuttX */

  struct uip_driver_s dev;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* At present, only a single DM90x0 device is supported. */

static struct dm9x_driver_s g_dm9x[CONFIG_DM9X_NINTERFACES];

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/* Utility functions */

static uint8 getreg(int reg);
static void putreg(int reg, uint8 value);
static void read8(uint8 *ptr, int len);
static void read16(uint8 *ptr, int len);
static void read32(uint8 *ptr, int len);
static void discard8(int len);
static void discard16(int len);
static void discard32(int len);
static void write8(const uint8 *ptr, int len);
static void write16(const uint8 *ptr, int len);
static void write32(const uint8 *ptr, int len);

/* static uint16 dm9x_readsrom(struct dm9x_driver_s *dm9x, int offset); */
static uint16 dm9x_phyread(struct dm9x_driver_s *dm9x, int reg);
static void dm9x_phywrite(struct dm9x_driver_s *dm9x, int reg, uint16 value);

#if defined(CONFIG_DM9X_STATS) || defined(CONFIG_DEBUG)
static void dm9x_resetstatistics(struct dm9x_driver_s *dm9x);
#else
# define dm9x_resetstatistics(dm9x)
#endif

#if defined(CONFIG_DM9X_STATS) && defined(CONFIG_DEBUG)
static void dm9x_dumpstatistics(struct dm9x_driver_s *dm9x);
#else
# define dm9x_dumpstatistics(dm9x)
#endif

#if defined(CONFIG_DM9X_CHECKSUM)
static boolean dm9x_rxchecksumready(uint8);
#else
#  define dm9x_rxchecksumready(a) ((a) == 0x01)
#endif

/* Common TX logic */

static int  dm9x_transmit(struct dm9x_driver_s *dm9x);
static int  dm9x_uiptxpoll(struct dm9x_driver_s *dm9x);

/* Interrupt handling */

static void dm9x_receive(struct dm9x_driver_s *dm9x);
static void dm9x_txdone(struct dm9x_driver_s *dm9x);
static int  dm9x_interrupt(int irq, FAR void *context);

/* Watchdog timer expirations */

static void dm9x_polltimer(int argc, uint32 arg, ...);
static void dm9x_txtimeout(int argc, uint32 arg, ...);

/* NuttX callback functions */

static int dm9x_ifup(struct uip_driver_s *dev);
static int dm9x_ifdown(struct uip_driver_s *dev);

/* Initialization functions */

static void dm9x_bringup(struct dm9x_driver_s *dm9x);
static void dm9x_reset(struct dm9x_driver_s *dm9x);

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Function: getreg and setreg
 *
 * Description:
 *   Access to memory-mapped DM90x0 8-bit registers
 *
 * Parameters:
 *   reg - Register number
 *   value - Value to write to the register (setreg only)
 *
 * Returned Value:
 *   Value read from the register (getreg only)
 *
 * Assumptions:
 *
 ****************************************************************************/

static uint8 getreg(int reg)
{
  DM9X_INDEX = reg;
  return DM9X_DATA & 0xff;
}

static void putreg(int reg, uint8 value)
{
  DM9X_INDEX   = reg;
  DM9X_DATA = value & 0xff;
}

/****************************************************************************
 * Function: read8, read16, read32
 *
 * Description:
 *   Read packet data from the DM90x0 SRAM based on its current I/O mode
 *
 * Parameters:
 *   ptr - Location to write the packet data
 *   len - The number of bytes to read
 *
 * Returned Value:
 *   None
 *
 * Assumptions:
 *
 ****************************************************************************/

static void read8(uint8 *ptr, int len)
{
  for (; len > 0; len--)
    {
      *ptr++ = DM9X_DATA;
    }
}

static void read16(uint8 *ptr, int len)
{
  register uint16 *ptr16 = (uint16*)ptr;
  for (; len > 0; len -= sizeof(uint16))
    {
      *ptr16++ = DM9X_DATA;
    }
}

static void read32(uint8 *ptr, int len)
{
  register uint32 *ptr32 = (uint32*)ptr;
  for (; len > 0; len -= sizeof(uint32))
    {
      *ptr32++ = DM9X_DATA;
    }
}

/****************************************************************************
 * Function: discard8, discard16, discard32
 *
 * Description:
 *   Read and discard packet data in the DM90x0 SRAM based on its current
 *   I/O mode
 *
 * Parameters:
 *   len - The number of bytes to discard
 *
 * Returned Value:
 *   None
 *
 * Assumptions:
 *
 ****************************************************************************/

static void discard8(int len)
{
  for (; len > 0; len--)
    {
      DM9X_DATA;
    }
}

static void discard16(int len)
{
  for (; len > 0; len -= sizeof(uint16))
    {
      DM9X_DATA;
    }
}

static void discard32(int len)
{
  for (; len > 0; len -= sizeof(uint32))
    {
      DM9X_DATA;
    }
}

/****************************************************************************
 * Function: write8, write16, write32
 *
 * Description:
 *   Write packet data into the DM90x0 SRAM based on its current I/O mode
 *
 * Parameters:
 *   ptr - Location to write the packet data
 *   len - The number of bytes to read
 *
 * Returned Value:
 *   None
 *
 * Assumptions:
 *
 ****************************************************************************/

static void write8(const uint8 *ptr, int len)
{
  for (; len > 0; len--)
    {
      DM9X_DATA =(*ptr++ & 0xff);
    }
}

static void write16(const uint8 *ptr, int len)
{
  register uint16 *ptr16 = (uint16*)ptr;
  for (; len > 0; len -= sizeof(uint16))
    {
       DM9X_DATA = *ptr16++;
    }
}

static void write32(const uint8 *ptr, int len)
{
  register uint32 *ptr32 = (uint32*)ptr;
  for (; len > 0; len -= sizeof(uint32))
    {
      DM9X_DATA = *ptr32++;
    }
}

/****************************************************************************
 * Function: dm9x_readsrom
 *
 * Description:
 *   Read a word from SROM
 *
 * Parameters:
 *   dm9x - Reference to the driver state structure
 *   offset - SROM offset to read from
 *
 * Returned Value:
 *   SROM content at that offset
 *
 * Assumptions:
 *
 ****************************************************************************/

#if 0 /* Not used */
static uint16 dm9x_readsrom(struct dm9x_driver_s *dm9x, int offset)
{
  putreg(DM9X_EEPHYA, offset);
  putreg(DM9X_EEPHYC, DM9X_EEPHYC_ERPRR);
  up_udelay(200);
  putreg(DM9X_EEPHYC, 0x00);
  return (getreg(DM9X_EEPHYDL) + (getreg(DM9X_EEPHYDH) << 8) );
}
#endif

/****************************************************************************
 * Function: dm9x_phyread and dm9x_phywrite
 *
 * Description:
 *   Read/write data from/to the PHY
 *
 * Parameters:
 *   dm9x  - Reference to the driver state structure
 *   reg   - PHY register offset
 *   value - The value to write to the PHY register (dm9x_write only)
 *
 * Returned Value:
 *   The value read from the PHY (dm9x_read only)
 *
 * Assumptions:
 *
 ****************************************************************************/

static uint16 dm9x_phyread(struct dm9x_driver_s *dm9x, int reg)
{
  /* Setup DM9X_EEPHYA, the EEPROM/PHY address register */

  putreg(DM9X_EEPHYA, DM9X_EEPHYA_EROA | reg);

  /* Issue PHY read command pulse in the EEPROM/PHY control register */

  putreg(DM9X_EEPHYC, (DM9X_EEPHYC_ERPRR|DM9X_EEPHYC_EPOS));
  up_udelay(100);
  putreg(DM9X_EEPHYC, 0x00);

  /* Return the data from the EEPROM/PHY data register pair */

  return (((uint16)getreg(DM9X_EEPHYDH)) << 8) | (uint16)getreg(DM9X_EEPHYDL);
}

static void dm9x_phywrite(struct dm9x_driver_s *dm9x, int reg, uint16 value)
{
  /* Setup DM9X_EEPHYA, the EEPROM/PHY address register */

  putreg(DM9X_EEPHYA, DM9X_EEPHYA_EROA | reg);

  /* Put the data to write in the EEPROM/PHY data register pair */

  putreg(DM9X_EEPHYDL, (value & 0xff));
  putreg(DM9X_EEPHYDH, ((value >> 8) & 0xff));

  /* Issue PHY write command pulse in the EEPROM/PHY control register */

  putreg(DM9X_EEPHYC, (DM9X_EEPHYC_ERPRW|DM9X_EEPHYC_EPOS));
  up_udelay(500);
  putreg(DM9X_EEPHYC, 0x0);
}

/****************************************************************************
 * Function: dm9x_resetstatistics
 *
 * Description:
 *   Reset all DM90x0 statistics
 *
 * Parameters:
 *   dm9x  - Reference to the driver state structure
 *
 * Returned Value:
 *   None
 *
 * Assumptions:
 *
 ****************************************************************************/

#if defined(CONFIG_DM9X_STATS) || defined(CONFIG_DEBUG)
static void dm9x_resetstatistics(struct dm9x_driver_s *dm9x)
{
  dm9x->ntxpackets      = 0; /* Count of packets sent */
  dm9x->ntxbytes        = 0; /* Count of bytes sent */
  dm9x->ntxerrors       = 0; /* Count of TX errors */
  dm9x->nrxpackets      = 0; /* Count of packets received */
  dm9x->nrxbytes        = 0; /* Count of bytes received */
  dm9x->nrxfifoerrors   = 0; /* Count of RX FIFO overflow errors */
  dm9x->nrxcrcerrors    = 0; /* Count of RX CRC errors */
  dm9x->nrxlengtherrors = 0; /* Count of RX length errors */
  dm9x->nphyserrors     = 0; /* Count of physical layer errors */
  dm9x->nresets         = 0; /* Counts number of resets */
  dm9x->ntxtimeouts     = 0; /* Counts resets caused by TX timeouts */
}
#else
# define dm9x_resetstatistics(dev)
#endif

/****************************************************************************
 * Function: dm9x_dumpstatistics
 *
 * Description:
 *   Print the current value of all DM90x0 statistics
 *
 * Parameters:
 *   dm9x  - Reference to the driver state structure
 *
 * Returned Value:
 *   None
 *
 * Assumptions:
 *
 ****************************************************************************/

#if defined(CONFIG_DM9X_STATS) && defined(CONFIG_DEBUG)
static void dm9x_dumpstatistics(struct dm9x_driver_s *dm9x)
{
  dbg("TX packets:            %d\n", dm9x->ntxpackets);
  dbg("   bytes:              %d\n", dm9x->ntxbytes);
  dbg("   errors:             %d\n", dm9x->ntxerrors);
  dbg("RX packets:            %d\n", dm9x->nrxpackets);
  dbg("   bytes:              %d\n", dm9x->nrxbytes);
  dbg("   FIFO overflows:     %d\n", dm9x->nrxfifoerrors);
  dbg("   CRC errors:         %d\n", dm9x->nrxcrcerrors);
  dbg("   length errors:      %d\n", dm9x->nrxlengtherrors);
  dbg("Physical layer errors: %d\n", dm9x->nphyserrors);
  dbg("Resets:                %d\n", dm9x->nresets);
  dbg("TX timeout resets:     %d\n", dm9x->ntxtimeouts);
}
#endif

/****************************************************************************
 * Function: dm9x_rxchecksumready
 *
 * Description:
 *   Return TRUE if the RX checksum is available
 *
 * Parameters:
 *   rxbyte
 *
 * Returned Value:
 *   TRUE: checksum is ready
 *
 * Assumptions:
 *
 ****************************************************************************/

#if defined(CONFIG_DM9X_CHECKSUM)
static inline boolean dm9x_rxchecksumready(uint8 rxbyte)
{
  if ((rxbyte & 0x01) == 0)
    {
      return FALSE;
    }

  return ((rxbyte >> 4) | 0x01) != 0;
}
#endif

/****************************************************************************
 * Function: dm9x_transmit
 *
 * Description:
 *   Start hardware transmission.  Called either from the txdone interrupt
 *   handling or from watchdog based polling.
 *
 * Parameters:
 *   dm9x  - Reference to the driver state structure
 *
 * Returned Value:
 *   OK on success; a negated errno on failure
 *
 * Assumptions:
 *
 ****************************************************************************/

static int dm9x_transmit(struct dm9x_driver_s *dm9x)
{
  /* Increment count of packets transmitted */

  dm9x->ntxpending++;
#if defined(CONFIG_DM9X_STATS) || defined(CONFIG_DEBUG)
  dm9x->ntxpackets++;
  dm9x->ntxbytes += dm9x->dev.d_len;
#endif

  /* Disable all DM90x0 interrupts */

  putreg(DM9X_IMR, DM9X_IMRDISABLE);

  /* Set the TX length */

  putreg(DM9X_TXPLL, (dm9x->dev.d_len & 0xff));
  putreg(DM9X_TXPLH, (dm9x->dev.d_len >> 8) & 0xff);

  /* Move the data to be sent into TX SRAM */

  DM9X_INDEX = DM9X_MWCMD;
  dm9x->write(dm9x->dev.d_buf, dm9x->dev.d_len);

#if !defined(CONFIG_DM9X_ETRANS)
  /* Issue TX polling command */

  putreg(DM9X_TXC, 0x1); /* Cleared after TX complete*/
#endif

  /* Clear count of back-to-back RX packet transfers */

  dm9x->ncrxpackets = 0;

  /* Re-enable DM90x0 interrupts */

  putreg(DM9X_IMR, DM9X_IMRENABLE);

  /* Setup the TX timeout watchdog (perhaps restarting the timer) */

  (void)wd_start(dm9x->txtimeout, DM6X_TXTIMEOUT, dm9x_txtimeout, 1, (uint32)dm9x);
  return OK;
}

/****************************************************************************
 * Function: dm9x_uiptxpoll
 *
 * Description:
 *   The transmitter is available, check if uIP has any outgoing packets ready
 *   to send.  This may be called:
 *
 *   1. When the preceding TX packet send is complete,
 *   2. When the preceding TX packet send timesout and the DM90x0 is reset
 *   3. During normal TX polling
 *
 * Parameters:
 *   dm9x  - Reference to the driver state structure
 *
 * Returned Value:
 *   OK on success; a negated errno on failure
 *
 * Assumptions:
 *
 ****************************************************************************/

static int dm9x_uiptxpoll(struct dm9x_driver_s *dm9x)
{
  int i;

  for(i = 0; i < UIP_CONNS; i++)
    {
      uip_tcppoll(&dm9x->dev, i);

      /* If the above function invocation resulted in data that
       * should be sent out on the network, the global variable
       * d_len is set to a value > 0.
       */

      if (dm9x->dev.d_len > 0)
        {
          uip_arp_out(&dm9x->dev);
          dm9x_transmit(dm9x);

          /* Check if there is room in the DM90x0 to hold another packet.  In 100M mode,
           * that can be 2 packets, otherwise it is a single packet.
          */

          if (dm9x->ntxpending > 1 || !dm9x->b100M)
            {
              return OK;
            }
        }
    }

#ifdef CONFIG_NET_UDP
  for(i = 0; i < UIP_UDP_CONNS; i++)
    {
      uip_udppoll(&dm9x->dev,i);

      /* If the above function invocation resulted in data that
       * should be sent out on the network, the global variable
       * d_len is set to a value > 0.
       */

      if (dm9x->dev.d_len > 0)
        {
          uip_arp_out(&dm9x->dev);
          dm9x_transmit(dm9x);

          /* Check if there is room in the DM90x0 to hold another packet.  In 100M mode,
           * that can be 2 packets, otherwise it is a single packet.
          */

          if (dm9x->ntxpending > 1 || !dm9x->b100M)
            {
              return OK;
            }
        }
    }
#endif /* CONFIG_NET_UDP */

  return OK;
}

/****************************************************************************
 * Function: dm9x_uiptxpoll
 *
 * Description:
 *   An interrupt was received indicating the availability of a new RX packet
 *
 * Parameters:
 *   dm9x  - Reference to the driver state structure
 *
 * Returned Value:
 *   None
 *
 * Assumptions:
 *
 ****************************************************************************/

static void dm9x_receive(struct dm9x_driver_s *dm9x)
{
  union rx_desc_u rx;
  boolean bchecksumready;
  uint8   mdrah;
  uint8   mdral;
  uint8   rxbyte;

  dbg("Packet received\n");

  do
    {
      /* Store the value of memory data read address register */

      mdrah = getreg(DM9X_MDRAH);
      mdral = getreg(DM9X_MDRAL);

      getreg(DM9X_MRCMDX);       /* Dummy read */
      rxbyte = (uint8)DM9X_DATA; /* Get the most up-to-date data */

      /* Packet ready for receive check */

      bchecksumready = dm9x_rxchecksumready(rxbyte);
      if (!bchecksumready)
        {
          break;
        }

      /* A packet is ready now. Get status/length */

      DM9X_INDEX = DM9X_MRCMD; /* set read ptr ++ */

      /* Read packet status & length */

      dm9x->read((uint8*)&rx, 4);

      /* Check if any errors were reported by the hardware */

      if (rx.desc.status & 0xbf)
        {
          /* Bad RX packet... update statistics */

#if defined(CONFIG_DM9X_STATS) || defined(CONFIG_DEBUG)
          if (rx.desc.status & 0x01) 
            {
              dm9x->nrxfifoerrors++;
              dbg("RX FIFO error: %d\n", dm9x->nrxfifoerrors);
            }

          if (rx.desc.status & 0x02) 
            {
              dm9x->nrxcrcerrors++;
              dbg("RX CRC error: %d\n", dm9x->nrxcrcerrors);
            }

          if (rx.desc.status & 0x80) 
            {
              dm9x->nrxlengtherrors++;
              dbg("RX length error: %d\n", dm9x->nrxlengtherrors);
            }

          if (rx.desc.status & 0x08)
            {
              dm9x->nphyserrors++;
              dbg("Physical Layer error: %d\n", dm9x->nphyserrors);
            }
#endif
          /* Drop this packet and continue to check the next packet */

          dm9x->discard(rx.desc.length);
        }

      /* Also check if the packet is a valid size for the uIP configuration */

      else if (rx.desc.length < UIP_LLH_LEN || rx.desc.length > (UIP_BUFSIZE + 2))
        {
#if defined(CONFIG_DM9X_STATS) || defined(CONFIG_DEBUG)
          dm9x->nrxlengtherrors++;
          dbg("RX length error: %d\n", dm9x->nrxlengtherrors);
#endif
          /* Drop this packet and continue to check the next packet */

          dm9x->discard(rx.desc.length);
        }
      else
        {
          /* Good packet... Copy the packet data out of SRAM and pass it one to uIP */

          dm9x->dev.d_len = rx.desc.length;
          dm9x->read(dm9x->dev.d_buf, rx.desc.length);

          /* We only accept IP packets of the configured type and ARP packets */

#ifdef CONFIG_NET_IPv6
          if (BUF->type == htons(UIP_ETHTYPE_IP6))
#else
          if (BUF->type == htons(UIP_ETHTYPE_IP))
#endif
            {
              uip_arp_ipin();
              uip_input(&dm9x->dev);

             /* If the above function invocation resulted in data that
              * should be sent out on the network, the global variable
              * d_len is set to a value > 0.
              */

              if (dm9x->dev.d_len > 0)
                {
                  uip_arp_out(&dm9x->dev);
                  dm9x_transmit(dm9x);
                }
            }
          else if (BUF->type == htons(UIP_ETHTYPE_ARP))
            {
              uip_arp_arpin(&dm9x->dev);

              /* If the above function invocation resulted in data that
               * should be sent out on the network, the global variable
               * d_len is set to a value > 0.
               */

              if (dm9x->dev.d_len > 0)
                {
                  dm9x_transmit(dm9x);
                }
            }
        }

#if defined(CONFIG_DM9X_STATS) || defined(CONFIG_DEBUG)
      dm9x->nrxpackets++;
      dm9x->nrxbytes += rx.desc.length;
#endif
      dm9x->ncrxpackets++;
    }
  while ((rxbyte & 0x01) == DM9X_PKTRDY && dm9x->ncrxpackets < DM9X_CRXTHRES);
  dbg("All RX packets processed\n");
}

/****************************************************************************
 * Function: dm9x_txdone
 *
 * Description:
 *   An interrupt was received indicating that the last TX packet(s) is done
 *
 * Parameters:
 *   dm9x  - Reference to the driver state structure
 *
 * Returned Value:
 *   None
 *
 * Assumptions:
 *
 ****************************************************************************/

static void dm9x_txdone(struct dm9x_driver_s *dm9x)
{
  int  nsr;

  dbg("TX done\n");

  /* Another packet has completed transmission.  Decrement the count of
   * of pending TX transmissions.
   */

  nsr = getreg(DM9X_NETS);
  if (nsr & DM9X_NETS_TX1END)
    {
      if (dm9x->ntxpending)
        {
          dm9x->ntxpending--;
        }
      else
        {
          dbg("ntxpending ERROR on TX1END\n");
        }
    }

  if (nsr & DM9X_NETS_TX2END)
    {
      if (dm9x->ntxpending)
        {
          dm9x->ntxpending--;
        }
      else
        {
          dbg("ntxpending ERROR on TX2END\n");
        }
    }

  /* Cancel the TX timeout */

  if (dm9x->ntxpending == 0)
    {
      wd_cancel(dm9x->txtimeout);
    }

  /* Then poll uIP for new XMIT data */

  (void)dm9x_uiptxpoll(dm9x);
}

/****************************************************************************
 * Function: dm9x_interrupt
 *
 * Description:
 *   DM90x0 interrupt handler
 *
 * Parameters:
 *   irq     - Number of the IRQ that generated the interrupt
 *   context - Interrupt register state save info (architecture-specific)
 *
 * Returned Value:
 *   OK on success
 *
 * Assumptions:
 *
 ****************************************************************************/

static int dm9x_interrupt(int irq, FAR void *context)
{
  register struct dm9x_driver_s *dm9x = &g_dm9x[0];
  uint8 isr;
  uint8 save;
  int i;

  /* Save previous register address */

  save = (uint8)DM9X_INDEX;

  /* Disable all DM90x0 interrupts */

  putreg(DM9X_IMR, DM9X_IMRDISABLE); 

  /* Get and clear the DM90x0 interrupt status bits */

  isr = getreg(DM9X_ISR);
  putreg(DM9X_ISR, isr);
  vdbg("Interrupt: ISR=%02x\n", isr);

  /* Check for link status change */

  if (isr & DM9X_INT_LNKCHG)
    {
      /* Wait up to 0.5s for link OK */

      for (i = 0; i < 500; i++)
        {
          dm9x_phyread(dm9x,0x1);
          if (dm9x_phyread(dm9x,0x1) & 0x4) /*Link OK*/
            {
              /* Wait to get detected speed */

              for (i = 0; i < 200; i++)
                {
                  up_mdelay(1);
                }

              /* Set the new network speed */

              if (dm9x_phyread(dm9x, 0) & 0x2000)
                {
                  dm9x->b100M = TRUE;
                }
              else
                {
                  dm9x->b100M = FALSE;
                }
              break;
            }
          up_mdelay(1);
        }
      dbg("delay: %d mS speed: %s\n", i, dm9x->b100M ? "100M" : "10M"); 
    }

 /* Check if we received an incoming packet */

 if (isr & DM9X_INT_PR)
    {
      dm9x_receive(dm9x);
    }

 /* Check if we are able to transmit a packet */

 if (isr & DM9X_INT_PT)
    {
      dm9x_txdone(dm9x);
    }

  /* If the number of consecutive receive packets exceeds a threshold, 
   * then disable the RX interrupt.
   */

  if (dm9x->ncrxpackets >= DM9X_CRXTHRES)
    {
      /* Eanble all DM90x0 interrupts EXCEPT for RX */

      putreg(DM9X_IMR, DM9X_IMRRXDISABLE);
    }
  else
    {
      /* Enable all DM90x0 interrupts */

      putreg(DM9X_IMR, DM9X_IMRENABLE);
    }

  /* Restore previous register address */

  DM9X_INDEX = save;
  return OK;
}

/****************************************************************************
 * Function: dm9x_txtimeout
 *
 * Description:
 *   Our TX watchdog timed out.  Called from the timer interrupt handler.
 *   The last TX never completed.  Reset the DM90x0 and start again.
 *
 * Parameters:
 *   argc - The number of available arguments
 *   arg  - The first argument
 *
 * Returned Value:
 *   None
 *
 * Assumptions:
 *
 ****************************************************************************/

static void dm9x_txtimeout(int argc, uint32 arg, ...)
{
  struct dm9x_driver_s *dm9x = (struct dm9x_driver_s *)arg;

  dbg("TX timeout\n");

  /* Increment statistics and dump debug info */

#if defined(CONFIG_DM9X_STATS) || defined(CONFIG_DEBUG)
  dm9x->ntxtimeouts++;
  dm9x->ntxerrors++;
#endif

  dbg("  TX packet count:           %d\n", dm9x->ntxpending); 
  dbg("  TX timeouts:               %d\n", dm9x->ntxtimeouts); 
  dbg("  TX read pointer address:   0x%02x:%02x\n",
      getreg(DM9X_TRPAH), getreg(DM9X_TRPAL));
  dbg("  Memory data write address: 0x%02x:%02x (DM9010)\n",
      getreg(DM9X_MDWAH), getreg(DM9X_MDWAL));

  /* Then reset the DM90x0 */

  dm9x_reset(dm9x);

  /* Then poll uIP for new XMIT data */

  (void)dm9x_uiptxpoll(dm9x);
}

/****************************************************************************
 * Function: dm9x_polltimer
 *
 * Description:
 *   Periodic timer handler.  Called from the timer interrupt handler.
 *
 * Parameters:
 *   argc - The number of available arguments
 *   arg  - The first argument
 *
 * Returned Value:
 *   None
 *
 * Assumptions:
 *
 ****************************************************************************/

static void dm9x_polltimer(int argc, uint32 arg, ...)
{
  struct dm9x_driver_s *dm9x = (struct dm9x_driver_s *)arg;

  dbg("Poll timer expiration\n");

  /* If the number of contiguous RX packets exceeds a threshold, reset the counter and
   * re-enable RX interrupts
   */

  if (dm9x->ncrxpackets >= DM9X_CRXTHRES)
    {
      dm9x->ncrxpackets = 0;
      putreg(DM9X_IMR, DM9X_IMRENABLE);
    }

  /* Check if there is room in the DM90x0 to hold another packet.  In 100M mode,
   * that can be 2 packets, otherwise it is a single packet.
   */

  if (dm9x->ntxpending < 1 || (dm9x->b100M && dm9x->ntxpending < 2))
    {
      /* If so, poll uIP for new XMIT data */

      (void)dm9x_uiptxpoll(dm9x);
    }

  /* Setup the watchdog poll timer again */

  (void)wd_start(dm9x->txpoll, DM6X_WDDELAY, dm9x_polltimer, 1, arg);
}

/****************************************************************************
 * Function: dm9x_phymode
 *
 * Description:
 *   Configure the PHY operating mode
 *
 * Parameters:
 *   dm9x  - Reference to the driver state structure
 *
 * Returned Value:
 *   None
 *
 * Assumptions:
 *
 ****************************************************************************/

static inline void dm9x_phymode(struct dm9x_driver_s *dm9x)
{
  uint16 phyreg0;
  uint16 phyreg4;

#if CONFIG_DM9X_MODE == DM9X_MODE_AUTO
  phyreg0 = 0x1200;  /* Auto-negotiation & Restart Auto-negotiation */
  phyreg4 = 0x01e1;  /* Default flow control disable*/
#elif CONFIG_DM9X_MODE == DM9X_MODE_10MHD
  phyreg4 = 0x21; 
  phyreg0 = 0x1000;
#elif CONFIG_DM9X_MODE == DM9X_MODE_10MFD
  phyreg4 = 0x41; 
  phyreg0 = 0x1100;
#elif CONFIG_DM9X_MODE == DM9X_MODE_100MHD
  phyreg4 = 0x81; 
  phyreg0 = 0x3000;
#elif CONFIG_DM9X_MODE == DM9X_MODE_100MFD
  phyreg4 = 0x101; 
  phyreg0 = 0x3100;
#else
# error "Recognized PHY mode"
#endif

  dm9x_phywrite(dm9x, 0, phyreg0);
  dm9x_phywrite(dm9x, 4, phyreg4);
}

/****************************************************************************
 * Function: dm9x_phymode
 *
 * Description:
 *   NuttX Callback: Bring up the DM90x0 interface when an IP address is
 *   provided 
 *
 * Parameters:
 *   dev  - Reference to the NuttX driver state structure
 *
 * Returned Value:
 *   None
 *
 * Assumptions:
 *
 ****************************************************************************/

static int dm9x_ifup(struct uip_driver_s *dev)
{
  struct dm9x_driver_s *dm9x = (struct dm9x_driver_s *)dev->d_private;
  uint8 netstatus;
  int i;

  dbg("Bringing the interface up\n" );

  /* Initilize DM90x0 chip */

  dm9x_bringup(dm9x);

  /* Check link state and media speed (waiting up to 3s for link OK) */

  dm9x->b100M = FALSE;
  for (i = 0; i < 3000; i++)
    {
      netstatus = getreg(DM9X_NETS);
      if (netstatus & DM9X_NETS_LINKST)
        {
          /* Link OK... Wait a bit before getting the detected speed */

          up_mdelay(200);
          netstatus = getreg(DM9X_NETS);
          if ((netstatus & DM9X_NETS_SPEED) == 0)
            {
              dm9x->b100M = TRUE;
            }
          break;
        }
      i++;
      up_mdelay(1);
    }

  dbg("i=%d mS speed=%s\n", i, dm9x->b100M ? "100M" : "10M"); 

  /* Set and activate a timer process */

  (void)wd_start(dm9x->txpoll, DM6X_WDDELAY, dm9x_polltimer, 1, (uint32)dm9x);

  /* Enable the DM9X interrupt */

  up_enable_irq(CONFIG_DM9X_IRQ);
  return OK;
}

/****************************************************************************
 * Function: dm9x_phymode
 *
 * Description:
 *   NuttX Callback: Stop the interface.
 *
 * Parameters:
 *   dev  - Reference to the NuttX driver state structure
 *
 * Returned Value:
 *   None
 *
 * Assumptions:
 *
 ****************************************************************************/

static int dm9x_ifdown(struct uip_driver_s *dev)
{
  struct dm9x_driver_s *dm9x = (struct dm9x_driver_s *)dev->d_private;
  irqstate_t flags;

  dbg("Stopping\n");

  /* Disable the DM9X interrupt */

  flags = irqsave();
  up_disable_irq(CONFIG_DM9X_IRQ);

  /* Cancel the TX poll timer and TX timeout timers */

  wd_cancel(dm9x->txpoll);
  wd_cancel(dm9x->txtimeout);

  /* Reset the device */

  dm9x_phywrite(dm9x, 0x00, 0x8000);  /* PHY RESET */
  putreg(DM9X_GPD, 0x01);             /* Power-down PHY (GEPIO0=1) */
  putreg(DM9X_IMR, DM9X_IMRDISABLE);  /* Disable all interrupts */
  putreg(DM9X_RXC, 0x00);             /* Disable RX */
  putreg(DM9X_ISR, DM9X_INT_ALL);     /* Clear interrupt status */
  irqrestore(flags);

  /* Dump statistics */

  dm9x_dumpstatistics(dm9x);
  return OK;
}

/****************************************************************************
 * Function: dm9x_bringup
 *
 * Description:
 *   Initialize the dm90x0 chip
 *
 * Parameters:
 *   dm9x  - Reference to the driver state structure
 *
 * Returned Value:
 *   None
 *
 * Assumptions:
 *
 ****************************************************************************/

static void dm9x_bringup(struct dm9x_driver_s *dm9x)
{
  dbg("Initializing\n");

  /* Set the internal PHY power-on, GPIOs normal, and wait 2ms */

  putreg(DM9X_GPD, 0x01);  /* Power-down the PHY (GEPIO0=1) */
  up_udelay(500);
  putreg(DM9X_GPD, 0x00);  /* Preactivate PHY (GPIO0=0 */
  up_udelay(20);              /* Wait 2ms for PHY power-on ready */

  /* Do a software reset and wait 20us (twice).  The reset autoclears
   * in 10us; 20us guarantees completion of the reset
   */

  putreg(DM9X_NETC, (DM9X_NETC_RST|DM9X_NETC_LBK1));
  up_udelay(20);
  putreg(DM9X_NETC, (DM9X_NETC_RST|DM9X_NETC_LBK1));
  up_udelay(20);

  /* Configure I/O mode */

  switch (getreg(DM9X_ISR) & DM9X_ISR_IOMODEM)
    {
      case DM9X_ISR_IOMODE8:
        dm9x->read    = read8;
        dm9x->write   = write8;
        dm9x->discard = discard8;
        break;

      case DM9X_ISR_IOMODE16:
        dm9x->read    = read16;
        dm9x->write   = write16;
        dm9x->discard = discard16;
        break;

      case DM9X_ISR_IOMODE32:
        dm9x->read    = read32;
        dm9x->write   = write32;
        dm9x->discard = discard32;
        break;

      default:
        break;
    }

  /* Set PHY operating mode */

  dm9x_phymode(dm9x);

  /* Program operating register */

  putreg(DM9X_NETC, 0x00);    /* Network control */
  putreg(DM9X_TXC, 0x00);     /* TX Polling clear */
  putreg(DM9X_BPTHRES, 0x3f); /* Less 3kb, 600us */
  putreg(DM9X_SMODEC, 0x00);  /* Special mode */
  putreg(DM9X_NETS, (DM9X_NETS_WAKEST|DM9X_NETS_TX1END|DM9X_NETS_TX2END)); /* Clear TX status */
  putreg(DM9X_ISR, DM9X_INT_ALL); /* Clear interrupt status */

#if defined(CONFIG_DM9X_CHECKSUM)
  putreg(DM9X_TCCR, 0x07);    /* TX UDP/TCP/IP checksum enable */
  putreg(DM9X_RCSR, 0x02);    /* Receive checksum enable */
#endif

#if defined(CONFIG_DM9X_ETRANS)
  putreg(DM9X_ETXCSR, 0x83);
#endif

  /* Initialize statistics */

  dm9x->ncrxpackets = 0; /* Number of continuous RX packets  */
  dm9x->ntxpending  = 0; /* Number of pending TX packets */
  dm9x_resetstatistics(dm9x);

  /* Activate DM9000A/DM9010 */

  putreg(DM9X_RXC, DM9X_RXCSETUP | 1); /* RX enable */
  putreg(DM9X_IMR, DM9X_IMRENABLE);    /* Enable TX/RX interrupts */
}

/****************************************************************************
 * Function: dm9x_reset
 *
 * Description:
 *   Stop, reset, re-initialize, and restart the DM90x0 chip and driver.  At
 *   present, the chip is only reset after a TX timeout.
 *
 * Parameters:
 *   dm9x  - Reference to the driver state structure
 *
 * Returned Value:
 *   None
 *
 * Assumptions:
 *
 ****************************************************************************/

static void dm9x_reset(struct dm9x_driver_s *dm9x)
{
  uint8 save;
  int i;

  /* Cancel the TX poll timer and TX timeout timers */

  wd_cancel(dm9x->txpoll);
  wd_cancel(dm9x->txtimeout);

  /* Save previous register address */

  save = (uint8)DM9X_INDEX;

  dm9x->nresets++;
  dm9x_bringup(dm9x);

  /* Wait up to 1 second for the link to be OK */

  dm9x->b100M = FALSE;
  for (i = 0; i < 1000; i++)
    {
      if (dm9x_phyread(dm9x,0x1) & 0x4) /*Link OK*/
        {
          if (dm9x_phyread(dm9x, 0) &0x2000)
            {
              dm9x->b100M = TRUE;
            }
          break;
        }
      up_mdelay(1);
    }

  /* Restore previous register address */

  DM9X_INDEX = save;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Function: dm9x_initialize
 *
 * Description:
 *   Initialize the DM90x0 driver
 *
 * Parameters:
 *   None
 *
 * Returned Value:
 *   OK on success; Negated errno on failure.
 *
 * Assumptions:
 *
 ****************************************************************************/

/* Initialize the DM90x0 chip and driver */

int dm9x_initialize(void)
{
  uint8 *mptr;
  uint16 vid;
  uint16 pid;
  int i;
  int j;

  /* Get the chip vendor ID and product ID */

  vid = (((uint16)getreg(DM9X_VIDH)) << 8) | (uint16)getreg(DM9X_VIDL);
  pid = (((uint16)getreg(DM9X_PIDH)) << 8) | (uint16)getreg(DM9X_PIDL);
  dbg("I/O base: %08x VID: %04x PID: %04x\n", CONFIG_DM9X_BASE, vid, pid);

  /* Check if a DM90x0 chip is recognized at this I/O base */

  if (vid != DM9X_DAVICOMVID || (pid != DM9X_DM9000PID && pid != DM9X_DM9010PID))
    {
      dbg("DM90x0 vender/product ID not found at this base address\n");
      return -ENODEV;
    }

  /* Attach the IRQ to the driver */

  if (irq_attach(CONFIG_DM9X_IRQ, dm9x_interrupt))
    {
      /* We could not attach the ISR to the ISR */
      dbg("irq_attach() failed\n");
      return -EAGAIN;
    }

  /* Initialize the driver structure */

  memset(g_dm9x, 0, CONFIG_DM9X_NINTERFACES*sizeof(struct dm9x_driver_s));
  g_dm9x[0].dev.ifup   = dm9x_ifup;
  g_dm9x[0].dev.ifdown = dm9x_ifdown;

  /* Create a watchdog for timing polling for and timing of transmisstions */

  g_dm9x[0].txpoll     = wd_create();
  g_dm9x[0].txtimeout  = wd_create();

  /* Read the MAC address */

  mptr = g_dm9x[0].dev.d_mac.addr;
  for (i = 0, j = DM9X_PAB0; i < 6; i++, j++)
    {
      mptr[i] = getreg(j);
    }

  dbg("MAC: %0x:%0x:%0x:%0x:%0x:%0x",
      mptr[0], mptr[1], mptr[2], mptr[3], mptr[4], mptr[5]);

  return 0;
}

#endif /* CONFIG_NET && CONFIG_NET_DM90x0 */

