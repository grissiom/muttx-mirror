/****************************************************************************
 * arch/arm/src/lpc17xx/lpc17_ethernet.c
 *
 *   Copyright (C) 2010 Gregory Nutt. All rights reserved.
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
#if defined(CONFIG_NET) && defined(CONFIG_LPC17_ETHERNET)

#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
#include <debug.h>
#include <wdog.h>
#include <errno.h>

#include <nuttx/irq.h>
#include <nuttx/arch.h>
#include <nuttx/mii.h>

#include <net/uip/uip.h>
#include <net/uip/uip-arp.h>
#include <net/uip/uip-arch.h>

#include "chip.h"
#include "up_arch.h"
#include "lpc17_syscon.h"
#include "lpc17_ethernet.h"
#include "lpc17_internal.h"

#include <arch/board/board.h>

/* Does this chip have and ethernet controller? */

#if LPC17_NETHCONTROLLERS > 0

/****************************************************************************
 * Definitions
 ****************************************************************************/

/* CONFIG_LPC17_NINTERFACES determines the number of physical interfaces
 * that will be supported -- unless it is more than actually supported by the
 * hardware!
 */

#if !defined(CONFIG_LPC17_NINTERFACES) || CONFIG_LPC17_NINTERFACES > LPC17_NETHCONTROLLERS
#  undef CONFIG_LPC17_NINTERFACES
#  define CONFIG_LPC17_NINTERFACES LPC17_NETHCONTROLLERS
#endif

/* The logic here has a few hooks for support for multiple interfaces, but
 * that capability is not yet in place (and I won't worry about it until I get
 * the first multi-interface LPC17xx).
 */

#if CONFIG_LPC17_NINTERFACES > 1
#  warning "Only a single ethernet controller is supported"
#  undef CONFIG_LPC17_NINTERFACES
#  define CONFIG_LPC17_NINTERFACES 1
#endif

/* If IGMP is enabled, then accept multi-cast frames. */

#if defined(CONFIG_NET_IGMP) && !defined(CONFIG_NET_MULTICAST)
#  define CONFIG_NET_MULTICAST 1
#endif

/* If the user did not specify a priority for Ethernet interrupts, set the
 * interrupt priority to the maximum.
 */

#ifndef CONFIG_NET_PRIORITY
#  define CONFIG_NET_PRIORITY NVIC_SYSH_PRIORITY_MAX
#endif

/* TX poll deley = 1 seconds. CLK_TCK is the number of clock ticks per second */

#define LPC17_WDDELAY        (1*CLK_TCK)
#define LPC17_POLLHSEC       (1*2)

/* TX timeout = 1 minute */

#define LPC17_TXTIMEOUT      (60*CLK_TCK)

/* Interrupts */

#define ETH_RXINTS           (ETH_INT_RXOVR | ETH_INT_RXERR | ETH_INT_RXFIN | ETH_INT_RXDONE)
#define ETH_TXINTS           (ETH_INT_TXUNR | ETH_INT_TXERR | ETH_INT_TXFIN | ETH_INT_TXDONE)

/* This is a helper pointer for accessing the contents of the Ethernet header */

#define BUF ((struct uip_eth_hdr *)priv->lp_dev.d_buf)

/* Select PHY-specific values.  Add more PHYs as needed. */

#ifdef CONFIG_PHY_KS8721
#  define LPC17_PHYNAME      "KS8721"
#  define LPC17_PHYID1       MII_PHYID1_KS8721
#  define LPC17_PHYID2       MII_PHYID2_KS8721
#  define LPC17_HAVE_PHY     1
#else
#  warning "No PHY specified!"
#  undef LPC17_HAVE_PHY
#endif

#define MII_BIG_TIMEOUT      666666

/* These definitions are used to remember the speed/duplex settings */

#define LPC17_SPEED_MASK     0x01
#define LPC17_SPEED_100      0x01
#define LPC17_SPEED_10       0x00

#define LPC17_DUPLEX_MASK    0x02
#define LPC17_DUPLEX_FULL    0x02
#define LPC17_DUPLEX_HALF    0x00

#define LPC17_10BASET_HD     (LPC17_SPEED_10  | LPC17_DUPLEX_HALF)
#define LPC17_10BASET_FD     (LPC17_SPEED_10  | LPC17_DUPLEX_FULL)
#define LPC17_100BASET_HD    (LPC17_SPEED_100 | LPC17_DUPLEX_HALF)
#define LPC17_100BASET_FD    (LPC17_SPEED_100 | LPC17_DUPLEX_FULL)

#ifdef CONFIG_PHY_SPEED100
#  ifdef CONFIG_PHY_FDUPLEX
#    define LPC17_MODE_DEFLT LPC17_100BASET_FD
#  else
#    define LPC17_MODE_DEFLT LPC17_100BASET_HD
#  endif
#else
#  ifdef CONFIG_PHY_FDUPLEX
#    define LPC17_MODE_DEFLT LPC17_10BASET_FD
#  else
#    define LPC17_MODE_DEFLT LPC17_10BASET_HD
#  endif
#endif

/* This is the number of ethernet GPIO pins that must be configured */

#define GPIO_NENET_PINS      10

/* EMAC DMA RAM and descriptor definitions.  The configured number of
 * descriptors will determine the organization and the size of the
 * descriptor and status tables.  There is a complex interaction between
 * the maximum packet size (CONFIG_NET_BUFSIZE) and the number of 
 * Rx and Tx descriptors that can be suppored (CONFIG_NET_NRXDESC and
 * CONFIG_NET_NTXDESC): Small buffers -> more packets.  This is
 * something that needs to be tuned for you system.
 *
 * For a 16Kb SRAM region, here is the relationship:
 *
 *  16384 <= ntx * (pktsize + 8 + 4) + nrx * (pktsize + 8 + 8)
 *
 * If ntx == nrx and pktsize == 424, then you could have
 * ntx = nrx = 18.
 *
 * An example with all of the details:
 *
 * NTXDESC=18 NRXDESC=18 CONFIG_NET_BUFSIZE=420:
 *   LPC17_TXDESCTAB_SIZE = 18*8 = 144
 *   LPC17_TXSTATTAB_SIZE = 18*4 =  72
 *   LPC17_TXTAB_SIZE     = 216
 *
 *   LPC17_RXDESCTAB_SIZE = 16*8 = 144
 *   LPC17_RXSTATTAB_SIZE = 16*8 = 144
 *   LPC17_TXTAB_SIZE     = 288
 *
 *   LPC17_DESCTAB_SIZE   = 504
 *   LPC17_DESC_BASE      = LPC17_SRAM_BANK0 + 0x00004000 - 504
 *                        = LPC17_SRAM_BANK0 + 0x00003e08
 *   LPC17_TXDESC_BASE    = LPC17_SRAM_BANK0 + 0x00003e08
 *   LPC17_TXSTAT_BASE    = LPC17_SRAM_BANK0 + 0x00003e98
 *   LPC17_RXDESC_BASE    = LPC17_SRAM_BANK0 + 0x00003ee0
 *   LPC17_RXSTAT_BASE    = LPC17_SRAM_BANK0 + 0x00003f70
 *
 *   LPC17_PKTMEM_BASE    = LPC17_SRAM_BANK0
 *   LPC17_PKTMEM_SIZE    = 0x00004000-504 = 0x00003e40
 *   LPC17_PKTMEM_END     = LPC17_SRAM_BANK0 + 0x00003e08

 *   LPC17_MAXPACKET_SIZE = ((420 + 3 + 2) & ~3) = 424
 *   LPC17_NTXPKTS        = 18
 *   LPC17_NRXPKTS        = 18

 *   LPC17_TXBUFFER_SIZE  = 18 * 424 = 0x00001dd0
 *   LPC17_RXBUFFER_SIZE  = 18 * 424 = 0x00001dd0
 *   LPC17_BUFFER_SIZE    = 0x00003ba0

 *   LPC17_BUFFER_BASE    = LPC17_SRAM_BANK0
 *   LPC17_TXBUFFER_BASE  = LPC17_SRAM_BANK0
 *   LPC17_RXBUFFER_BASE  = LPC17_SRAM_BANK0 + 0x00001dd0
 *   LPC17_BUFFER_END     = LPC17_SRAM_BANK0 + 0x00003ba0
 *
 *   Then the check LPC17_BUFFER_END < LPC17_PKTMEM_END passes. The
 *   amount of unused memory is small: 0x00003e08-0x00003ba0 or about
 *   616 bytes -- not enough for two more packets.
 *
 * [It is also possible, with some effort, to reclaim any unused
 *  SRAM for the use in the heap.  But that has not yet been pursued.]
 */

#ifndef CONFIG_NET_NTXDESC
#  define CONFIG_NET_NTXDESC 18
#endif
#define LPC17_TXDESCTAB_SIZE (CONFIG_NET_NTXDESC*LPC17_TXDESC_SIZE)
#define LPC17_TXSTATTAB_SIZE (CONFIG_NET_NTXDESC*LPC17_TXSTAT_SIZE)
#define LPC17_TXTAB_SIZE     (LPC17_TXDESCTAB_SIZE+LPC17_TXSTATTAB_SIZE)

#ifndef CONFIG_NET_NRXDESC
#  define CONFIG_NET_NRXDESC 18
#endif
#define LPC17_RXDESCTAB_SIZE (CONFIG_NET_NRXDESC*LPC17_RXDESC_SIZE)
#define LPC17_RXSTATTAB_SIZE (CONFIG_NET_NRXDESC*LPC17_RXSTAT_SIZE)
#define LPC17_RXTAB_SIZE     (LPC17_RXDESCTAB_SIZE+LPC17_RXSTATTAB_SIZE)

