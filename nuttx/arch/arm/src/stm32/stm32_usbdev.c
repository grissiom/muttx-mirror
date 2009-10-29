/****************************************************************************
 * arch/arm/src/stm32/stm32_usbdev.c
 *
 *   Copyright (C) 2009 Gregory Nutt. All rights reserved.
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
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <debug.h>

#include <nuttx/arch.h>
#include <nuttx/usb.h>
#include <nuttx/usbdev.h>
#include <nuttx/usbdev_trace.h>

#include <arch/irq.h>

#include "up_arch.h"
#include "stm32_internal.h"
#include "stm32_usbdev.h"

#if defined(CONFIG_USBDEV) && defined(CONFIG_STM32_USB)

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Configuration ************************************************************/

#ifndef CONFIG_USBDEV_EP0_MAXSIZE
#  define CONFIG_USBDEV_EP0_MAXSIZE 64
#endif

#ifndef  CONFIG_USBDEV_MAXPOWER
#  define CONFIG_USBDEV_MAXPOWER 100  /* mA */
#endif

#define USB_SLOW_INT USBDEV_DEVINT_EPSLOW
#define USB_DEVSTATUS_INT USBDEV_DEVINT_DEVSTAT

#ifdef CONFIG_STM32_USBDEV_EPFAST_INTERRUPT
#  define USB_FAST_INT USBDEV_DEVINT_EPFAST
#else
#  define USB_FAST_INT 0
#endif

#ifndef CONFIG_USB_PRI
#  define CONFIG_USB_PRI 2
#endif

/* Extremely detailed register debug that you would normally never want
 * enabled.
 */

#ifndef CONFIG_DEBUG
#  undef CONFIG_STM32_USBDEV_REGDEBUG
#endif

/* Initial interrupt mask */

#define STM32_CNTR_SETUP     (USB_CNTR_RESETM|USB_CNTR_SUSPM|USB_CNTR_WKUPM|USB_CNTR_CTRM)

/* Endpoint identifiers. The STM32 supports up to 16 mono-directional or 8
 * bidirectional endpoints.  However, when you take into account PMA buffer
 * usage (see below) and the fact that EP0 is bidirectional, then there is
 * a functional limitation of EP0 + 5 mono-directional endpoints = 6.
 */

#define STM32_NENDPOINTS      (8)
#define EP0                   (0)
#define EP1                   (1)
#define EP2                   (2)
#define EP3                   (3)
#define EP4                   (4)
#define EP5                   (5)
#define EP6                   (6)
#define EP7                   (7)

#define STM32_ENDP_BIT(ep)    (1 << (ep))
#define STM32_ENDP_ALLSET     0xff

/* Packet sizes.  We us a fixed 64 max packet size for all endpoint types */

#define STM32_MAXPACKET_SHIFT (6)
#define STM32_MAXPACKET_SIZE  (1 << (STM32_MAXPACKET_SHIFT))
#define STM32_MAXPACKET_MASK  (STM32_MAXPACKET_SIZE-1)

#define STM32_EP0MAXPACKET    STM32_MAXPACKET_SIZE 

/* Buffer description table.  We assume that USB has excluse use of the buffer
 * table.  The buffer table is positioned at the beginning of the 512-byte
 * CAN/USB memory.  We will use the first STM32_NENDPOINTS*4 words for the buffer
 * table.
 */

#define STM32_BTABLE_ADDRESS  (0x00)
#define STM32_BTABLE_SIZE     (0x18)

/* Buffer layout.  Assume that all buffers are 64-bytes (maxpacketsize), then
 * we have space for only 7 buffers; endpoint 0 will require two buffers, leaving
 * 5 for other endpoints.
 */

#define STM32_BUFFER_START    STM32_BTABLE_SIZE
#define STM32_EP0_RXADDR      STM32_BUFFER_START
#define STM32_EP0_TXADDR      (STM32_EP0_RXADDR+STM32_EP0MAXPACKET)

#define STM32_BUFFER_EP0      0x03
#define STM32_NBUFFERS        7
#define STM32_BUFFER_BIT(bn)  (1 << (bn))
#define STM32_BUFFER_ALLSET   0x7f
#define STM32_BUFNO2BUF(bn)   (STM32_BUFFER_START+((bn)<<STM32_MAXPACKET_SHIFT))

/* USB-related masks */

#define REQRECIPIENT_MASK     (USB_REQ_TYPE_MASK | USB_REQ_RECIPIENT_MASK)

/* Endpoint rister masks (handling toggle fields) */

#define EPR_NOTOG_MASK        (USB_EPR_CTR_RX  | USB_EPR_SETUP  | USB_EPR_EPTYPE_MASK |\
                               USB_EPR_EP_KIND | USB_EPR_CTR_TX | USB_EPR_EA_MASK)
#define EPR_TXDTOG_MASK       (USB_EPR_STATTX_MASK | EPR_NOTOG_MASK)
#define EPR_RXDTOG_MASK       (USB_EPR_STATRX_MASK | EPR_NOTOG_MASK)

/* Request queue operations *************************************************/

#define stm32_rqempty(ep)     ((ep)->head == NULL)
#define stm32_rqpeek(ep)      ((ep)->head)

/* USB trace ****************************************************************/
/* Trace error codes */

#define STM32_TRACEERR_ALLOCFAIL            0x0001
#define STM32_TRACEERR_BADCLEARFEATURE      0x0002
#define STM32_TRACEERR_BADDEVGETSTATUS      0x0003
#define STM32_TRACEERR_BADEPGETSTATUS       0x0004
#define STM32_TRACEERR_BADEPNO              0x0005
#define STM32_TRACEERR_BADEPTYPE            0x0006
#define STM32_TRACEERR_BADGETCONFIG         0x0007
#define STM32_TRACEERR_BADGETSETDESC        0x0008
#define STM32_TRACEERR_BADGETSTATUS         0x0009
#define STM32_TRACEERR_BADSETADDRESS        0x000a
#define STM32_TRACEERR_BADSETCONFIG         0x000b
#define STM32_TRACEERR_BADSETFEATURE        0x000c
#define STM32_TRACEERR_BINDFAILED           0x000d
#define STM32_TRACEERR_DISPATCHSTALL        0x000e
#define STM32_TRACEERR_DRIVER               0x000f
#define STM32_TRACEERR_DRIVERREGISTERED     0x0010
#define STM32_TRACEERR_EP0SETUPSTALLED      0x0011
#define STM32_TRACEERR_EPBUFFER             0x0012
#define STM32_TRACEERR_EPDISABLED           0x0013
#define STM32_TRACEERR_EPOUTNULLPACKET      0x0014
#define STM32_TRACEERR_EPRESERVE            0x0015
#define STM32_TRACEERR_INVALIDCTRLREQ       0x0016
#define STM32_TRACEERR_INVALIDPARMS         0x0017
#define STM32_TRACEERR_IRQREGISTRATION      0x0018
#define STM32_TRACEERR_NOTCONFIGURED        0x0019
#define STM32_TRACEERR_REQABORTED           0x001a

/* Trace interrupt codes */

#define STM32_TRACEINTID_CLEARFEATURE       0x0001
#define STM32_TRACEINTID_DEVGETSTATUS       0x0002
#define STM32_TRACEINTID_DISPATCH           0x0003
#define STM32_TRACEINTID_EP0SETUPSETADDRESS 0x0004
#define STM32_TRACEINTID_EPGETSTATUS        0x0005
#define STM32_TRACEINTID_EPIN               0x0006
#define STM32_TRACEINTID_EPINQEMPTY         0x0007
#define STM32_TRACEINTID_EPOUT              0x0008
#define STM32_TRACEINTID_EPOUTPENDING       0x0009
#define STM32_TRACEINTID_EPOUTQEMPTY        0x000a
#define STM32_TRACEINTID_ESOF               0x000b
#define STM32_TRACEINTID_GETCONFIG          0x000c
#define STM32_TRACEINTID_GETSETDESC         0x000d
#define STM32_TRACEINTID_GETSETIF           0x000e
#define STM32_TRACEINTID_GETSTATUS          0x000f
#define STM32_TRACEINTID_HPINTERRUPT        0x0010
#define STM32_TRACEINTID_IFGETSTATUS        0x0011
#define STM32_TRACEINTID_LPCTR              0x0012
#define STM32_TRACEINTID_LPINTERRUPT        0x0013
#define STM32_TRACEINTID_NOSTDREQ           0x0014
#define STM32_TRACEINTID_RESET              0x0015
#define STM32_TRACEINTID_SETCONFIG          0x0016
#define STM32_TRACEINTID_SETFEATURE         0x0017
#define STM32_TRACEINTID_SUSP               0x0018
#define STM32_TRACEINTID_SYNCHFRAME         0x0019
#define STM32_TRACEINTID_WKUP               0x001a

/* Ever-present MIN and MAX macros */

#ifndef MIN
#  define MIN(a,b) (a < b ? a : b)
#endif

#ifndef MAX
#  define MAX(a,b) (a > b ? a : b)
#endif

/****************************************************************************
 * Private Type Definitions
 ****************************************************************************/

/* The various states of a control pipe */

enum stm32_devstate_e 
{
  DEVSTATE_INIT,
  DEVSTATE_RDREQUEST,       /* Read request in progress */
  DEVSTATE_WRREQUEST,       /* Write request in progress */
  DEVSTATE_IDLE,            /* No transfer in progress */
  DEVSTATE_STALLED          /* We are stalled */
};

/* Resume states */

enum stm32_rsmstate_e 
{
  RSMSTATE_IDLE = 0,        /* Device is either fully suspended or running */
  RSMSTATE_STARTED,         /* Resume sequence has been started */
  RSMSTATE_WAITING          /* Waiting (on ESOFs) for end of sequence */
};

union wb_u
{
  uint16 w;
  ubyte  b[2];
};

/* A container for a request so that the request make be retained in a list */

struct stm32_req_s
{
  struct usbdev_req_s    req;           /* Standard USB request */
  struct stm32_req_s  *flink;           /* Supports a singly linked list */
};

/* This is the internal representation of an endpoint */

struct stm32_ep_s
{
  /* Common endpoint fields.  This must be the first thing defined in the
   * structure so that it is possible to simply cast from struct usbdev_ep_s
   * to struct stm32_ep_s.
   */

  struct usbdev_ep_s      ep;           /* Standard endpoint structure */

  /* STR71X-specific fields */

  struct stm32_usbdev_s *dev;           /* Reference to private driver data */
  struct stm32_req_s    *head;          /* Request list for this endpoint */
  struct stm32_req_s    *tail;
  ubyte                  bufno;         /* Allocated buffer number */
  ubyte                  stalled:1;     /* 1: Endpoint is stalled */
  ubyte                  halted:1;      /* 1: Endpoint feature halted */
  ubyte                  txbusy:1;      /* 1: TX endpoint FIFO full */
  ubyte                  txnullpkt:1;   /* Null packet needed at end of transfer */
};

struct stm32_usbdev_s
{
  /* Common device fields.  This must be the first thing defined in the
   * structure so that it is possible to simply cast from struct usbdev_s
   * to structstm32_usbdev_s.
   */

  struct usbdev_s usbdev;

  /* The bound device class driver */

  struct usbdevclass_driver_s *driver;

  /* STM32-specific fields */

  struct usb_ctrlreq_s     ctrl;          /* Last EP0 request */
  ubyte                    devstate;      /* Driver state (see enum stm32_devstate_e) */
  ubyte                    rsmstate;      /* Resume state (see enum stm32_rsmstate_e) */
  ubyte                    nesofs;        /* ESOF counter (for resume support) */
  ubyte                    rxpending:1;   /* 1: OUT data in PMA, but no read requests */
  ubyte                    selfpowered:1; /* 1: Device is self powered */
  ubyte                    epavail;       /* Bitset of available endpoints */
  ubyte                    bufavail;      /* Bitset of available buffers */
  uint16                   rxstatus;      /* Saved during interrupt processing */
  uint16                   txstatus;      /* "   " "    " "       " "        " */
  uint16                   imask;         /* Current interrupt mask */

  /* The endpoint list */