#define LPC17_DESCTAB_SIZE   (LPC17_TXTAB_SIZE+LPC17_RXTAB_SIZE)

/* All of AHB SRAM, Bank 0 is set aside for EMAC Tx and Rx descriptors. */

#define LPC17_BANK0_SIZE     0x00004000

#define LPC17_EMACRAM_BASE   LPC17_SRAM_BANK0
#define LPC17_EMACRAM_SIZE   LPC17_BANK0_SIZE

/* Descriptor table memory organization.  Descriptor tables are packed at
 * the end of AHB SRAM, Bank 0.  The beginning of bank 0 is reserved for
 * packet memory.
 */

#define LPC17_DESC_BASE      (LPC17_EMACRAM_BASE+LPC17_EMACRAM_SIZE-LPC17_DESCTAB_SIZE)
#define LPC17_TXDESC_BASE    LPC17_DESC_BASE
#define LPC17_TXSTAT_BASE    (LPC17_TXDESC_BASE+LPC17_TXDESCTAB_SIZE)
#define LPC17_RXDESC_BASE    (LPC17_TXSTAT_BASE+LPC17_TXSTATTAB_SIZE)
#define LPC17_RXSTAT_BASE    (LPC17_RXDESC_BASE + LPC17_RXDESCTAB_SIZE)

/* Now carve up the beginning of SRAM for packet memory.  The size of a
 * packet buffer is related to the size of the MTU.  We'll round sizes up
 * to multiples of 256 bytes.
 */

#define LPC17_PKTMEM_BASE     LPC17_EMACRAM_BASE
#define LPC17_PKTMEM_SIZE     (LPC17_EMACRAM_SIZE-LPC17_DESCTAB_SIZE)
#define LPC17_PKTMEM_END      (LPC17_EMACRAM_BASE+LPC17_PKTMEM_SIZE)

#define LPC17_MAXPACKET_SIZE  ((CONFIG_NET_BUFSIZE + 3 + 2) & ~3)
#define LPC17_NTXPKTS         CONFIG_NET_NTXDESC
#define LPC17_NRXPKTS         CONFIG_NET_NRXDESC

#define LPC17_TXBUFFER_SIZE   (LPC17_NTXPKTS * LPC17_MAXPACKET_SIZE)
#define LPC17_RXBUFFER_SIZE   (LPC17_NRXPKTS * LPC17_MAXPACKET_SIZE)
#define LPC17_BUFFER_SIZE     (LPC17_TXBUFFER_SIZE + LPC17_RXBUFFER_SIZE)

#define LPC17_BUFFER_BASE     LPC17_PKTMEM_BASE
#define LPC17_TXBUFFER_BASE   LPC17_BUFFER_BASE
#define LPC17_RXBUFFER_BASE   (LPC17_TXBUFFER_BASE + LPC17_TXBUFFER_SIZE)
#define LPC17_BUFFER_END      (LPC17_BUFFER_BASE + LPC17_BUFFER_SIZE)

#if LPC17_BUFFER_END > LPC17_PKTMEM_END
#  error "Packet memory overlaps descriptor tables"
#endif

/* Register debug -- can only happen of CONFIG_DEBUG is selected */

#ifndef CONFIG_DEBUG
#  undef  CONFIG_NET_REGDEBUG
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* EMAC statistics (debug only) */

#if defined(CONFIG_DEBUG) && defined(CONFIG_DEBUG_NET)
struct lpc17_statistics_s
{
#ifdef ENABLE_WOL
  uint32_t wol;            /* Wake-up interrupts */
#endif
  uint32_t rx_finished;    /* Rx finished interrupts */
  uint32_t rx_done;        /* Rx done interrupts */
  uint32_t rx_ovrerrors;   /* Number of Rx overrun error interrupts */
  uint32_t rx_errors;      /* Number of Rx error interrupts (OR of other errors) */
  uint32_t rx_packets;     /* Number of packets received (sum of the following): */
  uint32_t rx_ip;          /*   Number of Rx IP packets received */
  uint32_t rx_arp;         /*   Number of Rx ARP packets received */
  uint32_t rx_dropped;     /*   Number of dropped, unsupported Rx packets */
  uint32_t rx_pkterr;      /*   Number of dropped, error in Rx descriptor */
  uint32_t rx_pktsize;     /*   Number of dropped, too small or too big */
  uint32_t rx_fragment;    /*   Number of dropped, packet fragments */

  uint32_t tx_packets;     /* Number of Tx packets queued */
  uint32_t tx_pending;     /* Number of Tx packets that had to wait for a TxDesc */
  uint32_t tx_unpend;      /* Number of pending Tx packets that were sent */
  uint32_t tx_finished;    /* Tx finished interrupts */
  uint32_t tx_done;        /* Tx done interrupts */
  uint32_t tx_underrun;    /* Number of Tx underrun error interrupts */
  uint32_t tx_errors;      /* Number of Tx error inerrupts (OR of other errors) */
  uint32_t tx_timeouts;    /* Number of Tx timeout errors */
};
#  define EMAC_STAT(priv,name) priv->lp_stat.name++
#else
#  define EMAC_STAT(priv,name)
#endif

/* The lpc17_driver_s encapsulates all state information for a single hardware
 * interface
 */

struct lpc17_driver_s
{
  /* The following fields would only be necessary on chips that support
   * multiple Ethernet controllers.
   */

#if CONFIG_LPC17_NINTERFACES > 1
  uint32_t lp_base;             /* Ethernet controller base address */
  int      lp_irq;              /* Ethernet controller IRQ */
#endif

  bool     lp_ifup;             /* true:ifup false:ifdown */
  bool     lp_mode;             /* speed/duplex */
  bool     lp_txpending;        /* There is a pending Tx in lp_dev */
#ifdef LPC17_HAVE_PHY
  uint8_t  lp_phyaddr;          /* PHY device address */
#endif
  uint32_t lp_inten;            /* Shadow copy of INTEN register */
  WDOG_ID  lp_txpoll;           /* TX poll timer */
  WDOG_ID  lp_txtimeout;        /* TX timeout timer */
  
#if defined(CONFIG_DEBUG) && defined(CONFIG_DEBUG_NET)
  struct lpc17_statistics_s lp_stat;
#endif

  /* This holds the information visible to uIP/NuttX */