  struct stm32_ep_s        eplist[STM32_NENDPOINTS];
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/* Register operations ******************************************************/

#if defined(CONFIG_STM32_USBDEV_REGDEBUG) && defined(CONFIG_DEBUG)
static uint16 stm32_getreg(uint32 addr);
static void stm32_putreg(uint16 val, uint32 addr);
static void stm32_checksetup(void);
#else
# define stm32_getreg(addr)     getreg16(addr)
# define stm32_putreg(val,addr) putreg16(val,addr)
# define stm32_checksetup()
#endif

/* Low-Level Helpers ********************************************************/

static inline void
              stm32_epsettxcount(ubyte epno, uint16 count);
static inline void
              stm32_seteptxaddr(ubyte epno, ubyte addr);
static void   stm32_epsetrxcount(ubyte epno, uint16 count);
static inline void
              stm32_seteprxaddr(ubyte epno, ubyte addr);
static inline void
              stm32_setepaddress(ubyte epno, ubyte addr);
static inline void
              stm32_seteptype(ubyte epno, uint16 type);
static inline void
              stm32_seteptxaddr(ubyte epno, ubyte addr);
static inline void
              stm32_setstatusout(ubyte epno);
static inline void
              stm32_clrstatusout(ubyte epno);
static void   stm32_clrrxdtog(ubyte epno);
static void   stm32_clrtxdtog(ubyte epno);
static void   stm32_clrepctrrx(ubyte epno);
static void   stm32_clrepctrtx(ubyte epno);
static void   stm32_seteptxstatus(ubyte epno, uint16 state);
static void   stm32_seteprxstatus(ubyte epno, uint16 state);
static inline uint16
              stm32_geteptxstatus(ubyte epno);
static inline uint16
              stm32_geteprxstatus(ubyte epno);
static uint16 stm32_eptxstalled(ubyte epno);
static uint16 stm32_eprxstalled(ubyte epno);
static void   stm32_suspend(struct stm32_usbdev_s *priv);
static void   stm32_initresume(struct stm32_usbdev_s *priv);
static void   stm32_esofpoll(struct stm32_usbdev_s *priv) ;

/* Request Helpers **********************************************************/

static void   stm32_copytopma(const ubyte *buffer, uint16 pmaoffset, uint16 nbytes);
static inline void
              stm32_copyfrompma(ubyte *buffer, uint16 pmaoffset, uint16 nbytes);
static struct stm32_req_s *
              stm32_rqdequeue(struct stm32_ep_s *privep);
static void   stm32_rqenqueue(struct stm32_ep_s *privep,
                struct stm32_req_s *req);
static inline void
              stm32_abortrequest(struct stm32_ep_s *privep,
                struct stm32_req_s *privreq, sint16 result);
static void   stm32_reqcomplete(struct stm32_ep_s *privep, sint16 result);
static void   stm32_epwrite(struct stm32_usbdev_s *buf, struct stm32_ep_s *privep,
                const ubyte *data, uint32 nbytes);
static int    stm32_wrrequest(struct stm32_usbdev_s *priv, struct stm32_ep_s *privep);
static int    stm32_rdrequest(struct stm32_usbdev_s *priv, struct stm32_ep_s *privep);

/* Interrupt level processing ***********************************************/

static int    stm32_dispatchrequest(struct stm32_usbdev_s *priv);
static void   stm32_ep0post(struct stm32_usbdev_s *priv);
static void   stm32_ep0setup(struct stm32_usbdev_s *priv);
static void   stm32_ep0out(struct stm32_usbdev_s *priv, struct stm32_ep_s *privep);
static void   stm32_ep0in(struct stm32_usbdev_s *priv);
static void   stm32_setdevaddr(struct stm32_usbdev_s *priv, ubyte value);
static void   stm32_lptransfer(struct stm32_usbdev_s *priv);
static int    stm32_hpinterrupt(int irq, void *context);
static int    stm32_lpinterrupt(int irq, void *context);

/* Endpoint helpers *********************************************************/

static inline struct stm32_ep_s *
              stm32_reserveep(struct stm32_usbdev_s *priv, ubyte epset);
static inline void
              stm32_unreserveep(struct stm32_usbdev_s *priv,
                struct stm32_ep_s *privep);
static inline boolean
              stm32_epreserved(struct stm32_usbdev_s *priv, int epno);
static int    stm32_allocpma(struct stm32_usbdev_s *priv);
static inline void
              stm32_freepma(struct stm32_usbdev_s *priv,
                struct stm32_ep_s *privep);

/* Endpoint operations ******************************************************/

static int    stm32_epconfigure(struct usbdev_ep_s *ep,
                const struct usb_epdesc_s *desc, boolean last);
static int    stm32_epdisable(struct usbdev_ep_s *ep);
static struct usbdev_req_s *
              stm32_epallocreq(struct usbdev_ep_s *ep);
static void   stm32_epfreereq(struct usbdev_ep_s *ep,
                struct usbdev_req_s *);
static int    stm32_epsubmit(struct usbdev_ep_s *ep,
                struct usbdev_req_s *req);
static int    stm32_epcancel(struct usbdev_ep_s *ep,
                struct usbdev_req_s *req);
static int    stm32_epstall(struct usbdev_ep_s *ep, boolean resume);

/* USB device controller operations *****************************************/

static struct usbdev_ep_s *
              stm32_allocep(struct usbdev_s *dev, ubyte epno, boolean in,
                ubyte eptype);
static void   stm32_freeep(struct usbdev_s *dev, struct usbdev_ep_s *ep);
static int    stm32_getframe(struct usbdev_s *dev);
static int    stm32_wakeup(struct usbdev_s *dev);
static int    stm32_selfpowered(struct usbdev_s *dev, boolean selfpowered);

/* Initialization ***********************************************************/

static void   stm32_reset(struct stm32_usbdev_s *priv);
static void   stm32_hwreset(struct stm32_usbdev_s *priv);

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* Since there is only a single USB interface, all status information can be
 * be simply retained in a single global instance.
 */

static struct stm32_usbdev_s g_usbdev;

static const struct usbdev_epops_s g_epops =
{
  .configure   = stm32_epconfigure,
  .disable     = stm32_epdisable,
  .allocreq    = stm32_epallocreq,
  .freereq     = stm32_epfreereq,
  .submit      = stm32_epsubmit,
  .cancel      = stm32_epcancel,
  .stall       = stm32_epstall,
};

static const struct usbdev_ops_s g_devops =
{
  .allocep     = stm32_allocep,
  .freeep      = stm32_freeep,
  .getframe    = stm32_getframe,
  .wakeup      = stm32_wakeup,
  .selfpowered = stm32_selfpowered,
  .pullup      = stm32_usbpullup,
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Private Private Functions
 ****************************************************************************/
  
/****************************************************************************
 * Register Operations
 ****************************************************************************/
/****************************************************************************
 * Name: stm32_getreg
 ****************************************************************************/

#if defined(CONFIG_STM32_USBDEV_REGDEBUG) && defined(CONFIG_DEBUG)
static uint16 stm32_getreg(uint32 addr)
{
  static uint32 prevaddr = 0;
  static uint16 preval = 0;
  static uint32 count = 0;

  /* Read the value from the register */

  uint16 val = getreg16(addr);

  /* Is this the same value that we read from the same register last time?
   * Are we polling the register?  If so, suppress some of the output.
   */

  if (addr == prevaddr || val == preval)
    {
      if (count == 0xffffffff || ++count > 3)
        {
           if (count == 4)
             {
               lldbg("...\n");
             }
          return val;
        }
    }

  /* No this is a new address or value */

  else
    {
       /* Did we print "..." for the previous value? */

       if (count > 3)
         {
           /* Yes.. then show how many times the value repeated */

           lldbg("[repeats %d more times]\n", count-3);
         }

       /* Save the new address, value, and count */

       prevaddr = addr;
       preval   = val;
       count    = 1;
    }

  /* Show the register value read */

  lldbg("%08x->%08x\n", addr, val);
  return val;
}
#endif

/****************************************************************************
 * Name: stm32_putreg
 ****************************************************************************/

#if defined(CONFIG_STM32_USBDEV_REGDEBUG) && defined(CONFIG_DEBUG)
static void stm32_putreg(uint16 val, uint32 addr)
{
  /* Show the register value being written */

  lldbg("%08x<-%08x\n", addr, val);

  /* Write the value */

  putreg32(val, addr);
}
#endif

/****************************************************************************
 * Name: stm32_checksetup
 ****************************************************************************/

#if defined(CONFIG_STM32_USBDEV_REGDEBUG) && defined(CONFIG_DEBUG)
static void stm32_checksetup(void)
{
  uint32 cfgr = getreg32(STM32_RCC_CFGR);
  uint32 apb1rstr = getreg32(STM32_RCC_APB1RSTR);
  uint32 apb1enr  = getreg32(STM32_RCC_APB1ENR);

  lldbg("CFGR: %08x APB1RSTR: %08x APB1ENR: %08x\n", cfgr, apb1rstr, apb1enr);

  if ((apb1rstr & RCC_APB1RSTR_USBRST) != 0 ||
      (apb1enr & RCC_APB1ENR_USBEN) == 0)
    {
      lldbg("ERROR: USB is NOT setup correctly\n");
    }
}
#endif

/****************************************************************************
 * Low-Level Helpers
 ****************************************************************************/
/****************************************************************************
 * Name: stm32_epsettxcount
 ****************************************************************************/

static inline void stm32_epsettxcount(ubyte epno, uint16 count) 
{
  uint32 *epaddr = (uint32*)STM32_USB_COUNT_TX(epno);
  *epaddr        = count;
} 

/****************************************************************************
 * Name: stm32_seteptxaddr
 ****************************************************************************/

static inline void stm32_seteptxaddr(ubyte epno, ubyte addr)
{
  uint32 *txaddr = (uint32*)STM32_USB_ADDR_TX(epno);
  *txaddr        = addr;
}

/****************************************************************************
 * Name: stm32_epsetrxcount
 ****************************************************************************/

static void stm32_epsetrxcount(ubyte epno, uint16 count) 
{
  uint32 *epaddr = (uint32*)STM32_USB_COUNT_RX(epno);
  uint16  nblocks;

  if (count > 62)
    {
      /* Blocks of 32 */

      nblocks = count >> 5;

      if((count & 0x1f) == 0)
        {
          nblocks--;
        }

      *epaddr = (uint32)((nblocks << 10) | 0x8000);
    }
  else
    {
      /* Blocks of 2 */

      nblocks = count >> 1;

      if((count & 0x1) != 0)
        {
          nblocks++;
        }

      *epaddr = (uint32)(nblocks << 10);
    }
} 

/****************************************************************************
 * Name: stm32_seteprxaddr
 ****************************************************************************/

static inline void stm32_seteprxaddr(ubyte epno, ubyte addr)
{
  uint32 *rxaddr = (uint32*)STM32_USB_ADDR_RX(epno);
  *rxaddr        = addr;
}

/****************************************************************************
 * Name: stm32_setepaddress
 ****************************************************************************/

static inline void stm32_setepaddress(ubyte epno, ubyte addr) 
{
  uint32 epaddr = STM32_USB_EPR(epno);
  uint16 regval;

  regval  = stm32_getreg(epaddr);
  regval &= EPR_NOTOG_MASK;
  regval &= ~USB_EPR_EA_MASK;
  regval |= USB_EPR_EA_SHIFT;
  stm32_putreg(regval, epaddr);
} 

/****************************************************************************
 * Name: stm32_seteptype
 ****************************************************************************/

static inline void stm32_seteptype(ubyte epno, uint16 type)
{
  uint32 epaddr = STM32_USB_EPR(epno);
  uint16 regval;

  regval  = stm32_getreg(epaddr);
  regval &= EPR_NOTOG_MASK;
  regval &= ~USB_EPR_EPTYPE_MASK;
  regval |= type;
  stm32_putreg(regval, epaddr);
}

/****************************************************************************
 * Name: stm32_setstatusout
 ****************************************************************************/

static inline void stm32_setstatusout(ubyte epno)
{
  uint32 epaddr = STM32_USB_EPR(epno);
  uint16 regval;

  /* For a BULK endpoint the EP_KIND bit is used to enabled double buffering;
   * for a CONTROL endpoint, it is set to indicatate that a status OUT
   * transaction is expected.  The bit is not used with out endpoint types.
   */

  regval  = stm32_getreg(epaddr);
  regval &= EPR_NOTOG_MASK;
  regval |= USB_EPR_EP_KIND;
  stm32_putreg(regval, epaddr);
}

/****************************************************************************
 * Name: stm32_clrstatusout
 ****************************************************************************/

static inline void stm32_clrstatusout(ubyte epno)
{
  uint32 epaddr = STM32_USB_EPR(epno);
  uint16 regval;

  /* For a BULK endpoint the EP_KIND bit is used to enabled double buffering;
   * for a CONTROL endpoint, it is set to indicatate that a status OUT
   * transaction is expected.  The bit is not used with out endpoint types.
   */

  regval  = stm32_getreg(epaddr);
  regval &= EPR_NOTOG_MASK;
  regval &= ~USB_EPR_EP_KIND;
  stm32_putreg(regval, epaddr);
}

/****************************************************************************
 * Name: stm32_clrrxdtog
 ****************************************************************************/

static void stm32_clrrxdtog(ubyte epno) 
{
  uint32 epaddr = STM32_USB_EPR(epno);
  uint16 regval;

  regval = stm32_getreg(epaddr);
  if ((regval & USB_EPR_DTOG_RX) != 0)
    {
      regval &= EPR_NOTOG_MASK;
      regval |= USB_EPR_DTOG_RX;
      stm32_putreg(regval, epaddr);
    } 
} 

/****************************************************************************
 * Name: stm32_clrtxdtog
 ****************************************************************************/

static void stm32_clrtxdtog(ubyte epno) 
{
  uint32 epaddr = STM32_USB_EPR(epno);
  uint16 regval;

  regval = stm32_getreg(epaddr);
  if ((regval & USB_EPR_DTOG_TX) != 0)
    {
      regval &= EPR_NOTOG_MASK;
      regval |= USB_EPR_DTOG_TX;
      stm32_putreg(regval, epaddr);
    }
} 

/****************************************************************************
 * Name: stm32_clrepctrrx
 ****************************************************************************/

static void stm32_clrepctrrx(ubyte epno) 
{
  uint32 epaddr = STM32_USB_EPR(epno);
  uint16 regval;

  regval  = stm32_getreg(epaddr);
  regval &= EPR_NOTOG_MASK;
  regval &= ~USB_EPR_CTR_RX;
  stm32_putreg(regval, epaddr);
} 

/****************************************************************************
 * Name: stm32_clrepctrtx
 ****************************************************************************/

static void stm32_clrepctrtx(ubyte epno) 
{
  uint32 epaddr = STM32_USB_EPR(epno);
  uint16 regval;

  regval  = stm32_getreg(epaddr);
  regval &= EPR_NOTOG_MASK;
  regval &= ~USB_EPR_CTR_TX;
  stm32_putreg(regval, epaddr);
} 

/****************************************************************************
 * Name: stm32_geteptxstatus
 ****************************************************************************/

static inline uint16 stm32_geteptxstatus(ubyte epno) 
{
  return (uint16)(stm32_getreg(STM32_USB_EPR(epno)) & USB_EPR_STATTX_MASK);
}

/****************************************************************************
 * Name: stm32_geteprxstatus
 ****************************************************************************/

static inline uint16 stm32_geteprxstatus(ubyte epno) 
{
  return (stm32_getreg(STM32_USB_EPR(epno)) & USB_EPR_STATRX_MASK);
}

/****************************************************************************
 * Name: stm32_seteptxstatus
 ****************************************************************************/

static void stm32_seteptxstatus(ubyte epno, uint16 state) 
{
  uint32 epaddr = STM32_USB_EPR(epno);
  uint16 regval;

  regval = stm32_getreg(epaddr) & EPR_TXDTOG_MASK;

  /* Toggle first bit */

  if ((USB_EPR_STATTX_DTOG1 & state) != 0)
    {
      regval ^= USB_EPR_STATTX_DTOG1;
    }

  /* Toggle second bit */

  if ((USB_EPR_STATTX_DTOG2 & state) != 0)
    {
      regval ^= USB_EPR_STATTX_DTOG2;
    }

  stm32_putreg(regval, epaddr);
} 

/****************************************************************************
 * Name: stm32_seteprxstatus
 ****************************************************************************/

static void stm32_seteprxstatus(ubyte epno, uint16 state) 
{
  uint32 epaddr = STM32_USB_EPR(epno);
  uint16 regval;

  regval = stm32_getreg(epaddr) & EPR_RXDTOG_MASK;

  /* Toggle first bit */

  if ((USB_EPR_STATTX_DTOG1 & state) != 0)
    {
      regval ^= USB_EPR_STATRX_DTOG1;
    }

  /* Toggle second bit */

  if ((USB_EPR_STATTX_DTOG2 & state) != 0)
    {
      regval ^= USB_EPR_STATRX_DTOG2;
    }

  stm32_putreg(regval, epaddr);
} 

/****************************************************************************
 * Name: stm32_eptxstalled
 ****************************************************************************/

static inline uint16 stm32_eptxstalled(ubyte epno) 
{
  return (stm32_geteptxstatus(epno) == USB_EPR_STATTX_STALL);
}

/****************************************************************************
 * Name: stm32_eprxstalled
 ****************************************************************************/

static inline uint16 stm32_eprxstalled(ubyte epno) 
{
  return (stm32_geteprxstatus(epno) == USB_EPR_STATRX_STALL);
}

/****************************************************************************
 * Request Helpers
 ****************************************************************************/
/****************************************************************************
 * Name: stm32_copytopma
 ****************************************************************************/

static void stm32_copytopma(const ubyte *buffer, uint16 pmaoffset, uint16 nbytes) 
{
  uint16 *dest;
  uint16  ms;
  uint16  ls;
  int     nwords = (nbytes + 1) >> 1;
  int     i;

  /* Copy loop.  Source=user buffer, Dest=packet memory */

  dest = (uint16*)((pmaoffset << 1) + STM32_USBCANRAM_BASE);
  for (i = nwords; i != 0; i--)
    {
      /* Read two bytes and pack into on 16-bit word */

      ls = (uint16)(*buffer++);
      ms = (uint16)(*buffer++);
      *dest = ms << 8 | ls;

      /* Source address increments by 2*sizeof(ubyte) = 2; Dest address
       * increments by 2*sizeof(uint16) = 4.
       */

      dest += 2;
    }
}

/****************************************************************************
 * Name: stm32_copyfrompma
 ****************************************************************************/

static inline void
stm32_copyfrompma(ubyte *buffer, uint16 pmaoffset, uint16 nbytes) 
{
  uint32 *src;
  int     nwords = (nbytes + 1) >> 1;
  int     i;

  /* Copy loop.  Source=packet memory, Dest=user buffer */

  src = (uint32*)((pmaoffset << 1) + STM32_USBCANRAM_BASE);
  for (i = nwords; i != 0; i--)
    {
      /* Copy 16-bits from packet memory to user buffer. */

      *(uint16*)buffer = *src++;

      /* Source address increments by 1*sizeof(uint32) = 4; Dest address
       * increments by 2*sizeof(ubyte) = 2.
       */
      buffer += 2;
    }
}

/****************************************************************************
 * Name: stm32_rqdequeue
 ****************************************************************************/

static struct stm32_req_s *stm32_rqdequeue(struct stm32_ep_s *privep)
{
  struct stm32_req_s *ret = privep->head;

  if (ret)
    {
      privep->head = ret->flink;
      if (!privep->head)
        {
          privep->tail = NULL;
        }

      ret->flink = NULL;
    }

  return ret;
}

/****************************************************************************
 * Name: stm32_rqenqueue
 ****************************************************************************/

static void stm32_rqenqueue(struct stm32_ep_s *privep,
                              struct stm32_req_s *req)
{
  req->flink = NULL;
  if (!privep->head)
    {
      privep->head = req;
      privep->tail = req;
    }
  else
    {
      privep->tail->flink = req;
      privep->tail        = req;
    }
}

/****************************************************************************
 * Name: stm32_abortrequest
 ****************************************************************************/

static inline void
stm32_abortrequest(struct stm32_ep_s *privep, struct stm32_req_s *privreq, sint16 result)
{
  usbtrace(TRACE_DEVERROR(STM32_TRACEERR_REQABORTED), (uint16)USB_EPNO(privep->ep.eplog));

  /* Save the result in the request structure */

  privreq->req.result = result;

  /* Callback to the request completion handler */

  privreq->req.callback(&privep->ep, &privreq->req);
}

/****************************************************************************
 * Name: stm32_reqcomplete
 ****************************************************************************/

static void stm32_reqcomplete(struct stm32_ep_s *privep, sint16 result)
{
  struct stm32_req_s *privreq;
  irqstate_t flags;

  /* Remove the completed request at the head of the endpoint request list */

  flags = irqsave();
  privreq = stm32_rqdequeue(privep);
  irqrestore(flags);

  if (privreq)
    {
      /* If endpoint 0, temporarily reflect the state of protocol stalled
       * in the callback.
       */

      boolean stalled = privep->stalled;
      if (USB_EPNO(privep->ep.eplog) == EP0)
        {
          privep->stalled = (privep->dev->devstate == DEVSTATE_STALLED);
        }

      /* Save the result in the request structure */

      privreq->req.result = result;

      /* Callback to the request completion handler */

      privreq->flink = NULL;
      privreq->req.callback(&privep->ep, &privreq->req);

      /* Restore the stalled indication */

      privep->stalled = stalled;
    }
}

/****************************************************************************
 * Name: tm32_epwrite
 ****************************************************************************/

static void stm32_epwrite(struct stm32_usbdev_s *priv,
                          struct stm32_ep_s *privep,
                          const ubyte *buf, uint32 nbytes)
{
  ubyte epno = USB_EPNO(privep->ep.eplog);
  usbtrace(TRACE_WRITE(epno), nbytes);

  /* Check for NULL packet */

  if (nbytes > 0)
    {
      /* Copy the data from the user buffer into packet memory for this
       * endpoint
       */

      stm32_copytopma(buf, (uint16)STM32_USB_ADDR_RX(epno), nbytes);
    }

  /* Send the packet (might be a null packet nbytes == 0) */

  stm32_epsettxcount(epno, nbytes);
  priv->txstatus = USB_EPR_STATTX_VALID;

  /* Indicate that there is data in the TX packet memory.  This will be cleared
   * when the next data out interrupt is received.
   */

  privep->txbusy = 1;
  priv->devstate = DEVSTATE_WRREQUEST;
}

/****************************************************************************
 * Name: stm32_wrrequest
 ****************************************************************************/

static int stm32_wrrequest(struct stm32_usbdev_s *priv, struct stm32_ep_s *privep)
{
  struct stm32_req_s *privreq;
  ubyte *buf;
  ubyte epno;
  int nbytes;
  int bytesleft;

  /* We get here when an IN endpoint interrupt occurs.  So now we know that
   * there is no TX transfer in progress.
   */
  
  privep->txbusy = 0;

  /* Check the request from the head of the endpoint request queue */

  privreq = stm32_rqpeek(privep);
  if (!privreq)
    {
      /* There is no TX transfer in progress and no new pending TX
       * requests to send... STALL the TX status.
       */

      usbtrace(TRACE_INTDECODE(STM32_TRACEINTID_EPINQEMPTY), 0);
      priv->devstate = DEVSTATE_IDLE;
      priv->txstatus = USB_EPR_STATTX_STALL;
      return OK;
    }

  epno = USB_EPNO(privep->ep.eplog);
  ullvdbg("epno=%d req=%p: len=%d xfrd=%d nullpkt=%d\n",
          epno, privreq, privreq->req.len, privreq->req.xfrd, privep->txnullpkt);

  /* Get the number of bytes left to be sent in the packet */

  bytesleft         = privreq->req.len - privreq->req.xfrd;
  nbytes            = bytesleft;

#warning "REVISIT: If the EP supports double buffering, then we can do better"

  /* Send the next packet */

  if (nbytes > 0)
    {
      /* Either send the maxpacketsize or all of the remaining data in
       * the request.
       */

      privep->txnullpkt = 0;
      if (nbytes >= privep->ep.maxpacket)
        {
          nbytes =  privep->ep.maxpacket;

          /* Handle the case where this packet is exactly the
	   * maxpacketsize.  Do we need to send a NULL packet
	   * in this case?
           */

          if (bytesleft ==  privep->ep.maxpacket &&
             (privreq->req.flags & USBDEV_REQFLAGS_NULLPKT) != 0)
            {
              privep->txnullpkt = 1;
            }
        }
    }

  /* Send the packet (might be a null packet nbytes == 0) */

  buf = privreq->req.buf + privreq->req.xfrd;
  stm32_epwrite(priv, privep, buf, nbytes);

  /* Update for the next data IN interrupt */

  privreq->req.xfrd += nbytes;
  bytesleft          = privreq->req.len - privreq->req.xfrd;

  /* If all of the bytes were sent (including any final null packet)
   * then we are finished with the transfer
   */

  if (bytesleft == 0 && !privep->txnullpkt)
    {
      usbtrace(TRACE_COMPLETE(USB_EPNO(privep->ep.eplog)), privreq->req.xfrd);
      privep->txnullpkt = 0;
      stm32_reqcomplete(privep, OK);
    }

  return OK;
}

/****************************************************************************
 * Name: stm32_rdrequest
 ****************************************************************************/

static int stm32_rdrequest(struct stm32_usbdev_s *priv, struct stm32_ep_s *privep)
{
  struct stm32_req_s *privreq;
  ubyte *buf;
  int readlen;

  /* Check the request from the head of the endpoint request queue */

  privreq = stm32_rqpeek(privep);
  if (!privreq)
    {
      /* Incoming data available in PMA, but no packet to receive the data.
       * Mark that the RX data is pending and hope that a packet is returned
       * soon.
       */

      usbtrace(TRACE_INTDECODE(STM32_TRACEINTID_EPOUTQEMPTY), 0);
      priv->rxpending = TRUE;
      return OK;
    }

  ullvdbg("len=%d xfrd=%d nullpkt=%d\n",
          privreq->req.len, privreq->req.xfrd, privep->txnullpkt);

  /* Ignore any attempt to receive a zero length packet */

  if (privreq->req.len == 0)
    {
      usbtrace(TRACE_DEVERROR(STM32_TRACEERR_EPOUTNULLPACKET), 0);
      stm32_reqcomplete(privep, OK);
      return OK;
    }

  usbtrace(TRACE_READ(USB_EPNO(privep->ep.eplog)), privreq->req.xfrd);

  /* Receive the next packet */

  buf        = privreq->req.buf + privreq->req.xfrd;
  readlen    = MIN(privreq->req.len,  privep->ep.maxpacket);
  stm32_copyfrompma(buf, (uint16)STM32_USB_ADDR_TX(EP0), readlen);

  /* If the receive buffer is full then we are finished with the transfer */

  privreq->req.xfrd += readlen;
  if (privreq->req.xfrd >= privreq->req.len)
    {
      usbtrace(TRACE_COMPLETE(USB_EPNO(privep->ep.eplog)), privreq->req.xfrd);
      priv->devstate = DEVSTATE_IDLE;
      priv->rxstatus = USB_EPR_STATRX_VALID; /* Re-enable for next data reception */
      stm32_reqcomplete(privep, OK);
    }

  return OK;
}

/****************************************************************************
 * Name: stm32_cancelrequests
 ****************************************************************************/

static void stm32_cancelrequests(struct stm32_ep_s *privep)
{
  while (!stm32_rqempty(privep))
    {
      usbtrace(TRACE_COMPLETE(USB_EPNO(privep->ep.eplog)),
               (stm32_rqpeek(privep))->req.xfrd);
      stm32_reqcomplete(privep, -ESHUTDOWN);
    }
}

/****************************************************************************
 * Interrupt Level Processing
 ****************************************************************************/
/****************************************************************************
 * Name: stm32_dispatchrequest
 ****************************************************************************/

static int stm32_dispatchrequest(struct stm32_usbdev_s *priv)
{
  int ret = OK;

  usbtrace(TRACE_INTDECODE(STM32_TRACEINTID_DISPATCH), 0);
  if (priv && priv->driver)
    {
      /* Forward to the control request to the class driver implementation */

      ret = CLASS_SETUP(priv->driver, &priv->usbdev, &priv->ctrl);
      if (ret < 0)
        {
          /* Stall on failure */

          usbtrace(TRACE_DEVERROR(STM32_TRACEERR_DISPATCHSTALL), 0);
          priv->devstate = DEVSTATE_STALLED;
        }
    }
  return ret;
}

/****************************************************************************
 * Name: stm32_ep0post
 ****************************************************************************/

static void stm32_ep0post(struct stm32_usbdev_s *priv)
{
  stm32_epsetrxcount(EP0, STM32_EP0MAXPACKET);
  if (priv->devstate == DEVSTATE_STALLED)
    {
      priv->rxstatus = USB_EPR_STATRX_STALL;
      priv->txstatus = USB_EPR_STATTX_STALL;
    }
}

/****************************************************************************
 * Name: stm32_ep0setup
 ****************************************************************************/

static void stm32_ep0setup(struct stm32_usbdev_s *priv)
{
  struct stm32_ep_s   *ep0     = &priv->eplist[EP0];
  struct stm32_req_s  *privreq = stm32_rqpeek(ep0);
  struct stm32_ep_s   *privep;
  union wb_u           value;
  union wb_u           index;
  union wb_u           len;
  union wb_u           response;
  boolean              handled = FALSE;
  ubyte               *buf;
  ubyte                epno;
  int                  nbytes = 0;
  int                  ret;

  /* Terminate any pending requests */

  while (!stm32_rqempty(ep0))
    {
      sint16 result = OK;
      if (privreq->req.xfrd != privreq->req.len)
        {
          result = -EPROTO;
        }

      usbtrace(TRACE_COMPLETE(ep0->ep.eplog), privreq->req.xfrd);
      stm32_reqcomplete(ep0, result);
    }

  /* Assume NOT stalled */

  ep0->stalled  = 0;

  /* Get a 32-bit PMA address */

  buf = (ubyte*)(STM32_USBCANRAM_BASE + ((uint16)STM32_USB_ADDR_RX(EP0) << 1));

  /* Extract the request from PMA */

  priv->ctrl.type     = *buf++;    /* bmRequestType */
  priv->ctrl.req      = *buf++;    /* bRequest */
  buf                += 2;         /* Skip for 32 bits addressing */
  priv->ctrl.value[0] = *buf++;   /* wValue */
  priv->ctrl.value[1] = *buf++;   /* "    " */
  buf                += 2;         /* Skip for 32 bits addressing */
  priv->ctrl.index[0] = *buf++;   /* wIndex */
  priv->ctrl.index[1] = *buf++;   /* "    " */
  buf                += 2;         /* Skip for 32 bits addressing */
  priv->ctrl.len[0]   = *buf++;   /* wLength */
  priv->ctrl.len[1]   = *buf++;   /* "     " */

  /* And extract the little-endian 16-bit values to host order */

  value.w = GETUINT16(priv->ctrl.value);
  index.w = GETUINT16(priv->ctrl.index);
  len.w   = GETUINT16(priv->ctrl.len);

  ullvdbg("type=%02x req=%02x value=%04x index=%04x len=%04x\n",
          priv->ctrl.type, priv->ctrl.req, value.w, index.w, len.w);

  priv->devstate = DEVSTATE_INIT;

  /* Dispatch any non-standard requests */

  if ((priv->ctrl.type & USB_REQ_TYPE_MASK) != USB_REQ_TYPE_STANDARD)
    {
      usbtrace(TRACE_INTDECODE(STM32_TRACEINTID_NOSTDREQ), priv->ctrl.type);

      /* Let the class implementation handle all non-standar requests */

      if (stm32_dispatchrequest(priv) == OK)
        {
          /* stm32_dispatchrequest will return OK if the class implementation
           * handled the request and will request a stall if the class
           * implementation failed to handle the request.
           */

          handled = TRUE;
        }
    }

  /* Handle standard request.  Pick off the things of interest to the
   * USB device controller driver; pass what is left to the class driver
   */

  switch (priv->ctrl.req)
    {
    case USB_REQ_GETSTATUS:
      {
        /* type:  device-to-host; recipient = device, interface, endpoint
         * value: 0
         * index: zero interface endpoint
         * len:   2; data = status
         */

        usbtrace(TRACE_INTDECODE(STM32_TRACEINTID_GETSTATUS), priv->ctrl.type);
        if (len.w != 2      || (priv->ctrl.type & USB_REQ_DIR_IN) == 0 ||
            index.b[0] != 0 || value.w != 0)
          {
            usbtrace(TRACE_DEVERROR(STM32_TRACEERR_BADEPGETSTATUS), 0);
            priv->devstate = DEVSTATE_STALLED;
          }
        else
          {
            switch (priv->ctrl.type & USB_REQ_RECIPIENT_MASK)
              {
               case USB_REQ_RECIPIENT_ENDPOINT:
                {
                  epno = USB_EPNO(index.b[1]);
                  usbtrace(TRACE_INTDECODE(STM32_TRACEINTID_EPGETSTATUS), epno);
                  if (epno >= STM32_NENDPOINTS)
                    {
                      usbtrace(TRACE_DEVERROR(STM32_TRACEERR_BADEPGETSTATUS), epno);
                      priv->devstate = DEVSTATE_STALLED;
                    }
                  else
                    {
                      privep     = &priv->eplist[epno];
                      response.w = 0; /* Not stalled */
                      nbytes     = 2; /* Response size: 2 bytes */

                      if (USB_ISEPIN(index.b[1]))
                        {
                          /* IN endpoint */ 

                          if (stm32_eptxstalled(epno))
                            {
                              /* IN Endpoint stalled */

                              response.b[0] = 1; /* Stalled */
                            }
                          }
                      else
                        {
                          /* OUT endpoint */ 

                          if (stm32_eprxstalled(epno))
                            {
                              /*OUT Endpoint stalled */

                              response.b[0] |= 1; /* Stalled */
                            }
                        }
                    }
                }
                break;

              case USB_REQ_RECIPIENT_DEVICE:
                {
                 if (index.w == 0)
                    {
                      usbtrace(TRACE_INTDECODE(STM32_TRACEINTID_DEVGETSTATUS), 0);

                      /* Features:  Remote Wakeup=YES; selfpowered=? */

                      response.w    = 0;
                      response.b[0] = (priv->selfpowered << USB_FEATURE_SELFPOWERED) |
                                      (1 << USB_FEATURE_REMOTEWAKEUP);
                      nbytes        = 2; /* Response size: 2 bytes */
                    }
                  else
                    {
                      usbtrace(TRACE_DEVERROR(STM32_TRACEERR_BADDEVGETSTATUS), 0);
                      priv->devstate = DEVSTATE_STALLED;
                    }
                }
                break;

              case USB_REQ_RECIPIENT_INTERFACE:
                {
                  usbtrace(TRACE_INTDECODE(STM32_TRACEINTID_IFGETSTATUS), 0);
                  response.w = 0;
                  nbytes     = 2; /* Response size: 2 bytes */
                }
                break;

              default:
                {
                  usbtrace(TRACE_DEVERROR(STM32_TRACEERR_BADGETSTATUS), 0);
                  priv->devstate = DEVSTATE_STALLED;
                }
                break;
              }
          }
      }
      break;

    case USB_REQ_CLEARFEATURE:
      {
        /* type:  host-to-device; recipient = device, interface or endpoint
         * value: feature selector
         * index: zero interface endpoint;
         * len:   zero, data = none
         */

        usbtrace(TRACE_INTDECODE(STM32_TRACEINTID_CLEARFEATURE), priv->ctrl.type);
        if ((priv->ctrl.type & USB_REQ_RECIPIENT_MASK) != USB_REQ_RECIPIENT_ENDPOINT)
          {
            /* Let the class implementation handle all recipients (except for the
             * endpoint recipient)
             */

            if (stm32_dispatchrequest(priv) == OK)
              {
                /* stm32_dispatchrequest will return OK if the class implementation
                 * handled the request and will request a stall if the class
                 * implementation failed to handle the request.
                 */

                handled = TRUE;
              }
          }
        else
          {
            /* Endpoint recipient */

            epno = USB_EPNO(index.b[1]);
            if (epno < STM32_NENDPOINTS && index.b[0] == 0 &&
                value.w == USB_FEATURE_ENDPOINTHALT && len.w == 0)
              {
                privep         = &priv->eplist[epno];
                privep->halted = 0;
                ret            = stm32_epstall(&privep->ep, TRUE);
              }
            else
              {
                usbtrace(TRACE_DEVERROR(STM32_TRACEERR_BADCLEARFEATURE), 0);
                priv->devstate = DEVSTATE_STALLED;
              }
          }
      }
      break;

    case USB_REQ_SETFEATURE:
      {
        /* type:  host-to-device; recipient = device, interface, endpoint
         * value: feature selector
         * index: zero interface endpoint;
         * len:   0; data = none
         */

        usbtrace(TRACE_INTDECODE(STM32_TRACEINTID_SETFEATURE), priv->ctrl.type);
        if (((priv->ctrl.type & USB_REQ_RECIPIENT_MASK) == USB_REQ_RECIPIENT_DEVICE) &&
            value.w == USB_FEATURE_TESTMODE)
          {
            /* Special case recipient=device test mode */

            ullvdbg("test mode: %d\n", index.w);
          }
        else if ((priv->ctrl.type & USB_REQ_RECIPIENT_MASK) != USB_REQ_RECIPIENT_ENDPOINT)
          {
            /* The class driver handles all recipients except recipient=endpoint */

            if (stm32_dispatchrequest(priv) == OK)
              {
                /* stm32_dispatchrequest will return OK if the class implementation
                 * handled the request and will request a stall if the class
                 * implementation failed to handle the request.
                 */

                handled = TRUE;
              }
          }
        else
          {
            /* Handler recipient=endpoint */

            epno = USB_EPNO(index.b[1]);
            if (epno < STM32_NENDPOINTS && index.b[0] == 0 &&
                value.w == USB_FEATURE_ENDPOINTHALT && len.w == 0)
              {
                privep         = &priv->eplist[epno];
                privep->halted = 1;
                ret            = stm32_epstall(&privep->ep, FALSE);
              }
            else
              {
                usbtrace(TRACE_DEVERROR(STM32_TRACEERR_BADSETFEATURE), 0);
                priv->devstate = DEVSTATE_STALLED;
              }
          }
      }
      break;

    case USB_REQ_SETADDRESS:
      {
        /* type:  host-to-device; recipient = device
         * value: device address
         * index: 0
         * len:   0; data = none
         */

        usbtrace(TRACE_INTDECODE(STM32_TRACEINTID_EP0SETUPSETADDRESS), value.w);
        if ((priv->ctrl.type & USB_REQ_RECIPIENT_MASK) != USB_REQ_RECIPIENT_DEVICE ||
            index.w  != 0 || len.w != 0 || value.b[1] > 127 || value.b[0] != 0)
          {
            usbtrace(TRACE_DEVERROR(STM32_TRACEERR_BADSETADDRESS), 0);
            priv->devstate = DEVSTATE_STALLED;
          }
      }
      break;

    case USB_REQ_GETDESCRIPTOR:
      /* type:  device-to-host; recipient = device
       * value: descriptor type and index
       * index: 0 or language ID;
       * len:   descriptor len; data = descriptor
       */
    case USB_REQ_SETDESCRIPTOR:
      /* type:  host-to-device; recipient = device
       * value: descriptor type and index
       * index: 0 or language ID;
       * len:   descriptor len; data = descriptor
       */

      {
        usbtrace(TRACE_INTDECODE(STM32_TRACEINTID_GETSETDESC), priv->ctrl.type);
        if ((priv->ctrl.type & USB_REQ_RECIPIENT_MASK) == USB_REQ_RECIPIENT_DEVICE)
          {
            /* The request seems valid... let the class implementation handle it */

            if (stm32_dispatchrequest(priv) == OK)
              {
                /* stm32_dispatchrequest will return OK if the class implementation
                 * handled the request and will request a stall if the class
                 * implementation failed to handle the request.
                 */

                handled = TRUE;
              }
          }
        else
          {
            usbtrace(TRACE_DEVERROR(STM32_TRACEERR_BADGETSETDESC), 0);
            priv->devstate = DEVSTATE_STALLED;
          }
      }
      break;

    case USB_REQ_GETCONFIGURATION:
      /* type:  device-to-host; recipient = device
       * value: 0;
       * index: 0;
       * len:   1; data = configuration value
       */

      {
        usbtrace(TRACE_INTDECODE(STM32_TRACEINTID_GETCONFIG), priv->ctrl.type);
        if ((priv->ctrl.type & USB_REQ_RECIPIENT_MASK) == USB_REQ_RECIPIENT_DEVICE &&
            value.w == 0 && index.w == 0 && len.w == 1)
          {
            /* The request seems valid... let the class implementation handle it */

            if (stm32_dispatchrequest(priv) == OK)
              {
                /* stm32_dispatchrequest will return OK if the class implementation
                 * handled the request and will request a stall if the class
                 * implementation failed to handle the request.
                 */

                handled = TRUE;
              }
          }
        else
          {
            usbtrace(TRACE_DEVERROR(STM32_TRACEERR_BADGETCONFIG), 0);
            priv->devstate = DEVSTATE_STALLED;
          }
      }
      break;

    case USB_REQ_SETCONFIGURATION:
      /* type:  host-to-device; recipient = device
       * value: configuration value
       * index: 0;
       * len:   0; data = none
       */

      {
        usbtrace(TRACE_INTDECODE(STM32_TRACEINTID_SETCONFIG), priv->ctrl.type);
        if ((priv->ctrl.type & USB_REQ_RECIPIENT_MASK) == USB_REQ_RECIPIENT_DEVICE &&
            index.w == 0 && len.w == 0)
          {
             /* The request seems valid... let the class implementation handle it */

           if (stm32_dispatchrequest(priv) == OK)
              {
                /* stm32_dispatchrequest will return OK if the class implementation
                 * handled the request and will request a stall if the class
                 * implementation failed to handle the request.
                 */

                handled = TRUE;
              }
          }
        else
          {
            usbtrace(TRACE_DEVERROR(STM32_TRACEERR_BADSETCONFIG), 0);
            priv->devstate = DEVSTATE_STALLED;
          }
      }
      break;

    case USB_REQ_GETINTERFACE:
      /* type:  device-to-host; recipient = interface
       * value: 0
       * index: interface;
       * len:   1; data = alt interface
       */
    case USB_REQ_SETINTERFACE:
      /* type:  host-to-device; recipient = interface
       * value: alternate setting
       * index: interface;
       * len:   0; data = none
       */

      {
        /* Let the class implementation handle the request */

        usbtrace(TRACE_INTDECODE(STM32_TRACEINTID_GETSETIF), priv->ctrl.type);
        if (stm32_dispatchrequest(priv) == OK)
          {
            /* stm32_dispatchrequest will return OK if the class implementation
             * handled the request and will request a stall if the class
             * implementation failed to handle the request.
             */

            handled = TRUE;
          }
      }
      break;

    case USB_REQ_SYNCHFRAME:
      /* type:  device-to-host; recipient = endpoint
       * value: 0
       * index: endpoint;
       * len:   2; data = frame number
       */

      {
        usbtrace(TRACE_INTDECODE(STM32_TRACEINTID_SYNCHFRAME), 0);
      }
      break;

    default:
      {
        usbtrace(TRACE_DEVERROR(STM32_TRACEERR_INVALIDCTRLREQ), priv->ctrl.req);
        priv->devstate = DEVSTATE_STALLED;
      }
      break;
    }

  /* At this point, the request has been handled and there are three possible
   * outcomes:
   *
   * 1. The setup request was successfully handled above and a response packet
   *    must be sent (may be a zero length packet).
   * 2. The request was successfully handled by the class implementation.  In
   *    case, the response has already been sent and the local variable 'handled'
   *    will be set to TRUE;
   * 3. An error was detected in either the above logic or by the class implementation
   *    logic.  In either case, priv->state will be set DEVSTATE_STALLED
   *    to indicate this case.
   */

  if (priv->devstate == DEVSTATE_STALLED)
    {
      usbtrace(TRACE_DEVERROR(STM32_TRACEERR_EP0SETUPSTALLED), priv->devstate);
      stm32_epstall(priv->usbdev.ep0, FALSE);
      stm32_epstall(priv->usbdev.ep0, FALSE);
    }
  else if ((priv->ctrl.type & USB_REQ_DIR_IN) != 0)
    {
      if (!handled)
        {
          /* Restrict the data length to requested length */

          if (nbytes > len.w)
            {
              nbytes = len.w;
            }

          /* Send the response (might be a zero-length packet) */

          stm32_epwrite(priv, ep0, response.b, nbytes);
        }
    }
  else
    {
      /* Setup for next data reception */

      priv->devstate = DEVSTATE_IDLE;
      priv->rxstatus = USB_EPR_STATRX_VALID;
    }

  stm32_ep0post(priv);
}

/****************************************************************************
 * Name: stm32_ep0in
 ****************************************************************************/

static void stm32_ep0in(struct stm32_usbdev_s *priv)
{
  uint32 devstate = priv->devstate;
  if (priv->devstate == DEVSTATE_WRREQUEST)
    {
       stm32_wrrequest(priv, &priv->eplist[EP0]);
       devstate = priv->devstate;
    }
  else if (devstate == DEVSTATE_IDLE)
    {
      if (priv->ctrl.req == USB_REQ_SETADDRESS && 
          (priv->ctrl.type & REQRECIPIENT_MASK) == (USB_REQ_TYPE_STANDARD | USB_REQ_RECIPIENT_DEVICE))
        {
          union wb_u value;
          value.w = GETUINT16(priv->ctrl.value);
          stm32_setdevaddr(priv, value.b[1]);
        }

      devstate = DEVSTATE_STALLED;
    }
  else
    {
      devstate = DEVSTATE_STALLED;
    }

  priv->devstate = devstate;
  stm32_ep0post(priv);
}

/****************************************************************************
 * Name: stm32_ep0out
 ****************************************************************************/

static void stm32_ep0out(struct stm32_usbdev_s *priv, struct stm32_ep_s *privep)
{
  switch (priv->devstate)
    {
      case DEVSTATE_RDREQUEST:  /* Write request in progress */
      case DEVSTATE_IDLE:       /* No transfer in progress */
        stm32_rdrequest(priv, privep);
        break;

      default:
        /* Unexpected state OR host aborted the OUT transfer before it
         * completed, STALL the endpoint in either case
         */

        priv->devstate = DEVSTATE_STALLED;
        break;
    }

  stm32_ep0post(priv);
}

/****************************************************************************
 * Name: stm32_setdevaddr
 ****************************************************************************/

static void stm32_setdevaddr(struct stm32_usbdev_s *priv, ubyte value) 
{
  int epno;
  
  /* Set address in every allocated endpoint */

  for (epno = 0; epno < STM32_NENDPOINTS; epno++)
    {
      if (stm32_epreserved(priv, epno))
        {
          stm32_setepaddress((ubyte)epno, (ubyte)epno);
        }
    }

  /* Set the device address and enable function */

  stm32_putreg(value|USB_DADDR_EF, STM32_USB_DADDR);
}

/****************************************************************************
 * Name: stm32_lptransfer
 ****************************************************************************/

static void stm32_lptransfer(struct stm32_usbdev_s *priv) 
{
  struct stm32_ep_s *privep;
  ubyte  epno;
  uint16 epval;
  uint16 istr;

  /* Etay in loop while pending ints */

  while (((istr = stm32_getreg(STM32_USB_ISTR)) & USB_ISTR_CTR) != 0)
    {
      stm32_putreg((uint16)~USB_ISTR_CTR, STM32_USB_ISTR);    /* clear CTR flag */

      /* Extract highest priority endpoint number */ 

      epno   = (ubyte)(istr & USB_ISTR_EPID_MASK);
      privep = &priv->eplist[epno];

      if (epno == 0)
        {
          /* Decode and service control endpoint interrupt */ 
            
          /* Save RX & TX status */ 

          priv->rxstatus = stm32_geteprxstatus(EP0);
          priv->txstatus = stm32_geteptxstatus(EP0);

          /* Then set both to NAK */ 

          stm32_seteprxstatus(EP0, USB_EPR_STATRX_NAK);
          stm32_seteptxstatus(EP0, USB_EPR_STATTX_NAK);
          
          /* DIR bit = origin of the interrupt */ 

          if ((istr & USB_ISTR_DIR) == 0)
            {
              /* DIR = 0 => IN int */ 
              /* DIR = 0 implies that (USB_EPR_CTR_TX = 1) always */ 

              stm32_clrepctrtx(EP0);
              stm32_ep0in(priv);
              
              /* Set Tx & Rx status */ 

              stm32_seteprxstatus(EP0, priv->rxstatus);
              stm32_seteptxstatus(EP0, priv->txstatus);
              return;
            }
          else
            {
              /* DIR = 1 */ 
              /* DIR = 1 & CTR_RX => SETUP or OUT int */ 
              /* DIR = 1 & (CTR_TX | CTR_RX) => 2 int pending */ 

              epval = stm32_getreg(STM32_USB_EPR(EP0));
              if ((epval & USB_EPR_CTR_TX) != 0)
                {
                  stm32_clrepctrtx(EP0);
                  stm32_ep0in(priv);

                  /* Set Tx & Rx status */ 

                  stm32_seteprxstatus(EP0, priv->rxstatus);
                  stm32_seteptxstatus(EP0, priv->txstatus);
                  return;
                }
              else if ((epval & USB_EPR_SETUP) != 0)
                {
                  stm32_clrepctrrx(EP0); /* SETUP bit kept frozen while CTR_RX=1 */
                  stm32_ep0setup(priv);

                  /* Set Tx & Rx status */ 

                  stm32_seteprxstatus(EP0, priv->rxstatus);
                  stm32_seteptxstatus(EP0, priv->txstatus);
                  return;
                }
              else if ((epval & USB_EPR_CTR_RX) != 0)
                {
                  stm32_clrepctrrx(EP0);
                  stm32_ep0out(priv, privep);

                  /* Set Tx & Rx status */ 

                  stm32_seteprxstatus(EP0, priv->rxstatus);
                  stm32_seteptxstatus(EP0, priv->txstatus);
                  return;
                }
            }
        }
      else
        {
          /* Decode and service non control endpoints interrupt */ 
          /* process related endpoint register */ 

          epval = stm32_getreg(STM32_USB_EPR(epno));
          if ((epval & USB_EPR_CTR_RX) != 0)
            {
              /* OUT: host-to-device */
              /* Clear interrupt status */

              stm32_clrepctrrx(epno);

              /* Handle read requests */

              usbtrace(TRACE_INTDECODE(STM32_TRACEINTID_EPOUT), (uint16)epno);

              /* Read host data into the current read request */

              if (!stm32_rqempty(privep))
                {
                  stm32_rdrequest(priv, privep);
                }
              else
                {
                  usbtrace(TRACE_INTDECODE(STM32_TRACEINTID_EPOUTPENDING), (uint16)epno);
                  priv->rxpending = 1;
                }
            }

          if ((epval & USB_EPR_CTR_TX) != 0)
            {
              /* IN: device-to-host */
              /* Clear interrupt status */

              stm32_clrepctrtx(epno);
          
              /* Handle write requests */ 

              usbtrace(TRACE_INTDECODE(STM32_TRACEINTID_EPIN), (uint16)epno); 
              privep->txbusy = 0;
              stm32_wrrequest(priv, privep);
            }
        }
    }
}

/****************************************************************************
 * Name: stm32_hpinterrupt
 ****************************************************************************/

static int stm32_hpinterrupt(int irq, void *context)
{
  /* For now there is only one USB controller, but we will always refer to
   * it using a pointer to make any future ports to multiple USB controllers
   * easier.
   */

  struct stm32_usbdev_s *priv = &g_usbdev;
  struct stm32_ep_s *privep;
  uint32 epval = 0;
  uint16 istr;
  ubyte  epno;

  /* High priority interrupts are only triggered by a correct transfer event
   * for isochronous and double-buffer bulk transfers.
   */

  usbtrace(TRACE_INTENTRY(STM32_TRACEINTID_HPINTERRUPT), irq);
  while (((istr = stm32_getreg(STM32_USB_ISTR)) & USB_ISTR_CTR) != 0)
    {
      stm32_putreg((uint16)~USB_ISTR_CTR, STM32_USB_ISTR);
      
      /* Extract highest priority endpoint number */ 

      epno   = (ubyte)(istr & USB_ISTR_EPID_MASK);
      privep = &priv->eplist[epno];

      /* Process related endpoint register */ 

      epval  = stm32_getreg(STM32_USB_EPR(epno));
      if ((epval & USB_EPR_CTR_RX) != 0)
        {
          /* OUT: host-to-device */
          /* Clear interrupt status */

          stm32_clrepctrrx(epno);

          /* Handle read requests */

          usbtrace(TRACE_INTDECODE(STM32_TRACEINTID_EPOUT), (uint16)epno);

          /* Read host data into the current read request */

          if (!stm32_rqempty(privep))
            {
              stm32_rdrequest(priv, privep);
            }
          else
            {
              usbtrace(TRACE_INTDECODE(STM32_TRACEINTID_EPOUTPENDING), (uint16)epno);
              priv->rxpending = 1;
            }
        }
      else if ((epval & USB_EPR_CTR_TX) != 0)
        {
          /* IN: device-to-host */
          /* Clear interrupt status */

          stm32_clrepctrtx(epno);
          
          /* Handle write requests */ 

          usbtrace(TRACE_INTDECODE(STM32_TRACEINTID_EPIN), (uint16)epno); 
          privep->txbusy = 0;
          stm32_wrrequest(priv, privep);
        }
    }

  usbtrace(TRACE_INTEXIT(STM32_TRACEINTID_HPINTERRUPT), 0);
  return OK;
}

/****************************************************************************
 * Name: stm32_lpinterrupt
 ****************************************************************************/

static int stm32_lpinterrupt(int irq, void *context)
{
  /* For now there is only one USB controller, but we will always refer to
   * it using a pointer to make any future ports to multiple USB controllers
   * easier.
   */

  struct stm32_usbdev_s *priv = &g_usbdev;
  uint16 istr = stm32_getreg(STM32_USB_ISTR);

  usbtrace(TRACE_INTENTRY(STM32_TRACEINTID_LPINTERRUPT), irq);

  /* Handle Reset interrupts.  When this event occurs, the peripheral is left
   * in the same conditions it is left by the system reset (but with the
   * USB controller enabled).
   */

  if ((istr & USB_ISTR_RESET) != 0)
    {
      /* Wakeup interrupt received. Clear the WKUP interrupt status. */

      stm32_putreg(~USB_ISTR_RESET, STM32_USB_ISTR);
      usbtrace(TRACE_INTDECODE(STM32_TRACEINTID_RESET), 0);

      /* Restore our power-up state and exit now. */

      stm32_reset(priv);
      return OK;
    }

  /* Handle Wakeup interrupts.  This interrupt is only enable while the USB is
   * suspended.
   */

  if ((istr & USB_ISTR_WKUP & priv->imask) != 0)
    {
      /* Wakeup interrupt received. Clear the WKUP interrupt status. */

      stm32_putreg(~USB_ISTR_WKUP, STM32_USB_ISTR);
      usbtrace(TRACE_INTDECODE(STM32_TRACEINTID_WKUP), 0);

      /* Perform the wakeup action */

      stm32_initresume(priv);
      priv->rsmstate = RSMSTATE_IDLE;

      /* Disable ESOF polling, disable the wakeup interrupt, and
       * re-enable the suspend interrupt.
       */

      priv->imask &= ~(USB_CNTR_ESOFM|USB_CNTR_WKUPM);
      priv->imask |= USB_CNTR_SUSPM;
      stm32_putreg(priv->imask, STM32_USB_CNTR);

      /* Clear any pending interrupts that we just enabled */

      stm32_putreg(~USB_CNTR_SUSPM, STM32_USB_ISTR);
    }

  if ((istr & USB_ISTR_SUSP & priv->imask) != 0)
    {
        stm32_suspend(priv);

        /* Clear of the ISTR bit must be done after setting of USB_CNTR_FSUSP */ 

        stm32_putreg(~USB_ISTR_SUSP, STM32_USB_ISTR);
        usbtrace(TRACE_INTDECODE(STM32_TRACEINTID_SUSP), 0);
    }

  if ((istr & USB_ISTR_ESOF & priv->imask) != 0)
    {
      stm32_putreg(~USB_ISTR_ESOF, STM32_USB_ISTR);
      
      /* Resume handling timing is made with ESOFs */ 

      usbtrace(TRACE_INTDECODE(STM32_TRACEINTID_ESOF), 0);
      stm32_esofpoll(priv);
    }

  if ((istr & USB_ISTR_CTR & priv->imask) != 0)
    {
      /* Low priority endpoint correct transfer interrupt */ 

      usbtrace(TRACE_INTDECODE(STM32_TRACEINTID_LPCTR), 0);
      stm32_lptransfer(priv);
    }

  usbtrace(TRACE_INTEXIT(STM32_TRACEINTID_LPINTERRUPT), 0);
  return OK;
}

/****************************************************************************
 * Name: stm32_suspend
 ****************************************************************************/

static void stm32_suspend(struct stm32_usbdev_s *priv) 
{
  uint16 regval;
  
  /* Disable ESOF polling, disable the SUSP interrupt, and
   * enable the WKUP interrupt.
   */

  priv->imask &= ~(USB_CNTR_ESOFM|USB_CNTR_SUSPM);
  priv->imask |= USB_CNTR_WKUPM;
  stm32_putreg(priv->imask, STM32_USB_CNTR);

  /* Clear any pending interrupts that we just enabled */

  stm32_putreg(~USB_CNTR_WKUPM, STM32_USB_ISTR);

  /* Enter suspend mode */

  regval  = stm32_getreg(STM32_USB_CNTR);
  regval |= USB_CNTR_FSUSP;
  stm32_putreg(regval, STM32_USB_CNTR);

  /* Only works with bus-powered devices */
  /* Force low-power mode in the macrocell */

  regval = stm32_getreg(STM32_USB_CNTR);
  regval |= USB_CNTR_LPMODE;
  stm32_putreg(regval, STM32_USB_CNTR);
  
  /* Enter reduce-power consumption mode */ 

  stm32_usbsuspend((struct usbdev_s *)priv, FALSE);
} 

/****************************************************************************
 * Name: stm32_initresume
 ****************************************************************************/

static void stm32_initresume(struct stm32_usbdev_s *priv) 
{
  uint16 regval;

  /* Only works on bus-powered devices */
  /* USB_CNTR_LPMODE = 0 */ 

  regval = stm32_getreg(STM32_USB_CNTR);
  regval &= (~USB_CNTR_LPMODE);
  stm32_putreg(regval, STM32_USB_CNTR);
  
  /* Restore full power */ 

  stm32_usbsuspend((struct usbdev_s *)priv, TRUE);
  
  /* Reset FSUSP bit */

  stm32_putreg(STM32_CNTR_SETUP, STM32_USB_CNTR);
} 

/****************************************************************************
 * Name: stm32_esofpoll
 ****************************************************************************/

static void stm32_esofpoll(struct stm32_usbdev_s *priv) 
{
  uint16 regval;

    /* Called periodically from ESOF interrupt after RSMSTATE_STARTED */

  switch (priv->rsmstate)
    {
    /* One ESOF after internal resume requested */

    case RSMSTATE_STARTED:
      regval         = stm32_getreg(STM32_USB_CNTR);
      regval        |= USB_CNTR_RESUME;
      stm32_putreg(regval, STM32_USB_CNTR);
      priv->rsmstate = RSMSTATE_WAITING;
      priv->nesofs   = 10;
      break;

    /* Countdown before completing the operation */

    case RSMSTATE_WAITING:
      priv->nesofs--;
      if (priv->nesofs == 0)
        {
          /* Okay.. we are ready to resume normal operation */

          regval         = stm32_getreg(STM32_USB_CNTR);
          regval        &= (~USB_CNTR_RESUME);
          stm32_putreg(regval, STM32_USB_CNTR);
          priv->rsmstate = RSMSTATE_IDLE;

          /* Disable ESOF polling, disable the SUSP interrupt, and
           * enable the WKUP interrupt.
           */

          priv->imask &= ~(USB_CNTR_ESOFM|USB_CNTR_SUSPM);
          priv->imask |= USB_CNTR_WKUPM;
          stm32_putreg(priv->imask, STM32_USB_CNTR);

          /* Clear any pending interrupts that we just enabled */

          stm32_putreg(~USB_CNTR_WKUPM, STM32_USB_ISTR);
        }
      break;

    case RSMSTATE_IDLE:
    default:
      priv->rsmstate = RSMSTATE_IDLE;
      break;
    }
}

/****************************************************************************
 * Endpoint Helpers
 ****************************************************************************/
/****************************************************************************
 * Name: stm32_reserveep
 ****************************************************************************/

static inline struct stm32_ep_s *
stm32_reserveep(struct stm32_usbdev_s *priv, ubyte epset)
{
  struct stm32_ep_s *privep = NULL;
  irqstate_t flags;
  int epndx = 0;

  flags = irqsave();
  epset &= priv->epavail;
  if (epset)
    {
      /* Select the lowest bit in the set of matching, available endpoints
       * (skipping EP0)
       */

      for (epndx = 1; epndx < STM32_NENDPOINTS; epndx++)
        {
          ubyte bit = STM32_ENDP_BIT(epndx);
          if ((epset & bit) != 0)
            {
              /* Mark the endpoint no longer available */

              priv->epavail &= ~bit;

              /* And return the pointer to the standard endpoint structure */

              privep = &priv->eplist[epndx];
              break;
            }
        }
    }

  irqrestore(flags);
  return privep;
}

/****************************************************************************
 * Name: stm32_unreserveep
 ****************************************************************************/

static inline void
stm32_unreserveep(struct stm32_usbdev_s *priv, struct stm32_ep_s *privep)
{
  irqstate_t flags = irqsave();
  priv->epavail   |= STM32_ENDP_BIT(USB_EPNO(privep->ep.eplog));
  irqrestore(flags);
}

/****************************************************************************
 * Name: stm32_epreserved
 ****************************************************************************/

static inline boolean
stm32_epreserved(struct stm32_usbdev_s *priv, int epno)
{
  return ((priv->epavail & STM32_ENDP_BIT(epno)) == 0);
}

/****************************************************************************
 * Name: stm32_allocpma
 ****************************************************************************/

static int stm32_allocpma(struct stm32_usbdev_s *priv)
{
  irqstate_t flags;
  int bufno = ERROR;
  int bufndx;

  flags = irqsave();
  for (bufndx = 2; bufndx < STM32_NBUFFERS; bufndx++)
    {
      /* Check if this buffer is available */

      ubyte bit = STM32_BUFFER_BIT(bufndx);
      if ((priv->bufavail & bit) != 0)
        {
          /* Yes.. Mark the endpoint no longer available */

          priv->bufavail &= ~bit;

          /* And return the index of the allocated buffer */

          bufno = bufndx;
          break;
        }
    }

  irqrestore(flags);
  return bufno;
}

/****************************************************************************
 * Name: stm32_freepma
 ****************************************************************************/

static inline void
stm32_freepma(struct stm32_usbdev_s *priv, struct stm32_ep_s *privep)
{
  irqstate_t flags = irqsave();
  priv->epavail   |= STM32_ENDP_BIT(privep->bufno);
  irqrestore(flags);
}

/****************************************************************************
 * Endpoint operations
 ****************************************************************************/
/****************************************************************************
 * Name: stm32_epconfigure
 ****************************************************************************/

static int stm32_epconfigure(struct usbdev_ep_s *ep,
                             const struct usb_epdesc_s *desc,
                             boolean last)
{
  struct stm32_ep_s *privep = (struct stm32_ep_s *)ep;
  uint16 pma;
  uint16 setting;
  uint16 maxpacket;
  ubyte  epno;

#ifdef CONFIG_DEBUG
  if (!ep || !desc)
    {
      usbtrace(TRACE_DEVERROR(STM32_TRACEERR_INVALIDPARMS), 0);
      return -EINVAL;
    }
#endif

  /* Get the unadorned endpoint address */

  epno = USB_EPNO(desc->addr);
  usbtrace(TRACE_EPCONFIGURE, (uint16)epno);
  DEBUGASSERT(epno == USB_EPNO(ep->eplog));

  /* Set the requested type */

  switch (desc->type)
   {
    case USB_EP_ATTR_XFER_INT: /* Interrupt endpoint */
      setting = USB_EPR_EPTYPE_INTERRUPT;
      break;

    case USB_EP_ATTR_XFER_BULK: /* Bulk endpoint */
      setting = USB_EPR_EPTYPE_BULK;
      break;

    case USB_EP_ATTR_XFER_ISOC: /* Isochronous endpoint */
#warning "REVISIT: Need to review isochronous EP setup"
      setting = USB_EPR_EPTYPE_ISOC;
      break;

    case USB_EP_ATTR_XFER_CONTROL: /* Control endpoint */
      setting = USB_EPR_EPTYPE_CONTROL;
      break;

    default:
      usbtrace(TRACE_DEVERROR(STM32_TRACEERR_BADEPTYPE), (uint16)desc->type);
      return -EINVAL;
    }

  stm32_seteptype(epno, setting);

  /* Get the address of the PMA buffer allocated for this endpoint */

#warning "REVISIT: Should configure BULK EPs using double buffer feature"
  pma = STM32_BUFNO2BUF(privep->bufno);

  /* Get the maxpacket size of the endpoint. */

  maxpacket = GETUINT16(desc->mxpacketsize);
  DEBUGASSERT(maxpacket <= STM32_MAXPACKET_SIZE);
  ep->maxpacket = maxpacket;

  /* Get the subset matching the requested direction */

  if (USB_ISEPIN(desc->addr))
    {
      /* The full, logical EP number includes direction (which is zero
       * for IN endpoints.
       */
 
      ep->eplog = USB_EPIN(epno);

      /* Set up TX; disable RX */

      stm32_seteptxaddr(epno, pma);
      stm32_seteptxstatus(epno, USB_EPR_STATTX_NAK);
      stm32_seteprxstatus(epno, USB_EPR_STATRX_DIS);
    }
  else
    {
      /* The full, logical EP number includes direction */

      ep->eplog = USB_EPOUT(epno);

      /* Set up RX; disable TX */

      stm32_seteprxaddr(epno, pma);
      stm32_epsetrxcount(epno, maxpacket);
      stm32_seteprxstatus(epno, USB_EPR_STATRX_VALID);
      stm32_seteptxstatus(epno, USB_EPR_STATTX_DIS);
    }
   return OK;
}

/****************************************************************************
 * Name: stm32_epdisable
 ****************************************************************************/

static int stm32_epdisable(struct usbdev_ep_s *ep)
{
  struct stm32_ep_s *privep = (struct stm32_ep_s *)ep;
  irqstate_t flags;
  ubyte epno;

#ifdef CONFIG_DEBUG
  if (!ep)
    {
      usbtrace(TRACE_DEVERROR(STM32_TRACEERR_INVALIDPARMS), 0);
      return -EINVAL;
    }
#endif

  epno = USB_EPNO(ep->eplog);
  usbtrace(TRACE_EPDISABLE, epno);

  /* Cancel any ongoing activity */

  flags = irqsave();
  stm32_cancelrequests(privep);

  /* Disable TX; disable RX */

  stm32_epsetrxcount(epno, 0);
  stm32_seteprxstatus(epno, USB_EPR_STATRX_DIS);
  stm32_seteptxstatus(epno, USB_EPR_STATTX_DIS);

  irqrestore(flags);
  return OK;
}

/****************************************************************************
 * Name: stm32_epallocreq
 ****************************************************************************/

static struct usbdev_req_s *stm32_epallocreq(struct usbdev_ep_s *ep)
{
  struct stm32_req_s *privreq;

#ifdef CONFIG_DEBUG
  if (!ep)
    {
      usbtrace(TRACE_DEVERROR(STM32_TRACEERR_INVALIDPARMS), 0);
      return NULL;
    }
#endif
  usbtrace(TRACE_EPALLOCREQ, USB_EPNO(ep->eplog));

  privreq = (struct stm32_req_s *)malloc(sizeof(struct stm32_req_s));
  if (!privreq)
    {
      usbtrace(TRACE_DEVERROR(STM32_TRACEERR_ALLOCFAIL), 0);
      return NULL;
    }

  memset(privreq, 0, sizeof(struct stm32_req_s));
  return &privreq->req;
}

/****************************************************************************
 * Name: stm32_epfreereq
 ****************************************************************************/

static void stm32_epfreereq(struct usbdev_ep_s *ep, struct usbdev_req_s *req)
{
  struct stm32_req_s *privreq = (struct stm32_req_s*)req;

#ifdef CONFIG_DEBUG
  if (!ep || !req)
    {
      usbtrace(TRACE_DEVERROR(STM32_TRACEERR_INVALIDPARMS), 0);
      return;
    }
#endif
  usbtrace(TRACE_EPFREEREQ, USB_EPNO(ep->eplog));

  free(privreq);
}

/****************************************************************************
 * Name: stm32_epsubmit
 ****************************************************************************/

static int stm32_epsubmit(struct usbdev_ep_s *ep, struct usbdev_req_s *req)
{
  struct stm32_req_s *privreq = (struct stm32_req_s *)req;
  struct stm32_ep_s *privep = (struct stm32_ep_s *)ep;
  struct stm32_usbdev_s *priv;
  irqstate_t flags;
  int ret = OK;

#ifdef CONFIG_DEBUG
  if (!req || !req->callback || !req->buf || !ep)
    {
      usbtrace(TRACE_DEVERROR(STM32_TRACEERR_INVALIDPARMS), 0);
      ullvdbg("req=%p callback=%p buf=%p ep=%p\n", req, req->callback, req->buf, ep);
      return -EINVAL;
    }
#endif

  usbtrace(TRACE_EPSUBMIT, USB_EPNO(ep->eplog));
  priv = privep->dev;

  if (!priv->driver || priv->usbdev.speed == USB_SPEED_UNKNOWN)
    {
      usbtrace(TRACE_DEVERROR(STM32_TRACEERR_NOTCONFIGURED), priv->usbdev.speed);
      return -ESHUTDOWN;
    }

  /* Handle the request from the class driver */

  req->result = -EINPROGRESS;
  req->xfrd   = 0;
  flags       = irqsave();

  /* If we are stalled, then drop all requests on the floor */

  if (privep->stalled)
    {
      stm32_abortrequest(privep, privreq, -EBUSY);
      ret = -EBUSY;
    }

  /* Handle IN (device-to-host) requests */

  else if (USB_ISEPIN(ep->eplog))
    {
      /* Add the new request to the request queue for the IN endpoint */

      stm32_rqenqueue(privep, privreq);
      usbtrace(TRACE_INREQQUEUED(USB_EPNO(ep->eplog)), req->len);

      /* If the IN endpoint FIFO is available, then transfer the data now */

      if (privep->txbusy == 0)
        {
          ret = stm32_wrrequest(priv, privep);
        }
    }

  /* Handle OUT (host-to-device) requests */

  else
    {
      /* Add the new request to the request queue for the OUT endpoint */

      privep->txnullpkt = 0;
      stm32_rqenqueue(privep, privreq);
      usbtrace(TRACE_OUTREQQUEUED(USB_EPNO(ep->eplog)), req->len);

      /* This there a incoming data pending the availability of a request? */

      if (priv->rxpending)
        {
          ret = stm32_rdrequest(priv, privep);
          priv->rxpending = 0;
        }
    }

  irqrestore(flags);
  return ret;
}

/****************************************************************************
 * Name: stm32_epcancel
 ****************************************************************************/

static int stm32_epcancel(struct usbdev_ep_s *ep, struct usbdev_req_s *req)
{
  struct stm32_ep_s *privep = (struct stm32_ep_s *)ep;
  struct stm32_usbdev_s *priv;
  irqstate_t flags;

#ifdef CONFIG_DEBUG
  if (!ep || !req)
    {
      usbtrace(TRACE_DEVERROR(STM32_TRACEERR_INVALIDPARMS), 0);
      return -EINVAL;
    }
#endif
  usbtrace(TRACE_EPCANCEL, USB_EPNO(ep->eplog));
  priv = privep->dev;

  flags = irqsave();
  stm32_cancelrequests(privep);
  irqrestore(flags);
  return OK;
}

/****************************************************************************
 * Name: stm32_epstall
 ****************************************************************************/

static int stm32_epstall(struct usbdev_ep_s *ep, boolean resume)
{
  struct stm32_ep_s *privep;
  struct stm32_usbdev_s *priv;
  ubyte epno = USB_EPNO(ep->eplog);
  uint16 status;
  irqstate_t flags;

#ifdef CONFIG_DEBUG
  if (!ep)
    {
      usbtrace(TRACE_DEVERROR(STM32_TRACEERR_INVALIDPARMS), 0);
      return -EINVAL;
    }
#endif
  privep = (struct stm32_ep_s *)ep;
  priv   = (struct stm32_usbdev_s *)privep->dev;
  epno   = USB_EPNO(ep->eplog);

  /* STALL or RESUME the endpoint */

  flags = irqsave();
  usbtrace(resume ? TRACE_EPRESUME : TRACE_EPSTALL, USB_EPNO(ep->eplog));

  /* Get status of the endpoint; stall the request if the endpoint is
   * disabled
   */
 
  if (USB_ISEPIN(ep->eplog))
    {
      status = stm32_geteptxstatus(epno);
    }
  else
    {
      status = stm32_geteprxstatus(epno);
    }

  if (status == 0)
    {
      usbtrace(TRACE_DEVERROR(STM32_TRACEERR_EPDISABLED), 0);
      priv->devstate = DEVSTATE_STALLED;
      return -ENODEV;
    }

  /* Handle the resume condition */

  if (resume)
    {
      /* Resuming a stalled endpoint */

      usbtrace(TRACE_EPRESUME, epno);
      privep->stalled = 0;

      if (USB_ISEPIN(ep->eplog))
        {
          /* IN endpoint */ 

          if (stm32_eptxstalled(epno))
            {
              stm32_clrtxdtog(epno);
              stm32_seteptxstatus(epno, USB_EPR_STATTX_VALID);
            }
        }
      else
        {
          /* OUT endpoint */ 

          if (stm32_eprxstalled(epno))
            {
              if (epno == EP0)
                {
                  /* After clear the STALL, enable the default endpoint receiver */

                  stm32_epsetrxcount(epno, ep->maxpacket);
                  stm32_seteprxstatus(epno, USB_EPR_STATRX_VALID);
                }
              else
                {
                  stm32_clrrxdtog(epno);
                  stm32_seteprxstatus(epno, USB_EPR_STATRX_VALID);
                }
            }
        }  
    }

  /* Handle the stall condition */

  else
    {
      usbtrace(TRACE_EPSTALL, epno);
      privep->stalled = 1;

      if (USB_ISEPIN(ep->eplog))
        {
            /* IN endpoint */ 

            stm32_seteptxstatus(epno, USB_EPR_STATTX_STALL);
        }
      else
        {
            /* OUT endpoint */ 

            stm32_seteprxstatus(epno, USB_EPR_STATRX_STALL);
        }

      /* Restart any queue write requests */

      if (resume)
        {
          (void)stm32_wrrequest(priv, privep);
        }
    }

  irqrestore(flags);
  return OK;
}

/****************************************************************************
 * Device Controller Operations
 ****************************************************************************/
/****************************************************************************
 * Name: stm32_allocep
 ****************************************************************************/

static struct usbdev_ep_s *stm32_allocep(struct usbdev_s *dev, ubyte epno,
                                         boolean in, ubyte eptype)
{
  struct stm32_usbdev_s *priv = (struct stm32_usbdev_s *)dev;
  struct stm32_ep_s *privep = NULL;
  ubyte epset = STM32_ENDP_ALLSET;
  int bufno;

  usbtrace(TRACE_DEVALLOCEP, (uint16)epno);
#ifdef CONFIG_DEBUG
  if (!dev)
    {
      usbtrace(TRACE_DEVERROR(STM32_TRACEERR_INVALIDPARMS), 0);
      return NULL;
    }
#endif

  /* Ignore any direction bits in the logical address */

  epno = USB_EPNO(epno);

  /* A logical address of 0 means that any endpoint will do */

  if (epno > 0)
    {
      /* Otherwise, we will return the endpoint structure only for the requested
       * 'logical' endpoint.  All of the other checks will still be performed.
       *
       * First, verify that the logical endpoint is in the range supported by
       * by the hardware.
       */

      if (epno >= STM32_NENDPOINTS)
        {
          usbtrace(TRACE_DEVERROR(STM32_TRACEERR_BADEPNO), (uint16)epno);
          return NULL;
        }

      /* Convert the logical address to a physical OUT endpoint address and
       * remove all of the candidate endpoints from the bitset except for the
       * the IN/OUT pair for this logical address.
       */

      epset = STM32_ENDP_BIT(epno);
    }

  /* Check if the selected endpoint number is available */

  privep = stm32_reserveep(priv, epset);
  if (!privep)
    {
      usbtrace(TRACE_DEVERROR(STM32_TRACEERR_EPRESERVE), (uint16)epset);
      goto errout;
    }
  epno = USB_EPNO(privep->ep.eplog);

  /* Allocate a PMA buffer for this endpoint */

#warning "REVISIT: Should configure BULK EPs using double buffer feature"
  bufno = stm32_allocpma(priv);
  if (bufno < 0)
    {
      usbtrace(TRACE_DEVERROR(STM32_TRACEERR_EPBUFFER), 0);
      goto errout_with_ep;
    }
  privep->bufno = (ubyte)bufno;
  return &privep->ep;

errout_with_ep:
  stm32_unreserveep(priv, privep);
errout:
  return NULL;
}

/****************************************************************************
 * Name: stm32_freeep
 ****************************************************************************/

static void stm32_freeep(struct usbdev_s *dev, struct usbdev_ep_s *ep)
{
  struct stm32_usbdev_s *priv;
  struct stm32_ep_s *privep;

#ifdef CONFIG_DEBUG
  if (!dev || !ep)
    {
      usbtrace(TRACE_DEVERROR(STM32_TRACEERR_INVALIDPARMS), 0);
      return;
    }
#endif
  priv   = (struct stm32_usbdev_s *)dev;
  privep = (struct stm32_ep_s *)ep;
  usbtrace(TRACE_DEVFREEEP, (uint16)USB_EPNO(ep->eplog));

  if (priv && privep)
    {
      /* Free the PMA buffer assigned to this endpoint */

      stm32_freepma(priv, privep);

      /* Mark the endpoint as available */

      stm32_unreserveep(priv, privep);
    }
}

/****************************************************************************
 * Name: stm32_getframe
 ****************************************************************************/

static int stm32_getframe(struct usbdev_s *dev)
{
  uint16 fnr;

#ifdef CONFIG_DEBUG
  if (!dev)
    {
      usbtrace(TRACE_DEVERROR(STM32_TRACEERR_INVALIDPARMS), 0);
      return -EINVAL;
    }
#endif

  /* Return the last frame number detected by the hardware */

  fnr = stm32_getreg(STM32_USB_FNR);
  usbtrace(TRACE_DEVGETFRAME, fnr);
  return (fnr & USB_FNR_FN_MASK);
}

/****************************************************************************
 * Name: stm32_wakeup
 ****************************************************************************/

static int stm32_wakeup(struct usbdev_s *dev)
{
  struct stm32_usbdev_s *priv = (struct stm32_usbdev_s *)dev;
  irqstate_t flags;

  usbtrace(TRACE_DEVWAKEUP, 0);
#ifdef CONFIG_DEBUG
  if (!dev)
    {
      usbtrace(TRACE_DEVERROR(STM32_TRACEERR_INVALIDPARMS), 0);
      return -EINVAL;
    }
#endif

  /* Start the resume sequence.  The actual resume steps will be driven
   * by the ESOF interrupt.
   */

  flags = irqsave();
  stm32_initresume(priv);
  priv->rsmstate = RSMSTATE_STARTED;

  /* Disable the SUSP interrupt (until we are fully resumed), disable
   * the WKUP interrupt (we are already waking up), and enable the
   * ESOF interrupt that will drive the resume operations.
   */

  priv->imask &= ~(USB_CNTR_WKUPM|USB_CNTR_SUSPM);
  priv->imask |= USB_CNTR_ESOFM;
  stm32_putreg(priv->imask, STM32_USB_CNTR);

  /* Clear any pending interrupts that we just enabled */

  stm32_putreg(~USB_CNTR_ESOFM, STM32_USB_ISTR);
  irqrestore(flags);
  return OK;
}

/****************************************************************************
 * Name: stm32_selfpowered
 ****************************************************************************/

static int stm32_selfpowered(struct usbdev_s *dev, boolean selfpowered)
{
  struct stm32_usbdev_s *priv = (struct stm32_usbdev_s *)dev;

  usbtrace(TRACE_DEVSELFPOWERED, (uint16)selfpowered);

#ifdef CONFIG_DEBUG
  if (!dev)
    {
      usbtrace(TRACE_DEVERROR(STM32_TRACEERR_INVALIDPARMS), 0);
      return -ENODEV;
    }
#endif

  priv->selfpowered = selfpowered;
  return OK;
}

/****************************************************************************
 * Initialization
 ****************************************************************************/
/****************************************************************************
 * Name: stm32_reset
 ****************************************************************************/

static void stm32_reset(struct stm32_usbdev_s *priv)
{
  int epno;

  /* Disable the USB controller, disable all USB interrupts */

  stm32_putreg(USB_CNTR_FRES|USB_CNTR_PDWN, STM32_USB_CNTR);

  /* Reset the device state structure */

  priv->devstate  = DEVSTATE_INIT;
  priv->rsmstate  = RSMSTATE_IDLE;
  priv->rxpending = FALSE;

  /* Reset endpoints */

  for (epno = 0; epno < STM32_NENDPOINTS; epno++)
    {
      struct stm32_ep_s *privep = &priv->eplist[epno];

      /* Cancel any queue requests */

      stm32_cancelrequests(privep);

      /* Reset endpoint status */

      privep->stalled   = FALSE;
      privep->halted    = FALSE;
      privep->txbusy    = FALSE;
      privep->txnullpkt = FALSE;
    }

  /* Re-configure the USB controller in its initial, unconnected state */

  stm32_hwreset(priv);

  /* Enable USB controller interrupts */

  up_enable_irq(STM32_IRQ_USBHPCANTX);
  up_enable_irq(STM32_IRQ_USBLPCANRX0);

  /* Set the interrrupt priority */

  up_prioritize_irq(STM32_IRQ_USBHPCANTX, CONFIG_USB_PRI);
  up_prioritize_irq( STM32_IRQ_USBLPCANRX0, CONFIG_USB_PRI);
} 

/****************************************************************************
 * Name: stm32_hwreset
 ****************************************************************************/

static void stm32_hwreset(struct stm32_usbdev_s *priv)
{
  /* Enable pull-up to connect the device */

  stm32_usbpullup(&priv->usbdev, TRUE);
  stm32_putreg(USB_CNTR_FRES, STM32_USB_CNTR);
  
  priv->imask = 0;
  stm32_putreg(priv->imask, STM32_USB_CNTR);

  /* Clear pending interrupts */

  stm32_putreg(0, STM32_USB_ISTR);

  /* Set the STM32 BTABLE address */

  stm32_putreg(STM32_BTABLE_ADDRESS & 0xfff8, STM32_USB_BTABLE);

  /* Initialize EP0 */

  stm32_seteptype(EP0, USB_EPR_EPTYPE_CONTROL);
  stm32_seteptxstatus(EP0, USB_EPR_STATTX_NAK);
  stm32_seteprxaddr(EP0, STM32_EP0_RXADDR);
  stm32_epsetrxcount(EP0, STM32_EP0MAXPACKET);
  stm32_seteptxaddr(EP0, STM32_EP0_TXADDR);
  stm32_clrstatusout(EP0);
  stm32_seteprxstatus(EP0, USB_EPR_STATRX_VALID);

  /* Set the device to respond on default address */

  stm32_setdevaddr(priv, 0);

  /* Enable interrupts at the USB controllr */

  priv->imask = STM32_CNTR_SETUP;
  stm32_putreg(priv->imask, STM32_USB_CNTR);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/
/****************************************************************************
 * Name: up_usbinitialize
 * Description:
 *   Initialize the USB driver
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

void up_usbinitialize(void) 
{
  /* For now there is only one USB controller, but we will always refer to
   * it using a pointer to make any future ports to multiple USB controllers
   * easier.
   */

  struct stm32_usbdev_s *priv = &g_usbdev;
  int epno;

  usbtrace(TRACE_DEVINIT, 0);
  stm32_checksetup();

  /* Disable the USB controller, disable all USB interrupts */

  stm32_putreg(USB_CNTR_FRES|USB_CNTR_PDWN, STM32_USB_CNTR);

  /* Initialize the device state structure.  NOTE: many fields
   * have the initial value of zero and, hence, are not explicitly
   * initialized here.
   */

  memset(priv, 0, sizeof(struct stm32_usbdev_s));
  priv->usbdev.ops   = &g_devops;
  priv->usbdev.ep0   = &priv->eplist[EP0].ep;
  priv->epavail      = STM32_ENDP_ALLSET & ~STM32_ENDP_BIT(EP0);
  priv->bufavail     = STM32_BUFFER_ALLSET & ~STM32_BUFFER_EP0;

  /* Initialize the endpoint list */

  for (epno = 0; epno < STM32_NENDPOINTS; epno++)
    {
      /* Set endpoint operations, reference to driver structure (not
       * really necessary because there is only one controller), and
       * the (physical) endpoint number which is just the index to the
       * endpoint.
       */

      priv->eplist[epno].ep.ops    = &g_epops;
      priv->eplist[epno].dev       = priv;
      priv->eplist[epno].ep.eplog  = epno;

      /* We will use a fixed maxpacket size for all endpoints (perhaps
       * ISOC endpoints could have larger maxpacket???).  A smaller
       * packet size can be selected when the endpoint is configured.
       */

      priv->eplist[epno].ep.maxpacket = STM32_MAXPACKET_SIZE;
    }

  /* Select a smallest endpoint size for EP0 */

#if STM32_EP0MAXPACKET < STM32_MAXPACKET_SIZE
  priv->eplist[EP0].ep.maxpacket = STM32_EP0MAXPACKET;
#endif

  /* Setup the USB controller */

  stm32_hwreset(priv);

  /* Attach USB controller interrupt handlers */

  if (irq_attach(STM32_IRQ_USBHPCANTX, stm32_hpinterrupt) != 0)
    {
      usbtrace(TRACE_DEVERROR(STM32_TRACEERR_IRQREGISTRATION),
               (uint16)STM32_IRQ_USBHPCANTX);
      goto errout;
    }

  if (irq_attach STM32_IRQ_USBLPCANRX0, stm32_lpinterrupt) != 0)
    {
      usbtrace(TRACE_DEVERROR(STM32_TRACEERR_IRQREGISTRATION),
               (uint16)STM32_IRQ_USBLPCANRX0);
      goto errout;
    }
  return;

errout:
  up_usbuninitialize();
} 

/****************************************************************************
 * Name: up_usbuninitialize
 * Description:
 *   Initialize the USB driver
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

void up_usbuninitialize(void) 
{
  /* For now there is only one USB controller, but we will always refer to
   * it using a pointer to make any future ports to multiple USB controllers
   * easier.
   */