  struct uip_driver_s lp_dev;  /* Interface understood by uIP */
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* Array of ethernet driver status structures */

static struct lpc17_driver_s g_ethdrvr[CONFIG_LPC17_NINTERFACES];

/* ENET pins are on P1[0,1,4,6,8,9,10,14,15] + MDC on P1[16] or P2[8] and
 * MDIO on P1[17] or P2[9].  The board.h file will define GPIO_ENET_MDC and
 * PGIO_ENET_MDIO to selec which pin setting to use.
 *
 * On older Rev '-' devices, P1[6] ENET-TX_CLK would also have be to configured.
 */

static const uint16_t g_enetpins[GPIO_NENET_PINS] =
{
  GPIO_ENET_TXD0, GPIO_ENET_TXD1, GPIO_ENET_TXEN,   GPIO_ENET_CRS, GPIO_ENET_RXD0,
  GPIO_ENET_RXD1, GPIO_ENET_RXER, GPIO_ENET_REFCLK, GPIO_ENET_MDC, GPIO_ENET_MDIO
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/* Register operations */

#ifdef CONFIG_NET_REGDEBUG
static void lpc17_printreg(uint32_t addr, uint32_t val, bool iswrite);
static void lpc17_checkreg(uint32_t addr, uint32_t val, bool iswrite);
static uint32_t lpc17_getreg(uint32_t addr);
static void lpc17_putreg(uint32_t val, uint32_t addr);
#else
# define lpc17_getreg(addr)     getreg32(addr)
# define lpc17_putreg(val,addr) putreg32(val,addr)
#endif

/* Common TX logic */

static int  lpc17_txdesc(struct lpc17_driver_s *priv);
static int  lpc17_transmit(struct lpc17_driver_s *priv);
static int  lpc17_uiptxpoll(struct uip_driver_s *dev);

/* Interrupt handling */

static void lpc17_response(struct lpc17_driver_s *priv);
static void lpc17_rxdone(struct lpc17_driver_s *priv);
static void lpc17_txdone(struct lpc17_driver_s *priv);
static int  lpc17_interrupt(int irq, void *context);

/* Watchdog timer expirations */

static void lpc17_polltimer(int argc, uint32_t arg, ...);
static void lpc17_txtimeout(int argc, uint32_t arg, ...);

/* NuttX callback functions */

static int lpc17_ifup(struct uip_driver_s *dev);
static int lpc17_ifdown(struct uip_driver_s *dev);
static int lpc17_txavail(struct uip_driver_s *dev);
#ifdef CONFIG_NET_IGMP
static int lpc17_addmac(struct uip_driver_s *dev, const uint8_t *mac);
static int lpc17_rmmac(struct uip_driver_s *dev, const uint8_t *mac);
#endif

/* Initialization functions */

#ifdef CONFIG_NET_REGDEBUG
static void lpc17_showpins(void);
#else
#  define lpc17_showpins()
#endif

/* PHY initialization functions */

#ifdef LPC17_HAVE_PHY
#  ifdef CONFIG_NET_REGDEBUG
static void lpc17_showmii(uint8_t phyaddr, const char *msg);
#  else
#    define lpc17_showmii(phyaddr,msg)
#  endif

static void lpc17_phywrite(uint8_t phyaddr, uint8_t regaddr,
                           uint16_t phydata);
static uint16_t lpc17_phyread(uint8_t phyaddr, uint8_t regaddr);
static inline int lpc17_phyreset(uint8_t phyaddr);
#  ifdef CONFIG_PHY_AUTONEG
static inline int lpc17_phyautoneg(uint8_t phyaddr);
#  endif
static int lpc17_phymode(uint8_t phyaddr, uint8_t mode);
static inline int lpc17_phyinit(struct lpc17_driver_s *priv);
#else
#  define lpc17_phyinit(priv)
#endif

/* EMAC Initialization functions */

static inline void lpc17_txdescinit(struct lpc17_driver_s *priv);
static inline void lpc17_rxdescinit(struct lpc17_driver_s *priv);
static void lpc17_macmode(uint8_t mode);
static void lpc17_ethreset(struct lpc17_driver_s *priv);

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/*******************************************************************************
 * Name: lpc17_printreg
 *
 * Description:
 *   Print the contents of an LPC17xx register operation
 *
 *******************************************************************************/

#ifdef CONFIG_NET_REGDEBUG
static void lpc17_printreg(uint32_t addr, uint32_t val, bool iswrite)
{
  dbg("%08x%s%08x\n", addr, iswrite ? "<-" : "->", val);
}
#endif

/*******************************************************************************
 * Name: lpc17_checkreg
 *
 * Description:
 *   Get the contents of an LPC17xx register
 *
 *******************************************************************************/

#ifdef CONFIG_NET_REGDEBUG
static void lpc17_checkreg(uint32_t addr, uint32_t val, bool iswrite)
{
  static uint32_t prevaddr = 0;
  static uint32_t preval = 0;
  static uint32_t count = 0;
  static bool     prevwrite = false;

  /* Is this the same value that we read from/wrote to the same register last time?
   * Are we polling the register?  If so, suppress the output.
   */

  if (addr == prevaddr && val == preval && prevwrite == iswrite)
    {
      /* Yes.. Just increment the count */

      count++;
    }
  else
    {
      /* No this is a new address or value or operation. Were there any
       * duplicate accesses before this one?
       */

      if (count > 0)
        {
          /* Yes.. Just one? */

          if (count == 1)
            {
              /* Yes.. Just one */

              lpc17_printreg(prevaddr, preval, prevwrite);
            }
          else
            {
              /* No.. More than one. */

              dbg("[repeats %d more times]\n", count);
            }
        }

      /* Save the new address, value, count, and operation for next time */

      prevaddr  = addr;
      preval    = val;
      count     = 0;
      prevwrite = iswrite;

      /* Show the new regisgter access */

      lpc17_printreg(addr, val, iswrite);
    }
}
#endif

/*******************************************************************************
 * Name: lpc17_getreg
 *
 * Description:
 *   Get the contents of an LPC17xx register
 *
 *******************************************************************************/

#ifdef CONFIG_NET_REGDEBUG
static uint32_t lpc17_getreg(uint32_t addr)
{
  /* Read the value from the register */

  uint32_t val = getreg32(addr);

  /* Check if we need to print this value */

  lpc17_checkreg(addr, val, false);
  return val;
}
#endif

/*******************************************************************************
 * Name: lpc17_putreg
 *
 * Description:
 *   Set the contents of an LPC17xx register to a value
 *
 *******************************************************************************/

#ifdef CONFIG_NET_REGDEBUG
static void lpc17_putreg(uint32_t val, uint32_t addr)
{
  /* Check if we need to print this value */

  lpc17_checkreg(addr, val, true);

  /* Write the value */

  putreg32(val, addr);
}
#endif

/****************************************************************************
 * Function: lpc17_txdesc
 *
 * Description:
 *   Check if a free TX descriptor is available.
 *
 * Parameters:
 *   priv  - Reference to the driver state structure
 *
 * Returned Value:
 *   OK on success; a negated errno on failure
 *
 * Assumptions:
 *   May or may not be called from an interrupt handler.  In either case,
 *   global interrupts are disabled, either explicitly or indirectly through
 *   interrupt handling logic.
 *
 ****************************************************************************/

static int lpc17_txdesc(struct lpc17_driver_s *priv)
{
  unsigned int prodidx;
  unsigned int considx;

  /* Get the next producer index */

  prodidx = lpc17_getreg(LPC17_ETH_TXPRODIDX) & ETH_TXPRODIDX_MASK;
  if (++prodidx >= CONFIG_NET_NTXDESC)
    {
     /* Wrap back to index zero */

      prodidx = 0;
    }

  /* If the next producer index would overrun the consumer index, then there
   * are not available descriptors.
   */

  considx = lpc17_getreg(LPC17_ETH_TXCONSIDX) & ETH_TXCONSIDX_MASK;
  return prodidx != considx ? OK : -EAGAIN;
}

/****************************************************************************
 * Function: lpc17_transmit
 *
 * Description:
 *   Start hardware transmission.  Called either from the txdone interrupt
 *   handling or from watchdog based polling.
 *
 * Parameters:
 *   priv  - Reference to the driver state structure
 *
 * Returned Value:
 *   OK on success; a negated errno on failure
 *
 * Assumptions:
 *   May or may not be called from an interrupt handler.  In either case,
 *   global interrupts are disabled, either explicitly or indirectly through
 *   interrupt handling logic.
 *
 ****************************************************************************/

static int lpc17_transmit(struct lpc17_driver_s *priv)
{
  uint32_t *txdesc;
  void     *txbuffer;
  unsigned int prodidx;

  /* Verify that the hardware is ready to send another packet.  If we get
   * here, then we are committed to sending a packet; Higher level logic
   * must have assured that there is not transmission in progress.
   */

  DEBUGASSERT(lpc17_txdesc(priv) == OK);

  /* Increment statistics */

  EMAC_STAT(priv, tx_packets);

  /* Get the current producer index */

  prodidx = lpc17_getreg(LPC17_ETH_TXPRODIDX) & ETH_TXPRODIDX_MASK;

  /* Get the packet address from the descriptor and set the descriptor control
   * fields.
   */

  txdesc   = (uint32_t*)(LPC17_TXDESC_BASE + (prodidx << 3));
  txbuffer = (void*)*txdesc++;
  *txdesc  = TXDESC_CONTROL_INT | TXDESC_CONTROL_LAST | TXDESC_CONTROL_CRC |
             (priv->lp_dev.d_len - 1);

  /* Copy the packet data into the Tx buffer assignd to this.  It should fit;
   * because each packet buffer is MTU size and breaking up larger TCP messages
   * is handled by higher level logic.  The hardware does, however, support
   * breaking up larger messages into many fragments, however, that capability
   * is not exploited here.
   *
   * This would be a great performance improvement:  Remove the buffer from
   * the lp_dev structure and replace it a pointer directly into the EMAC
   * DMA memory.  This could eliminate the following, costly memcpy.
   */

  DEBUGASSERT(priv->lp_dev.d_len <= LPC17_MAXPACKET_SIZE);
  memcpy(txbuffer, priv->lp_dev.d_buf, priv->lp_dev.d_len);

  /* Bump the producer index, making the packet available for transmission. */
  
  if (++prodidx >= CONFIG_NET_NTXDESC)
    {
     /* Wrap back to index zero */

      prodidx = 0;
    }
  lpc17_putreg(prodidx, LPC17_ETH_TXPRODIDX);

  /* Enable Tx interrupts */

  priv->lp_inten |= ETH_TXINTS;
  lpc17_putreg(priv->lp_inten, LPC17_ETH_INTEN);

  /* Setup the TX timeout watchdog (perhaps restarting the timer) */

  (void)wd_start(priv->lp_txtimeout, LPC17_TXTIMEOUT, lpc17_txtimeout, 1, (uint32_t)priv);
  return OK;
}

/****************************************************************************
 * Function: lpc17_uiptxpoll
 *
 * Description:
 *   The transmitter is available, check if uIP has any outgoing packets ready
 *   to send.  This is a callback from uip_poll().  uip_poll() may be called:
 *
 *   1. When the preceding TX packet send is complete,
 *   2. When the preceding TX packet send timesout and the interface is reset
 *   3. During normal TX polling
 *
 * Parameters:
 *   dev  - Reference to the NuttX driver state structure
 *
 * Returned Value:
 *   OK on success; a negated errno on failure
 *
 * Assumptions:
 *   May or may not be called from an interrupt handler.  In either case,
 *   global interrupts are disabled, either explicitly or indirectly through
 *   interrupt handling logic.
 *
 ****************************************************************************/

static int lpc17_uiptxpoll(struct uip_driver_s *dev)
{
  struct lpc17_driver_s *priv = (struct lpc17_driver_s *)dev->d_private;
  int ret = OK;

  /* If the polling resulted in data that should be sent out on the network,
   * the field d_len is set to a value > 0.
   */

  if (priv->lp_dev.d_len > 0)
    {
      /* Send this packet.  In this context, we know that there is space for
       * at least one more packet in the descriptor list.
       */

      uip_arp_out(&priv->lp_dev);
      lpc17_transmit(priv);

      /* Check if there is room in the device to hold another packet. If not,
       * return any non-zero value to terminate the poll.
       */

      ret = lpc17_txdesc(priv);
    }

  /* If zero is returned, the polling will continue until all connections have
   * been examined.
   */

  return ret;
}

/****************************************************************************
 * Function: lpc17_response
 *
 * Description:
 *   While processing an RxDone event, higher logic decides to send a packet,
 *   possibly a response to the incoming packet (but probably not, in reality).
 *   However, since the Rx and Tx operations are decoupled, there is no
 *   guarantee that there will be a Tx descriptor available at that time.
 *   This function will perform that check and, if no Tx descriptor is 
 *   available, this function will (1) stop incoming Rx processing (bad), and
 *   (2) hold the outgoing packet in a pending state until the next Tx
 *   interrupt occurs. 
 *
 * Parameters:
 *   priv  - Reference to the driver state structure
 *
 * Returned Value:
 *   None
 *
 * Assumptions:
 *   Global interrupts are disabled by interrupt handling logic.
 *
 ****************************************************************************/

static void lpc17_response(struct lpc17_driver_s *priv)
{
  int ret;

  /* Check if there is room in the device to hold another packet. */

  ret = lpc17_txdesc(priv);
  if (ret == OK)
    {
       /* Yes.. queue the packet now. */

       lpc17_transmit(priv);
    }
  else
    {
       /* No.. mark the Tx as pending and halt further Tx interrupts */

       DEBUGASSERT((priv->lp_inten & ETH_INT_TXDONE) != 0);
       
       priv->lp_txpending = true;
       priv->lp_inten    &= ~ETH_RXINTS;
       lpc17_putreg(priv->lp_inten, LPC17_ETH_INTEN);
       EMAC_STAT(priv, tx_pending);
    }
}

/****************************************************************************
 * Function: lpc17_rxdone
 *
 * Description:
 *   An interrupt was received indicating the availability of a new RX packet
 *
 * Parameters:
 *   priv  - Reference to the driver state structure
 *
 * Returned Value:
 *   None
 *
 * Assumptions:
 *   Global interrupts are disabled by interrupt handling logic.
 *
 ****************************************************************************/

static void lpc17_rxdone(struct lpc17_driver_s *priv)
{
  uint32_t    *rxstat;
  bool         fragment;
  unsigned int prodidx;
  unsigned int considx;
  unsigned int pktlen;

  /* Get the current producer and consumer indices */

  considx = lpc17_getreg(LPC17_ETH_RXCONSIDX) & ETH_RXCONSIDX_MASK;
  prodidx = lpc17_getreg(LPC17_ETH_RXPRODIDX) & ETH_RXPRODIDX_MASK;

  /* Loop while there are incoming packets to be processed */

  /* If the next producer index would overrun the consumer index, then there
   * are not available descriptors.
   */

  fragment = false;
  while (considx != prodidx)
    {
      /* Update statistics */

      EMAC_STAT(priv, rx_packets);

      /* Get the Rx status and packet length */
    
      rxstat   = (uint32_t*)(LPC17_RXSTAT_BASE + (considx << 3));
      pktlen   = (*rxstat & RXSTAT_INFO_RXSIZE_MASK) + 1;

      /* Check for errors */

      if ((*rxstat & RXSTAT_INFO_ERROR) != 0)
        {
          nlldbg("Error.  rxstat: %08d\n", *rxstat);
          EMAC_STAT(priv, rx_pkterr);
        }

      /* If the pktlen is greater then the buffer, then we cannot accept
       * the packet.  Also, since the DMA packet buffers are set up to
       * be the same size as our max packet size, any fragments also
       * imply that the packet is too big.
       */
 
      else if (pktlen > CONFIG_NET_BUFSIZE+2)
        {
          nlldbg("Too big.  pktlen: %d rxstat: %08x\n", pktlen, *rxstat);
          EMAC_STAT(priv, rx_pktsize);
        }
      else if ((*rxstat & RXSTAT_INFO_LASTFLAG) != 0)
        {
          nlldbg("Fragment.  rxstat: %08x\n", pktlen, *rxstat);
          EMAC_STAT(priv, rx_fragment);
          fragment = true;
        }
      else if (fragment)
        {
          nlldbg("Last fragment.  rxstat: %08x\n", pktlen, *rxstat);
          EMAC_STAT(priv, rx_fragment);
          fragment = false;
        }
      else
        {
          uint32_t *rxdesc;
          void     *rxbuffer;

          /* Get the Rx buffer address from the Rx descriptor */
 
          rxdesc   = (uint32_t*)(LPC17_RXDESC_BASE + (considx << 3));
          rxbuffer = (void*)*rxdesc;

          /* Copy the data data from the EMAC DMA RAM to priv->lp_dev.d_buf. 
           * Set amount of data in priv->lp_dev.d_len
           *
           * This would be a great performance improvement:  Remove the
           * buffer from the lp_dev structure and replaceit a pointer
           * directly into the EMAC DMA memory.  This could eliminate the
           * following, costly memcpy.
           */

          memcpy(priv->lp_dev.d_buf, rxbuffer, pktlen);
          priv->lp_dev.d_len = pktlen;

          /* We only accept IP packets of the configured type and ARP packets */

#ifdef CONFIG_NET_IPv6
          if (BUF->type == HTONS(UIP_ETHTYPE_IP6))
#else
          if (BUF->type == HTONS(UIP_ETHTYPE_IP))
#endif
            {
              /* Handle the incoming Rx packet */

              EMAC_STAT(priv, rx_ip);
              uip_arp_ipin();
              uip_input(&priv->lp_dev);

              /* If the above function invocation resulted in data that
               * should be sent out on the network, the field  d_len will
               * set to a value > 0.
               */

              if (priv->lp_dev.d_len > 0)
                {
                  uip_arp_out(&priv->lp_dev);
                  lpc17_response(priv);
                }
            }
          else if (BUF->type == htons(UIP_ETHTYPE_ARP))
            {
              EMAC_STAT(priv, rx_arp);
              uip_arp_arpin(&priv->lp_dev);

              /* If the above function invocation resulted in data that
               * should be sent out on the network, the field  d_len will
               * set to a value > 0.
               */

              if (priv->lp_dev.d_len > 0)
                {
                  lpc17_response(priv);
                }
            }
          else
            {
              /* Unrecognized... drop it. */

              EMAC_STAT(priv, rx_dropped);
            }
        }

      /* Bump up the consumer index and resample the producer index (which
       * might also have gotten bumped up by the hardware).
       */

      if (++considx >= CONFIG_NET_NRXDESC)
       {
         /* Wrap back to index zero */

          considx = 0;
        }

      lpc17_putreg(considx, LPC17_ETH_RXCONSIDX);
      prodidx = lpc17_getreg(LPC17_ETH_RXPRODIDX) & ETH_RXPRODIDX_MASK;
    }
}

/****************************************************************************
 * Function: lpc17_txdone
 *
 * Description:
 *   An interrupt was received indicating that the last TX packet(s) is done
 *
 * Parameters:
 *   priv  - Reference to the driver state structure
 *
 * Returned Value:
 *   None
 *
 * Assumptions:
 *   Global interrupts are disabled by interrupt handling logic.
 *
 ****************************************************************************/

static void lpc17_txdone(struct lpc17_driver_s *priv)
{
  /* Cancel the pending Tx timeout */

  wd_cancel(priv->lp_txtimeout);

  /* Disable further Tx interrupts.  Tx interrupts may be re-enabled again
   * depending upon the result of the poll.
   */

  priv->lp_inten &= ~ETH_TXINTS;
  lpc17_putreg(priv->lp_inten, LPC17_ETH_INTEN);

  /* Verify that the hardware is ready to send another packet.  Since a Tx
   * just completed, this must be the case.
   */

  DEBUGASSERT(lpc17_txdesc(priv) == OK);

  /* Check if there is a pending Tx transfer that was scheduled by Rx handling
   * while the Tx logic was busy.  If so, processing that pending Tx now.
   */

  if (priv->lp_txpending)
    {
      /* Clear the pending condition, send the packet, and restore Rx interrupts */

      priv->lp_txpending = false;
      EMAC_STAT(priv, tx_unpend);

      lpc17_transmit(priv);

      priv->lp_inten    |= ETH_RXINTS;
      lpc17_putreg(priv->lp_inten, LPC17_ETH_INTEN);
    }

  /* Otherwise poll uIP for new XMIT data */

  else
    {
      (void)uip_poll(&priv->lp_dev, lpc17_uiptxpoll);
    }
}

/****************************************************************************
 * Function: lpc17_interrupt
 *
 * Description:
 *   Hardware interrupt handler
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

static int lpc17_interrupt(int irq, void *context)
{
  register struct lpc17_driver_s *priv = &g_ethdrvr[0];
  uint32_t status;

  /* Get the interrupt status (zero means no interrupts pending). */

  status = lpc17_getreg(LPC17_ETH_INTST);
  if (status != 0)
    {
      /* Handle each pending interrupt */
      /* Check for receive errors */

      if ((status & ETH_INT_RXOVR) != 0)
        {
          lpc17_putreg(ETH_INT_RXOVR, LPC17_ETH_INTCLR);
          EMAC_STAT(priv, rx_ovrerrors);
        }

      if ((status & ETH_INT_RXERR) != 0)
        {
          lpc17_putreg(ETH_INT_RXERR, LPC17_ETH_INTCLR);
          EMAC_STAT(priv, rx_errors);
        }

      /* Check if we received an incoming packet, if so, call lpc17_rxdone() */

      if ((status & ETH_INT_RXFIN) != 0)
        {
          lpc17_putreg(ETH_INT_RXFIN, LPC17_ETH_INTCLR);
          EMAC_STAT(priv, rx_finished);
          DEBUGASSERT(lpc17_getreg(LPC17_ETH_RXPRODIDX) == lpc17_getreg(LPC17_ETH_RXCONSIDX));
        }

      if ((status & ETH_INT_RXDONE) != 0)
        {
          lpc17_putreg(ETH_INT_RXDONE, LPC17_ETH_INTCLR);
          EMAC_STAT(priv, rx_done);
          lpc17_rxdone(priv);
        }

      /* Check for Tx errors */

      if ((status & ETH_INT_TXUNR) != 0)
        {
          lpc17_putreg(ETH_INT_TXUNR, LPC17_ETH_INTCLR);
          EMAC_STAT(priv, tx_underrun);
        }

      if ((status & ETH_INT_TXERR) != 0)
        {
          lpc17_putreg(ETH_INT_TXERR, LPC17_ETH_INTCLR);
          EMAC_STAT(priv, tx_errors);
        }

      /* Check is a packet transmission just completed.  If so, call lpc17_txdone */

      if ((status & ETH_INT_TXFIN) != 0)
        {
          lpc17_putreg(ETH_INT_TXFIN, LPC17_ETH_INTCLR);
          EMAC_STAT(priv, tx_finished);
        }

      if ((status & ETH_INT_TXDONE) != 0)
        {
          lpc17_putreg(ETH_INT_TXDONE, LPC17_ETH_INTCLR);
          EMAC_STAT(priv, tx_done);
          lpc17_txdone(priv);
        }

      /* Check for Wake-Up on Lan */

#ifdef CONFIG_NET_WOL
      if ((status & ETH_INT_WKUP) != 0)
        {
          lpc17_putreg(ETH_INT_WKUP, LPC17_ETH_INTCLR);
          EMAC_STAT(priv, wol);
#         warning "Missing logic"
        }
#endif

    }

  return OK;
}

/****************************************************************************
 * Function: lpc17_txtimeout
 *
 * Description:
 *   Our TX watchdog timed out.  Called from the timer interrupt handler.
 *   The last TX never completed.  Reset the hardware and start again.
 *
 * Parameters:
 *   argc - The number of available arguments
 *   arg  - The first argument
 *
 * Returned Value:
 *   None
 *
 * Assumptions:
 *   Global interrupts are disabled by the watchdog logic.
 *
 ****************************************************************************/

static void lpc17_txtimeout(int argc, uint32_t arg, ...)
{
  struct lpc17_driver_s *priv = (struct lpc17_driver_s *)arg;

  /* Increment statistics and dump debug info */

  EMAC_STAT(priv, tx_timeouts);
  
  /* Then reset the hardware */

  /* Then poll uIP for new XMIT data */

  (void)uip_poll(&priv->lp_dev, lpc17_uiptxpoll);
}

/****************************************************************************
 * Function: lpc17_polltimer
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
 *   Global interrupts are disabled by the watchdog logic.
 *
 ****************************************************************************/

static void lpc17_polltimer(int argc, uint32_t arg, ...)
{
  struct lpc17_driver_s *priv = (struct lpc17_driver_s *)arg;

  /* Check if there is room in the send another TX packet.  We cannot perform
   * the TX poll if he are unable to accept another packet for transmission.
   */

  /* If so, update TCP timing states and poll uIP for new XMIT data. Hmmm..
   * might be bug here.  Does this mean if there is a transmit in progress,
   * we will missing TCP time state updates?
   */

  (void)uip_timer(&priv->lp_dev, lpc17_uiptxpoll, LPC17_POLLHSEC);

  /* Setup the watchdog poll timer again */

  (void)wd_start(priv->lp_txpoll, LPC17_WDDELAY, lpc17_polltimer, 1, arg);
}

/****************************************************************************
 * Function: lpc17_ifup
 *
 * Description:
 *   NuttX Callback: Bring up the Ethernet interface when an IP address is
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

static int lpc17_ifup(struct uip_driver_s *dev)
{
  struct lpc17_driver_s *priv = (struct lpc17_driver_s *)dev->d_private;
  uint32_t regval;
  int ret;

  ndbg("Bringing up: %d.%d.%d.%d\n",
       dev->d_ipaddr & 0xff, (dev->d_ipaddr >> 8) & 0xff,
       (dev->d_ipaddr >> 16) & 0xff, dev->d_ipaddr >> 24);

  /* Reset the Ethernet controller (again) */