  struct stm32_usbdev_s *priv = &g_usbdev;
  uint16 regval;
  irqstate_t flags;

  usbtrace(TRACE_DEVUNINIT, 0);

  if (priv->driver)
    {
      usbtrace(TRACE_DEVERROR(STM32_TRACEERR_DRIVERREGISTERED), 0);
      usbdev_unregister(priv->driver);
    }

  flags = irqsave();
  priv->usbdev.speed = USB_SPEED_UNKNOWN;

  /* Disable and detach IRQs */

  up_disable_irq(STM32_IRQ_USBHPCANTX);
  up_disable_irq(STM32_IRQ_USBLPCANRX0);
  irq_detach(STM32_IRQ_USBHPCANTX);
  irq_detach(STM32_IRQ_USBLPCANRX0);

  /* Disable all ints and force USB reset */ 

  stm32_putreg(USB_CNTR_FRES, STM32_USB_CNTR);
  
  /* Clear pending interrupts */ 

  stm32_putreg(0, STM32_USB_ISTR);
  
  /* Disconnect the device / disable the pull-up */ 

  stm32_usbpullup(&priv->usbdev, FALSE);
  
  /* Disable USB */

  stm32_putreg(USB_CNTR_FRES|USB_CNTR_PDWN, STM32_USB_CNTR);
  irqrestore(flags);
}

/****************************************************************************
 * Name: usbdev_register
 *
 * Description:
 *   Register a USB device class driver. The class driver's bind() method will be
 *   called to bind it to a USB device driver.
 *
 ****************************************************************************/

int usbdev_register(struct usbdevclass_driver_s *driver)
{
  /* For now there is only one USB controller, but we will always refer to
   * it using a pointer to make any future ports to multiple USB controllers
   * easier.
   */

  struct stm32_usbdev_s *priv = &g_usbdev;
  int ret;

  usbtrace(TRACE_DEVREGISTER, 0);

#ifdef CONFIG_DEBUG
  if (!driver || !driver->ops->bind || !driver->ops->unbind ||
      !driver->ops->disconnect || !driver->ops->setup)
    {
      usbtrace(TRACE_DEVERROR(STM32_TRACEERR_INVALIDPARMS), 0);
      return -EINVAL;
    }

  if (priv->driver)
    {
      usbtrace(TRACE_DEVERROR(STM32_TRACEERR_DRIVER), 0);
      return -EBUSY;
    }
#endif

  /* First hook up the driver */

   priv->driver = driver;

  /* Then bind the class driver */

  ret = CLASS_BIND(driver, &priv->usbdev);
  if (ret)
    {
      usbtrace(TRACE_DEVERROR(STM32_TRACEERR_BINDFAILED), (uint16)-ret);
       priv->driver = NULL;
    }
  else
    {
      /* Enable USB controller interrupts */

      up_enable_irq(STM32_IRQ_USBHPCANTX);
      up_enable_irq(STM32_IRQ_USBLPCANRX0);

      /* Set the interrrupt priority */

      up_prioritize_irq(STM32_IRQ_USBHPCANTX, CONFIG_USB_PRI);
      up_prioritize_irq(STM32_IRQ_USBLPCANRX0, CONFIG_USB_PRI);
   }
  return ret;
}