  lpc17_ethreset(priv);

  /* Initialize the PHY and wait for the link to be established */

  ret = lpc17_phyinit(priv);
  if (ret != 0)
    {
      ndbg("lpc17_phyinit failed: %d\n", ret);
      return ret;
    }

  /* Configure the MAC station address */

  regval = (uint32_t)priv->lp_dev.d_mac.ether_addr_octet[1] << 8 |
           (uint32_t)priv->lp_dev.d_mac.ether_addr_octet[0];
  lpc17_putreg(regval, LPC17_ETH_SA0);

  regval = (uint32_t)priv->lp_dev.d_mac.ether_addr_octet[3] << 8 |
           (uint32_t)priv->lp_dev.d_mac.ether_addr_octet[2];
  lpc17_putreg(regval, LPC17_ETH_SA1);

  regval = (uint32_t)priv->lp_dev.d_mac.ether_addr_octet[5] << 8 |
           (uint32_t)priv->lp_dev.d_mac.ether_addr_octet[4];
  lpc17_putreg(regval, LPC17_ETH_SA2);

  /* Initialize Ethernet interface for the PHY setup */

  lpc17_macmode(priv->lp_mode);

  /* Initialize EMAC DMA memory -- descriptors, status, packet buffers, etc. */

  lpc17_txdescinit(priv);
  lpc17_rxdescinit(priv);

  /* Configure to pass all received frames */

  regval = lpc17_getreg(LPC17_ETH_MAC1);
  regval |= ETH_MAC1_PARF;
  lpc17_putreg(regval, LPC17_ETH_MAC1);

  /* Set up RX filter and configure to accept broadcast addresses, multicast
   * addresses, and perfect station address matches.
   */

  regval = ETH_RXFLCTRL_PERFEN;
#ifdef CONFIG_NET_BROADCAST
  regval |= ETH_RXFLCTRL_BCASTEN;
#endif
#ifdef CONFIG_NET_MULTICAST
  RXFILTERCTRL |= (ETH_RXFLCTRL_MCASTEN | ETH_RXFLCTRL_UCASTEN);
#endif
#ifdef CONFIG_NET_HASH
  RXFILTERCTRL |= (ETH_RXFLCTRL_MCASTHASHEN | ETH_RXFLCTRL_UCASTHASHEN);
#endif
  lpc17_putreg(regval, LPC17_ETH_RXFLCTRL);

  /* Clear any pending interrupts (shouldn't be any) */

  lpc17_putreg(0xffffffff, LPC17_ETH_INTCLR);

  /* Configure interrupts.  The Ethernet interrupt was attached during one-time
   * initialization, so we only need to set the interrupt priority, configure
   * interrupts, and enable them.
   */

  /* Set the interrupt to the highest priority */

#ifdef CONFIG_ARCH_IRQPRIO
#if CONFIG_LPC17_NINTERFACES > 1
  (void)up_prioritize_irq(priv->irq, CONFIG_NET_PRIORITY);
#else
  (void)up_prioritize_irq(LPC17_IRQ_ETH, CONFIG_NET_PRIORITY);
#endif
#endif

  /* Enable Ethernet interrupts.  The way we do this depends on whether or
   * not Wakeup on Lan (WoL) has been configured.
   */

#ifdef CONFIG_NET_WOL
  /* Configure WoL: Clear all receive filter WoLs and enable the perfect
   * match WoL interrupt.  We will wait until the Wake-up to finish
   * bringing things up.
   */

  lpc17_putreg(0xffffffff, LPC17_ETH_RXFLWOLCLR);
  lpc17_putreg(ETH_RXFLCTRL_RXFILEN, LPC17_ETH_RXRLCTRL);

  priv->lp_inten = ETH_INT_WKUP;
  lpc17_putreg(ETH_INT_WKUP, LPC17_ETH_INTEN);
#else
  /* Otherwise, enable all Rx interrupts.  Tx interrupts, SOFTINT and WoL are
   * excluded.  Tx interrupts will not be enabled until there is data to be
   * sent.
   */

  priv->lp_inten = ETH_RXINTS;
  lpc17_putreg(ETH_RXINTS, LPC17_ETH_INTEN);
#endif

  /* Set and activate a timer process */

  (void)wd_start(priv->lp_txpoll, LPC17_WDDELAY, lpc17_polltimer, 1, (uint32_t)priv);

  /* Finally, enable the Ethernet interrupt at the interrupt controller */

  priv->lp_ifup = true;
#if CONFIG_LPC17_NINTERFACES > 1
  up_enable_irq(priv->irq);
#else
  up_enable_irq(LPC17_IRQ_ETH);
#endif
  return OK;
}

/****************************************************************************
 * Function: lpc17_ifdown
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

static int lpc17_ifdown(struct uip_driver_s *dev)
{
  struct lpc17_driver_s *priv = (struct lpc17_driver_s *)dev->d_private;
  irqstate_t flags;

  /* Disable the Ethernet interrupt */

  flags = irqsave();
  up_disable_irq(LPC17_IRQ_ETH);

  /* Cancel the TX poll timer and TX timeout timers */

  wd_cancel(priv->lp_txpoll);
  wd_cancel(priv->lp_txtimeout);

  /* Reset the device and mark it as down. */

  lpc17_ethreset(priv);
  priv->lp_ifup = false;
  irqrestore(flags);
  return OK;
}

/****************************************************************************
 * Function: lpc17_txavail
 *
 * Description:
 *   Driver callback invoked when new TX data is available.  This is a 
 *   stimulus perform an out-of-cycle poll and, thereby, reduce the TX
 *   latency.
 *
 * Parameters:
 *   dev  - Reference to the NuttX driver state structure
 *
 * Returned Value:
 *   None
 *
 * Assumptions:
 *   Called in normal user mode
 *
 ****************************************************************************/