/****************************************************************************
 * Name: usbdev_unregister
 *
 * Description:
 *   Un-register usbdev class driver.If the USB device is connected to a USB host,
 *   it will first disconnect().  The driver is also requested to unbind() and clean
 *   up any device state, before this procedure finally returns.
 *
 ****************************************************************************/

int usbdev_unregister(struct usbdevclass_driver_s *driver)
{
  /* For now there is only one USB controller, but we will always refer to
   * it using a pointer to make any future ports to multiple USB controllers
   * easier.
   */

  struct stm32_usbdev_s *priv = &g_usbdev;
  usbtrace(TRACE_DEVUNREGISTER, 0);

#ifdef CONFIG_DEBUG
  if (driver != priv->driver)
    {
      usbtrace(TRACE_DEVERROR(STM32_TRACEERR_INVALIDPARMS), 0);
      return -EINVAL;
    }
#endif

  /* Unbind the class driver */

  CLASS_UNBIND(driver, &priv->usbdev);

  /* Disable USB controller interrupts */

  up_disable_irq(STM32_IRQ_USBHPCANTX);
  up_disable_irq(STM32_IRQ_USBLPCANRX0);

  /* Unhook the driver */

  priv->driver = NULL;
  return OK;
}

#endif /* CONFIG_USBDEV && CONFIG_STM32_USB */