static int lpc17_txavail(struct uip_driver_s *dev)
{
  struct lpc17_driver_s *priv = (struct lpc17_driver_s *)dev->d_private;
  irqstate_t flags;

  /* Disable interrupts because this function may be called from interrupt
   * level processing.
   */

  flags = irqsave();

  /* Ignore the notification if the interface is not yet up */

  if (priv->lp_ifup)
    {
      /* Check if there is room in the hardware to hold another outgoing packet. */

      if (lpc17_txdesc(priv) == OK)
        {
          /* If so, then poll uIP for new XMIT data */

          (void)uip_poll(&priv->lp_dev, lpc17_uiptxpoll);
        }
    }

  irqrestore(flags);
  return OK;
}

/****************************************************************************
 * Function: lpc17_addmac
 *
 * Description:
 *   NuttX Callback: Add the specified MAC address to the hardware multicast
 *   address filtering
 *
 * Parameters:
 *   dev  - Reference to the NuttX driver state structure
 *   mac  - The MAC address to be added 
 *
 * Returned Value:
 *   None
 *
 * Assumptions:
 *
 ****************************************************************************/

#ifdef CONFIG_NET_IGMP
static int lpc17_addmac(struct uip_driver_s *dev, const uint8_t *mac)
{
  struct lpc17_driver_s *priv = (struct lpc17_driver_s *)dev->d_private;

  /* Add the MAC address to the hardware multicast routing table */

#warning "Not implemented"
  return OK;
}
#endif

/****************************************************************************
 * Function: lpc17_rmmac
 *
 * Description:
 *   NuttX Callback: Remove the specified MAC address from the hardware multicast
 *   address filtering
 *
 * Parameters:
 *   dev  - Reference to the NuttX driver state structure
 *   mac  - The MAC address to be removed 
 *
 * Returned Value:
 *   None
 *
 * Assumptions:
 *
 ****************************************************************************/

#ifdef CONFIG_NET_IGMP
static int lpc17_rmmac(struct uip_driver_s *dev, const uint8_t *mac)
{
  struct lpc17_driver_s *priv = (struct lpc17_driver_s *)dev->d_private;

  /* Add the MAC address to the hardware multicast routing table */

#warning "Not implemented"
  return OK;
}
#endif

/*******************************************************************************
 * Name: lpc17_showpins
 *
 * Description:
 *   Dump GPIO registers
 *
 * Parameters:
 *   None 
 *
 * Returned Value:
 *   None
 *
 * Assumptions:
 *
 *******************************************************************************/

#ifdef CONFIG_NET_REGDEBUG
static void lpc17_showpins(void)
{
  lpc17_dumpgpio(GPIO_PORT0|GPIO_PIN0, "P0[1-15]");
  lpc17_dumpgpio(GPIO_PORT0|GPIO_PIN16, "P0[16-31]");
}
#endif

/*******************************************************************************
 * Name: lpc17_showmii
 *
 * Description:
 *   Dump PHY MII registers
 *
 * Parameters:
 *   phyaddr - The device address where the PHY was discovered
 *
 * Returned Value:
 *   None
 *
 * Assumptions:
 *
 *******************************************************************************/

#if defined(CONFIG_NET_REGDEBUG) && defined(LPC17_HAVE_PHY)
static void lpc17_showmii(uint8_t phyaddr, const char *msg)
{
  dbg("PHY " LPC17_PHYNAME ": %s\n", msg);
  dbg("  MCR:       %04x\n", lpc17_getreg(MII_MCR));
  dbg("  MSR:       %04x\n", lpc17_getreg(MII_MSR));
  dbg("  ADVERTISE: %04x\n", lpc17_getreg(MII_ADVERTISE));
  dbg("  LPA:       %04x\n", lpc17_getreg(MII_LPA));
  dbg("  EXPANSION: %04x\n", lpc17_getreg(MII_EXPANSION));
#ifdef CONFIG_PHY_KS8721
  dbg("  10BTCR:    %04x\n", lpc17_getreg(MII_KS8721_10BTCR));
#endif
}
#endif

/****************************************************************************
 * Function: lpc17_phywrite
 *
 * Description:
 *   Write a value to an MII PHY register
 *
 * Parameters:
 *   phyaddr - The device address where the PHY was discovered
 *   regaddr - The address of the PHY register to be written
 *   phydata - The data to write to the PHY register 
 *
 * Returned Value:
 *   None
 *
 * Assumptions:
 *
 ****************************************************************************/

#ifdef LPC17_HAVE_PHY
static void lpc17_phywrite(uint8_t phyaddr, uint8_t regaddr, uint16_t phydata)
{
  uint32_t regval;

  /* Set PHY address and PHY register address */

  regval = ((uint32_t)phyaddr << ETH_MADR_PHYADDR_SHIFT) |
           ((uint32_t)regaddr << ETH_MADR_REGADDR_SHIFT);
  lpc17_putreg(regval, LPC17_ETH_MADR);

  /* Set up to write */

  lpc17_putreg(ETH_MCMD_WRITE, LPC17_ETH_MCMD);

  /* Write the register data to the PHY */

  lpc17_putreg((uint32_t)phydata, LPC17_ETH_MWTD);

  /* Wait for the PHY command to complete */

  while ((lpc17_getreg(LPC17_ETH_MIND) & ETH_MIND_BUSY) != 0);
}
#endif

/****************************************************************************
 * Function: lpc17_phyread
 *
 * Description:
 *   Read a value from an MII PHY register
 *
 * Parameters:
 *   phyaddr - The device address where the PHY was discovered
 *   regaddr - The address of the PHY register to be written
 *
 * Returned Value:
 *   Data read from the PHY register
 *
 * Assumptions:
 *
 ****************************************************************************/

#ifdef LPC17_HAVE_PHY
static uint16_t lpc17_phyread(uint8_t phyaddr, uint8_t regaddr)
{
  uint32_t regval;

  lpc17_putreg(0, LPC17_ETH_MCMD);

  /* Set PHY address and PHY register address */

  regval = ((uint32_t)phyaddr << ETH_MADR_PHYADDR_SHIFT) |
           ((uint32_t)regaddr << ETH_MADR_REGADDR_SHIFT);
  lpc17_putreg(regval, LPC17_ETH_MADR);

  /* Set up to read */

  lpc17_putreg(ETH_MCMD_READ, LPC17_ETH_MCMD);

  /* Wait for the PHY command to complete */

  while ((lpc17_getreg(LPC17_ETH_MIND) & (ETH_MIND_BUSY|ETH_MIND_NVALID)) != 0);
  lpc17_putreg(0, LPC17_ETH_MCMD);

  /* Return the PHY register data */

  return (uint16_t)(lpc17_getreg(LPC17_ETH_MRDD) & ETH_MRDD_MASK);
}
#endif

/****************************************************************************
 * Function: lpc17_phyreset
 *
 * Description:
 *   Reset the PHY
 *
 * Parameters:
 *   phyaddr - The device address where the PHY was discovered
 *
 * Returned Value:
 *   None
 *
 * Assumptions:
 *
 ****************************************************************************/

#ifdef LPC17_HAVE_PHY
static inline int lpc17_phyreset(uint8_t phyaddr)
{
  int32_t timeout;
  uint16_t phyreg;

  /* Reset the PHY.  Needs a minimal 50uS delay after reset. */

  lpc17_phywrite(phyaddr, MII_MCR, MII_MCR_RESET);

  /* Wait for a minimum of 50uS no matter what */

  up_udelay(50);

  /* The MCR reset bit is self-clearing.  Wait for it to be clear
   * indicating that the reset is complete.
   */

  for (timeout = MII_BIG_TIMEOUT; timeout > 0; timeout--)
    {
      phyreg = lpc17_phyread(phyaddr, MII_MCR);
      if ((phyreg & MII_MCR_RESET) == 0)
        {
          return OK;
        }
    }

  ndbg("Reset failed. MCR: %04x\n", phyreg);
  return -ETIMEDOUT;
}
#endif

/****************************************************************************
 * Function: lpc17_phyautoneg
 *
 * Description:
 *   Enable auto-negotiation.
 *
 * Parameters:
 *   phyaddr - The device address where the PHY was discovered
 *
 * Returned Value:
 *   None
 *
 * Assumptions:
 *   The adverisement regiser has already been configured.
 *
 ****************************************************************************/

#if defined(LPC17_HAVE_PHY) && defined(CONFIG_PHY_AUTONEG)
static inline int lpc17_phyautoneg(uint8_t phyaddr)
{
  int32_t timeout;
  uint16_t phyreg;

  /* Start auto-negotiation */

  lpc17_phywrite(phyaddr, MII_MCR, MII_MCR_ANENABLE | MII_MCR_ANRESTART);

  /* Wait for autonegotiation to complete */

  for (timeout = MII_BIG_TIMEOUT; timeout > 0; timeout--)
    {
      /* Check if auto-negotiation has completed */

      phyreg = lpc17_phyread(phyaddr, MII_MSR);
      if ((phyreg & (MII_MSR_LINKSTATUS | MII_MSR_ANEGCOMPLETE)) == 
          (MII_MSR_LINKSTATUS | MII_MSR_ANEGCOMPLETE))
        {
          /* Yes.. return success */

          return OK;
        }
    }

  ndbg("Auto-negotiation failed. MSR: %04x\n", phyreg);
  return -ETIMEDOUT;
}
#endif

/****************************************************************************
 * Function: lpc17_phymode
 *
 * Description:
 *   Set the PHY to operate at a selected speed/duplex mode.
 *
 * Parameters:
 *   phyaddr - The device address where the PHY was discovered
 *   mode - speed/duplex mode
 *
 * Returned Value:
 *   None
 *
 * Assumptions:
 *
 ****************************************************************************/

#ifdef LPC17_HAVE_PHY
static int lpc17_phymode(uint8_t phyaddr, uint8_t mode)
{
  int32_t timeout;
  uint16_t phyreg;

  /* Disable auto-negotiation and set fixed Speed and Duplex settings */

  phyreg = 0;
  if ((mode & LPC17_SPEED_MASK) ==  LPC17_SPEED_100)
    {
      phyreg = MII_MCR_SPEED100;
    }

  if ((mode & LPC17_DUPLEX_MASK) == LPC17_DUPLEX_FULL)
    {
      phyreg |= MII_MCR_FULLDPLX;
    }

  lpc17_phywrite(phyaddr, MII_MCR, phyreg);

  /* Then wait for the link to be established */

  for (timeout = MII_BIG_TIMEOUT; timeout > 0; timeout--)
    {
      phyreg = lpc17_phyread(phyaddr, MII_MSR);
      if (phyreg & MII_MSR_LINKSTATUS)
        {
          /* Yes.. return success */

          return OK;
        }
    }

  ndbg("Link failed. MSR: %04x\n", phyreg);
  return -ETIMEDOUT;
}
#endif

/****************************************************************************
 * Function: lpc17_phyinit
 *
 * Description:
 *   Initialize the PHY
 *
 * Parameters:
 *   priv - Pointer to EMAC device driver structure 
 *
 * Returned Value:
 *   None directly.  As a side-effect, it will initialize priv->lp_phyaddr
 *   and priv->lp_phymode.
 *
 * Assumptions:
 *
 ****************************************************************************/

#ifdef LPC17_HAVE_PHY
static inline int lpc17_phyinit(struct lpc17_driver_s *priv)
{
  unsigned int phyaddr;
  uint16_t phyreg;
  uint32_t regval;
  int ret;

  /* MII configuration: host clocked divided per board.h, no suppress
   * preambl,e no scan increment.
   */

  lpc17_putreg(ETH_MCFG_CLKSEL_DIV, LPC17_ETH_MCFG);
  lpc17_putreg(0, LPC17_ETH_MCMD);

  /* Enter RMII mode and select 100 MBPS support */

  lpc17_putreg(ETH_CMD_RMII, LPC17_ETH_CMD);
  lpc17_putreg(ETH_SUPP_SPEED, LPC17_ETH_SUPP);

  /* Find PHY Address.  Because controller has a pull-up and the
   * PHY have pull-down resistors on RXD lines some times the PHY
   * latches different at different addresses.
   */

  for (phyaddr = 1; phyaddr < 32; phyaddr++)
    {
       /* Check if we can see the selected device ID at this
        * PHY address.
        */

       phyreg = (unsigned int)lpc17_phyread(phyaddr, MII_PHYID1);
       if (phyreg == LPC17_PHYID1)
        {
          phyreg = lpc17_phyread(phyaddr, MII_PHYID2);
          if (phyreg  == LPC17_PHYID2)
            {
              break;
            }
        }
    }
  nvdbg("phyaddr: %d\n", phyaddr);

  /* Check if the PHY device address was found */

  if (phyaddr > 31)
    {
      /* Failed to find PHY at any location */

      return -ENODEV;
    }

  /* Save the discovered PHY device address */

  priv->lp_phyaddr = phyaddr;

  /* Reset the PHY */

  ret = lpc17_phyreset(phyaddr);
  if (ret < 0)
    {
      return ret;
    }
  lpc17_showmii(phyaddr, "After reset");

  /* Check for preamble suppression support */

  phyreg = lpc17_phyread(phyaddr, MII_MSR);
  if ((phyreg & MII_MSR_MFRAMESUPPRESS) != 0)
    {
      /* The PHY supports preamble suppression */

      regval  = lpc17_getreg(LPC17_ETH_MCFG);
      regval |= ETH_MCFG_SUPPRE;
      lpc17_putreg(regval, LPC17_ETH_MCFG);
    }

  /* Are we configured to do auto-negotiation? */

#ifdef CONFIG_PHY_AUTONEG
  /* Setup the Auto-negotiation advertisement: 100 or 10, and HD or FD */

  lpc17_phywrite(phyaddr, MII_ADVERTISE, 
                 (MII_ADVERTISE_100BASETXFULL | MII_ADVERTISE_100BASETXHALF |
                  MII_ADVERTISE_10BASETXFULL  | MII_ADVERTISE_10BASETXHALF  |
                  MII_ADVERTISE_CSMA));

  /* Then perform the auto-negotiation */

  ret = lpc17_phyautoneg(phyaddr);
  if (ret < 0)
    {
      return ret;
    }
#else
  /* Set up the fixed PHY configuration */

  ret = lpc17_phymode(phyaddr, LPC17_MODE_DEFLT);
  if (ret < 0)
    {
      return ret;
    }
#endif

  /* The link is established */

  lpc17_showmii(phyaddr, "After link established");

  /* Check configuration */

#ifdef CONFIG_PHY_KS8721
  phyreg = lpc17_phyread(phyaddr, MII_KS8721_10BTCR);

  switch (phyreg & KS8721_10BTCR_MODE_MASK)
    {
      case KS8721_10BTCR_MODE_10BTHD:  /* 10BASE-T half duplex */
        priv->lp_mode = LPC17_10BASET_HD;
        lpc17_putreg(0, LPC17_ETH_SUPP);
        break;
      case KS8721_10BTCR_MODE_100BTHD: /* 100BASE-T half duplex */
        priv->lp_mode = LPC17_100BASET_HD;
        break;
      case KS8721_10BTCR_MODE_10BTFD: /* 10BASE-T full duplex */
        priv->lp_mode = LPC17_10BASET_FD;
        lpc17_putreg(0, LPC17_ETH_SUPP);
        break;
      case KS8721_10BTCR_MODE_100BTFD: /* 100BASE-T full duplex */
        priv->lp_mode = LPC17_100BASET_FD;
        break;
      default:
        dbg("Unrecognized mode: %04x\n", phyreg);
        return -ENODEV;
    }
#else
#  warning "PHY Unknown: speed and duplex are bogus"
#endif

  ndbg("%dBase-T %s duplex\n",
       (priv->lp_mode & LPC17_SPEED_MASK) ==  LPC17_SPEED_100 ? 100 : 10,
       (priv->lp_mode & LPC17_DUPLEX_MASK) == LPC17_DUPLEX_FULL ?"full" : "half");

  /* Disable auto-configuration.  Set the fixed speed/duplex mode.
   * (probably more than little redundant).
   */

  ret = lpc17_phymode(phyaddr, LPC17_MODE_DEFLT);
  lpc17_showmii(phyaddr, "After final configuration");
  return ret;
}
#else
static inline int lpc17_phyinit(struct lpc17_driver_s *priv)
{
  priv->lp_mode = LPC17_MODE_DEFLT;
  return OK;
}
#endif

/****************************************************************************
 * Function: lpc17_txdescinit
 *
 * Description:
 *   Initialize the EMAC Tx descriptor table
 *
 * Parameters:
 *   priv - Pointer to EMAC device driver structure 
 *
 * Returned Value:
 *   None directory.
 *   As a side-effect, it will initialize priv->lp_phyaddr and
 *   priv->lp_phymode.
 *
 * Assumptions:
 *
 ****************************************************************************/

static inline void lpc17_txdescinit(struct lpc17_driver_s *priv)
{
  uint32_t *txdesc;
  uint32_t *txstat;
  uint32_t pktaddr;
  int i;

  /* Configure Tx descriptor and status tables */

  lpc17_putreg(LPC17_TXDESC_BASE, LPC17_ETH_TXDESC);
  lpc17_putreg(LPC17_TXSTAT_BASE, LPC17_ETH_TXSTAT);
  lpc17_putreg(CONFIG_NET_NTXDESC-1, LPC17_ETH_TXDESCRNO);

  /* Initialize Tx descriptors and link to packet buffers */

  txdesc  = (uint32_t*)LPC17_TXDESC_BASE;
  pktaddr = LPC17_TXBUFFER_BASE;

  for (i = 0; i < CONFIG_NET_NTXDESC; i++)
    {
      *txdesc++ = pktaddr;
      *txdesc++ = (TXDESC_CONTROL_INT | (LPC17_MAXPACKET_SIZE - 1));
      pktaddr  += LPC17_MAXPACKET_SIZE;
    }

  /* Initialize Tx status */

  txstat  = (uint32_t*)LPC17_TXSTAT_BASE;
  for (i = 0; i < CONFIG_NET_NTXDESC; i++)
    {
      *txstat++ = 0;
    }

  /* Point to first Tx descriptor */

  lpc17_putreg(0, LPC17_ETH_TXPRODIDX);
}

/****************************************************************************
 * Function: lpc17_rxdescinit
 *
 * Description:
 *   Initialize the EMAC Rx descriptor table
 *
 * Parameters:
 *   priv - Pointer to EMAC device driver structure 
 *
 * Returned Value:
 *   None directory.
 *   As a side-effect, it will initialize priv->lp_phyaddr and
 *   priv->lp_phymode.
 *
 * Assumptions:
 *
 ****************************************************************************/

static inline void lpc17_rxdescinit(struct lpc17_driver_s *priv)
{
  uint32_t *rxdesc;
  uint32_t *rxstat;
  uint32_t pktaddr;
  int i;

  /* Configure Rx descriptor and status tables */

  lpc17_putreg(LPC17_RXDESC_BASE, LPC17_ETH_RXDESC);
  lpc17_putreg(LPC17_RXSTAT_BASE, LPC17_ETH_RXSTAT);
  lpc17_putreg(CONFIG_NET_NRXDESC-1, LPC17_ETH_RXDESCNO);

  /* Initialize Rx descriptors and link to packet buffers */

  rxdesc  = (uint32_t*)LPC17_RXDESC_BASE;
  pktaddr = LPC17_RXBUFFER_BASE;

  for (i = 0; i < CONFIG_NET_NRXDESC; i++)
    {
      *rxdesc++ = pktaddr;
      *rxdesc++ = (RXDESC_CONTROL_INT | (LPC17_MAXPACKET_SIZE - 1));
      pktaddr  += LPC17_MAXPACKET_SIZE;
    }

  /* Initialize Rx status */

  rxstat  = (uint32_t*)LPC17_TXSTAT_BASE;
  for (i = 0; i < CONFIG_NET_NRXDESC; i++)
    {
      *rxstat++ = 0;
      *rxstat++ = 0;
    }

  /* Point to first Tx descriptor */

  lpc17_putreg(0, LPC17_ETH_RXPRODIDX);
}

/****************************************************************************
 * Function: lpc17_macmode
 *
 * Description:
 *   Set the MAC to operate at a selected speed/duplex mode.
 *
 * Parameters:
 *   mode - speed/duplex mode
 *
 * Returned Value:
 *   None
 *
 * Assumptions:
 *
 ****************************************************************************/

#ifdef LPC17_HAVE_PHY
static void lpc17_macmode(uint8_t mode)
{
  uint32_t regval;

  /* Set up for full or half duplex operation */

  if ((mode & LPC17_DUPLEX_MASK) == LPC17_DUPLEX_FULL)
    {
      /* Set the back-to-back inter-packet gap */
 
      lpc17_putreg(21, LPC17_ETH_IPGT);

      /* Set MAC to operate in full duplex mode with CRC and Pad enabled */

      regval = lpc17_getreg(LPC17_ETH_MAC2);
      regval |= (ETH_MAC2_FD | ETH_MAC2_CRCEN | ETH_MAC2_PADCRCEN);
      lpc17_putreg(regval, LPC17_ETH_MAC2);

      /* Select full duplex operation for ethernet controller */

      regval = lpc17_getreg(LPC17_ETH_CMD);
      regval |= (ETH_CMD_FD | ETH_CMD_RMII | ETH_CMD_PRFRAME);
      lpc17_putreg(regval, LPC17_ETH_CMD);
    }
  else
    {
      /* Set the back-to-back inter-packet gap */
 
      lpc17_putreg(18, LPC17_ETH_IPGT);

      /* Set MAC to operate in half duplex mode with CRC and Pad enabled */

      regval = lpc17_getreg(LPC17_ETH_MAC2);
      regval &= ~ETH_MAC2_FD;
      regval |= (ETH_MAC2_CRCEN | ETH_MAC2_PADCRCEN);
      lpc17_putreg(regval, LPC17_ETH_MAC2);

      /* Select half duplex operation for ethernet controller */

      regval = lpc17_getreg(LPC17_ETH_CMD);
      regval &= ~ETH_CMD_FD;
      regval |= (ETH_CMD_RMII | ETH_CMD_PRFRAME);
      lpc17_putreg(regval, LPC17_ETH_CMD);
    }

  /* This is currently done in lpc17_phyinit().  That doesn't
   * seem like the right place. It should be done here.
   */

#if 0
  regval = lpc17_getreg(LPC17_ETH_SUPP);
  if ((mode & LPC17_SPEED_MASK) == LPC17_SPEED_100)
    {
      regval |= ETH_SUPP_SPEED;
    }
  else
    {
      regval &= ~ETH_SUPP_SPEED;
    }
  lpc17_putreg(regval, LPC17_ETH_SUPP);
#endif
}
#endif

/****************************************************************************
 * Function: lpc17_ethreset
 *
 * Description:
 *   Configure and reset the Ethernet module, leaving it in a disabled state.
 *
 * Parameters:
 *   priv   - Reference to the driver state structure
 *
 * Returned Value:
 *   OK on success; a negated errno on failure
 *
 * Assumptions:
 *
 ****************************************************************************/

static void lpc17_ethreset(struct lpc17_driver_s *priv)
{
  irqstate_t flags;

  /* Reset the MAC */

  flags = irqsave();

  /* Put the MAC into the reset state */

  lpc17_putreg((ETH_MAC1_TXRST    | ETH_MAC1_MCSTXRST |ETH_MAC1_RXRST |
                ETH_MAC1_MCSRXRST | ETH_MAC1_SIMRST   | ETH_MAC1_SOFTRST),
               LPC17_ETH_MAC1);

  /* Disable RX/RX, clear modes, reset all control registers */

  lpc17_putreg((ETH_CMD_REGRST | ETH_CMD_TXRST | ETH_CMD_RXRST),
               LPC17_ETH_CMD);

  /* Take the MAC out of the reset state */

  up_udelay(50);
  lpc17_putreg(0, LPC17_ETH_MAC1);

  /* The RMII bit must be set on initialization (I'm not sure
   * this needs to be done here but... oh well.
   */

  lpc17_putreg(ETH_CMD_RMII, LPC17_ETH_CMD);

  /* Set other misc configuration-related registers to default values */

  lpc17_putreg(0, LPC17_ETH_MAC2);
  lpc17_putreg(0, LPC17_ETH_SUPP);
  lpc17_putreg(0, LPC17_ETH_TEST);

  lpc17_putreg(18, LPC17_ETH_IPGR);
  lpc17_putreg(((15 << ETH_CLRT_RMAX_SHIFT) | (55 << ETH_CLRT_COLWIN_SHIFT)),
               LPC17_ETH_CLRT);
  lpc17_putreg(0x0600, LPC17_ETH_MAXF);

  /* Disable all Ethernet controller interrupts */

  lpc17_putreg(0, LPC17_ETH_INTEN);

  /* Clear any pending interrupts (shouldn't be any) */

  lpc17_putreg(0xffffffff, LPC17_ETH_INTCLR);
  irqrestore(flags);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Function: lpc17_ethinitialize
 *
 * Description:
 *   Initialize one Ethernet controller and driver structure.
 *
 * Parameters:
 *   intf - Selects the interface to be initialized.
 *
 * Returned Value:
 *   OK on success; Negated errno on failure.
 *
 * Assumptions:
 *
 ****************************************************************************/

#if CONFIG_LPC17_NINTERFACES > 1
int lpc17_ethinitialize(int intf)
#else
static inline int lpc17_ethinitialize(int intf)
#endif
{
  struct lpc17_driver_s *priv;
  uint32_t regval;
  int ret;
  int i;

  DEBUGASSERT(intf < CONFIG_LPC17_NINTERFACES);
  priv = &g_ethdrvr[intf];

  /* Turn on the ethernet MAC clock */

  regval  = lpc17_getreg(LPC17_SYSCON_PCONP);
  regval |= SYSCON_PCONP_PCENET;
  lpc17_putreg(regval, LPC17_SYSCON_PCONP);

  /* Configure all GPIO pins needed by ENET */

  for (i = 0; i < GPIO_NENET_PINS; i++)
    {
      (void)lpc17_configgpio(g_enetpins[i]);
    }
  lpc17_showpins();

  /* Initialize the driver structure */

  memset(priv, 0, sizeof(struct lpc17_driver_s));
  priv->lp_dev.d_ifup    = lpc17_ifup;    /* I/F down callback */
  priv->lp_dev.d_ifdown  = lpc17_ifdown;  /* I/F up (new IP address) callback */
  priv->lp_dev.d_txavail = lpc17_txavail; /* New TX data callback */
#ifdef CONFIG_NET_IGMP
  priv->lp_dev.d_addmac  = lpc17_addmac;  /* Add multicast MAC address */
  priv->lp_dev.d_rmmac   = lpc17_rmmac;   /* Remove multicast MAC address */
#endif
  priv->lp_dev.d_private = (void*)priv;   /* Used to recover private state from dev */

#if CONFIG_LPC17_NINTERFACES > 1
# error "A mechanism to associate base address an IRQ with an interface is needed"
  priv->lp_base          = ??;            /* Ethernet controller base address */
  priv->lp_irq           = ??;            /* Ethernet controller IRQ number */
#endif

  /* Create a watchdog for timing polling for and timing of transmisstions */

  priv->lp_txpoll        = wd_create();   /* Create periodic poll timer */
  priv->lp_txtimeout     = wd_create();   /* Create TX timeout timer */

  /* Reset the Ethernet controller and leave in the ifdown statue.  The
   * Ethernet controller will be properly re-initialized each time
   * lpc17_ifup() is called.
   */

  lpc17_ifdown(&priv->lp_dev);

  /* Attach the IRQ to the driver */

#if CONFIG_LPC17_NINTERFACES > 1
  ret = irq_attach(priv->irq, lpc17_interrupt);
#else
  ret = irq_attach(LPC17_IRQ_ETH, lpc17_interrupt);
#endif
  if (ret != 0)
    {
      /* We could not attach the ISR to the the interrupt */

      return -EAGAIN;
    }

  /* Register the device with the OS so that socket IOCTLs can be performed */

  (void)netdev_register(&priv->lp_dev);
  return OK;
}

/****************************************************************************
 * Name: up_netinitialize
 *
 * Description:
 *   Initialize the first network interface.  If there are more than one
 *   interface in the chip, then board-specific logic will have to provide
 *   this function to determine which, if any, Ethernet controllers should
 *   be initialized.
 *
 ****************************************************************************/

#if CONFIG_LPC17_NINTERFACES == 1
void up_netinitialize(void)
{
  (void)lpc17_ethinitialize(0);
}
#endif
#endif /* LPC17_NETHCONTROLLERS > 0 */
#endif /* CONFIG_NET && CONFIG_LPC17_ETHERNET */
