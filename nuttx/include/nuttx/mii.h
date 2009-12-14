/****************************************************************************
 * include/nuttx/mii.h
 *
 *   Copyright (C) 2008-2009 Gregory Nutt. All rights reserved.
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

#ifndef __INCLUDE_NUTTX_MII_H
#define __INCLUDE_NUTTX_MII_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

/****************************************************************************
 * Pre-Processor Definitions
 ****************************************************************************/

/* MII register offsets *****************************************************/

/* Common MII management registers. The IEEE 802.3 standard specifies a
 * register set for controlling and gathering status from the PHY layer. The
 * registers are collectively known as the MII Management registers and are
 * detailed in Section 22.2.4 of the IEEE 802.3 specification.
 */

#define MII_MCR                      0x00      /* MII management control */
#define MII_MSR                      0x01      /* MII management status */
#define MII_PHYID1                   0x02      /* PHY ID 1 */
#define MII_PHYID2                   0x03      /* PHY ID 2 */
#define MII_ADVERTISE                0x04      /* Auto-negotiation advertisement */
#define MII_LPA                      0x05      /* Auto-negotiation link partner base page ability */
#define MII_EXPANSION                0x06      /* Auto-negotiation expansion */
#define MII_NEXTPAGE                 0x07      /* Auto-negotiation next page */
#define MII_LPANEXTPAGE              0x08      /* Auto-negotiation link partner received next page */
#define MII_MSCONTROL                0x09      /* Master/slave control register */
#define MII_MSSTATUS                 0x0a      /* Master/slave status register */
#define MII_PSECONTROL               0x0b      /* PSE control register */
#define MII_PSESTATUS                0x0c      /* PSE status register */
#define MII_MMDCONTROL               0x0d      /* MMD access control register */
#define MII_ESTATUS                  0x0f      /* Extended status register */

/* Registers 16-31 may be used for vendor specific abilities */

/* DP83840: 0x07-0x11, 0x14, 0x1a, 0x1d-0x1f reserved */

#define MII_DP83840_COUNTER          0x12      /* Disconnect counter */
#define MII_DP83840_FCSCOUNTER       0x13      /* False carrier sense counter */
#define MII_DP83840_NWAYTEST         0x14      /* N-way auto-neg test reg */
#define MII_DP83840_RERRCOUNTER      0x15      /* Receive error counter */
#define MII_DP83840_SREVISION        0x16      /* Silicon revision */
#define MII_DP83840_LBRERROR         0x18      /* Loopback, bypass and receiver error */
#define MII_DP83840_PHYADDR          0x19      /* PHY address */
#define MII_DP83840_10BTSR           0x1b      /* 10BASE-T status register */
#define MII_DP83840_10BTCR           0x1c      /* 10BASE-T configuration register */

/* Am79c874: 0x08-0x0f, 0x14, 0x16, 0x19-0x1f reserved */

#define MII_AM79C874_NPADVERTISE     0x07      /* Auto-negotiation next page advertisement */
#define MII_AM79C874_MISCFEATURES    0x10      /* Miscellaneous features reg */
#define MII_AM79C874_INTCS           0x11      /* Interrupt control/status */
#define MII_AM79C874_DIAGNOSTIC      0x12      /* Diagnostic */
#define MII_AM79C874_LOOPBACK        0x13      /* Power management/loopback */
#define MII_AM79C874_MODEC           0x15      /* Mode control register */
#define MII_AM79C874_DISCONNECT      0x17      /* Disconnect counter */
#define MII_AM79C874_RCVERROR        0x18      /* Receive error counter */

/* Luminary LM3S6918 built-in PHY, 0x07-0x0f, 0x14-0x16, 0x19-0x1f reserved */

#define MII_LM3S_VSPECIFIC           0x10      /* Vendor-Specific */
#define MII_LM3S_INTCS               0x11      /* Interrupt control/status */
#define MII_LM3S_DIAGNOSTIC          0x12      /* Diagnostic */
#define MII_LM3S_XCVRCONTROL         0x13      /* Transceiver Control */
#define MII_LM3S_LEDCONFIG           0x17      /* LED Configuration */
#define MII_LM3S_MDICONTROL          0x18      /* Ethernet PHY Management MDI/MDIX Control */

/* */

#define MII_CTRL1000                 0x09      /* 1000BASE-T control */
#define MII_STAT1000                 0x0a      /* 1000BASE-T status */
#define MII_NCONFIG                  0x1c      /* Network interface config */

/* MII register bit settings ************************************************/

/* MII Control register bit definitions */

#define MII_MCR_SPEED1000            (1 << 6)  /* Bit 6:  MSB of Speed (1000 reserved on 10/100) */
#define MII_MCR_CTST                 (1 << 7)  /* Bit 7:  Enable collision test  */
#define MII_MCR_FULLDPLX             (1 << 8)  /* Bit 8:  Full duplex */
#define MII_MCR_ANRESTART            (1 << 9)  /* Bit 9:  Restart auto negotiation */
#define MII_MCR_ISOLATE              (1 << 10) /* Bit 10: Electronically isolate PHY from MII */
#define MII_MCR_PDOWN                (1 << 11) /* Bit 11: Powerdown the PHY */
#define MII_MCR_ANENABLE             (1 << 12) /* Bit 12: Enable auto negotiation */
#define MII_MCR_SPEED100             (1 << 13) /* Bit 13: Select 100Mbps */
#define MII_MCR_LOOPBACK             (1 << 14) /* Bit 14: Enable loopback mode */
#define MII_MCR_RESET                (1 << 15) /* Bit 15: PHY reset */

/* MII Status register bit definitions */

#define MII_MSR_EXTCAP               (1 << 0)  /* Bit 0:  Extended register capability */
#define MII_MSR_JABBERDETECT         (1 << 1)  /* Bit 1:  Jabber detect */
#define MII_MSR_LINKSTATUS           (1 << 2)  /* Bit 2:  Link status */
#define MII_MSR_ANEGABLE             (1 << 3)  /* Bit 3:  Auto-negotiation able */
#define MII_MSR_RFAULT               (1 << 4)  /* Bit 4:  Remote fault */
#define MII_MSR_ANEGCOMPLETE         (1 << 5)  /* Bit 5:  Auto-negotiation complete */
#define MII_MSR_MFRAMESUPPRESS       (1 << 6)  /* Bit 6:  Management frame suppression */
#define MII_MSR_ESTATEN              (1 << 8)  /* Bit 8:  Extended Status in R15 */
#define MII_MSR_100BASET2FULL        (1 << 9)  /* Bit 9:  100BASE-T2 half duplex able */
#define MII_MSR_100BASET2HALF        (1 << 10) /* Bit 10: 100BASE-T2 full duplex able */
#define MII_MSR_10BASETXHALF         (1 << 11) /* Bit 11: 10BASE-TX half duplex able */
#define MII_MSR_10BASETXFULL         (1 << 12) /* Bit 12: 10BASE-TX full duplex able */
#define MII_MSR_100BASETXHALF        (1 << 13) /* Bit 13: 100BASE-TX half duplex able */
#define MII_MSR_100BASETXFULL        (1 << 14) /* Bit 14: 100BASE-TX full duplex able */
#define MII_MSR_100BASET4            (1 << 15) /* Bit 15: 100BASE-T4 able */

/* MII ID2 register bits */

#define MII_PHYID2_OUI               0xfc00    /* Bits 19-24 of OUI mask */
#define MII_PHYID2_MODEL             0x03f0    /* Model number mask */
#define MII_PHYID2_REV               0x000f    /* Revision number mask */

#define MII_PHYID1_AM79C874          0x0022    /* ID1 value for Am79c874 */
#define MII_PHYID2_AM79C874          0x561b    /* ID2 value for Am79c874 Rev B */

/* Advertisement control register bit definitions */

#define MII_ADVERTISE_SELECT         0x001f    /* Bits 0-4: Selector field */
#define MII_ADVERTISE_CSMA           (1 << 0)  /*         CSMA */
#define MII_ADVERTISE_8023           (1 << 0)  /*         IEEE Std 802.3 */
#define MII_ADVERTISE_8029           (2 << 0)  /*         IEEE Std 802.9 ISLAN-16T */
#define MII_ADVERTISE_8025           (3 << 0)  /*         IEEE Std 802.5 */
#define MII_ADVERTISE_1394           (4 << 0)  /*         IEEE Std 1394 */
#define MII_ADVERTISE_10BASETXHALF   (1 << 5)  /* Bit 5:  Try 10BASE-TX half duplex */
#define MII_ADVERTISE_1000XFULL      (1 << 5)  /* Bit 5:  Try 1000BASE-X full duplex */
#define MII_ADVERTISE_10BASETXFULL   (1 << 6)  /* Bit 6:  Try 10BASE-TX full duplex */
#define MII_ADVERTISE_1000XHALF      (1 << 6)  /* Bit 6:  Try 1000BASE-X half duplex */
#define MII_ADVERTISE_100BASETXHALF  (1 << 7)  /* Bit 7:  Try 100BASE-TX half duplex */
#define MII_ADVERTISE_1000XPAUSE     (1 << 7)  /* Bit 7:  Try 1000BASE-X pause */
#define MII_ADVERTISE_100BASETXFULL  (1 << 8)  /* Bit 8:  Try 100BASE-TX full duplex*/
#define MII_ADVERTISE_1000XASYMPAU   (1 << 8)  /* Bit 8:  Try 1000BASE-X asym pause */
#define MII_ADVERTISE_100BASET4      (1 << 9)  /* Bit 9:  Try 100BASE-T4 */
#define MII_ADVERTISE_FDXPAUSE       (1 << 10) /* Bit 10: Try full duplex flow control */
#define MII_ADVERTISE_ASYMPAUSE      (1 << 11) /* Bit 11: Try asymetric pause */
#define MII_ADVERTISE_RFAULT         (1 << 13) /* Bit 13: Remote fault supported */
#define MII_ADVERTISE_LPACK          (1 << 14) /* Bit 14: Ack link partners response */
#define MII_ADVERTISE_NXTPAGE        (1 << 15) /* Bit 15: Next page enabled */

/* Link partner ability register bit definitions */

#define MII_LPA_SELECT               0x001f    /* Bits 0-4: Link partner selector field */
#define MII_LPA_CSMA                 (1 << 0)  /*         CSMA */
#define MII_LPA_8023                 (1 << 0)  /*         IEEE Std 802.3 */
#define MII_LPA_8029                 (2 << 0)  /*         IEEE Std 802.9 ISLAN-16T */
#define MII_LPA_8025                 (3 << 0)  /*         IEEE Std 802.5 */
#define MII_LPA_1394                 (4 << 0)  /*         IEEE Std 1394 */
#define MII_LPA_10BASETXHALF         (1 << 5)  /* Bit 5:  10BASE-TX half duplex able */
#define MII_LPA_1000XFULL            (1 << 5)  /* Bit 5:  1000BASE-X full-duplex able */
#define MII_LPA_10BASETXFULL         (1 << 6)  /* Bit 6:  10BASE-TX full duplex able */
#define MII_LPA_1000XHALF            (1 << 6)  /* Bit 6:  1000BASE-X half-duplex */
#define MII_LPA_100BASETXHALF        (1 << 7)  /* Bit 7:  100BASE-TX half duplex able */
#define MII_LPA_1000XPAUSE           (1 << 7)  /* Bit 7:  1000BASE-X pause able */
#define MII_LPA_100BASETXFULL        (1 << 8)  /* Bit 8:  100BASE-TX full duplex able */
#define MII_LPA_1000XASYMPAU         (1 << 8)  /* Bit 8:  1000BASE-X asym pause able */
#define MII_LPA_100BASET4            (1 << 9)  /* Bit 9:  100BASE-T4 able */
#define MII_LPA_FDXPAUSE             (1 << 10) /* Bit 10: Full duplex flow control able */
#define MII_LPA_ASYMPAUSE            (1 << 11) /* Bit 11: Asynchronous pause able */
#define MII_LPA_RFAULT               (1 << 13) /* Bit 13: Link partner remote fault request */
#define MII_LPA_LPACK                (1 << 14) /* Bit 14: Link partner acknowledgement */
#define MII_LPA_NXTPAGE              (1 << 15) /* Bit 15: Next page requested */

/* Link partner ability in next page format */

#define MII_LPANP_MESSAGE            0x07ff    /* Bits 0-10: Link partner's message code */
#define MII_LPANP_TOGGLE             (1 << 11) /* Bit 11: Link partner toggle */
#define MII_LPANP_LACK2              (1 << 12) /* Bit 12: Link partner can comply ACK */
#define MII_LPANP_MSGPAGE            (1 << 13) /* Bit 13: Link partner message page request */
#define MII_LPANP_LPACK              (1 << 14) /* Bit 14: Link partner acknowledgement */
#define MII_LPANP_NXTPAGE            (1 << 15) /* Bit 15: Next page requested */

/* MII Auto-negotiation expansion register bit definitions */

#define MII_EXPANSION_ANEGABLE       (1 << 0)  /* Bit 0: Link partner is auto-negotion able */
#define MII_EXPANSION_PAGERECVD      (1 << 1)  /* Bit 1: New link code word in LPA ability reg */
#define MII_EXPANSION_ENABLENPAGE    (1 << 2)  /* Bit 2: This enables npage words */
#define MII_EXPANSION_NXTPAGEABLE    (1 << 3)  /* Bit 3: Link partner supports next page */
#define MII_EXPANSION_PARFAULTS      (1 << 4)  /* Bit 4: Fault detected by parallel logic */

/* Auto-negotiation next page advertisement */

#define MII_NPADVERTISE_CODE         0x07ff    /* Bits 0-10: message/un-formated code field */
#define MII_NPADVERTISE_TOGGLE       (1 << 11) /* Bit 11: Toggle */
#define MII_NPADVERTISE_ACK2         (1 << 12) /* Bit 12: Acknowledgement 2 */
#define MII_NPADVERTISE_MSGPAGE      (1 << 13) /* Bit 13: Message page */
#define MII_NPADVERTISE_NXTPAGE      (1 << 15) /* Bit 15: Next page indication */

/* MII PHYADDR register bit definitions */

#define DP83840_PHYADDR_DUPLEX       (1 << 7)
#define DP83840_PHYADDR_SPEED        (1 << 6)

/* Am79c874 diagnostics register */

#define AM79C874_DIAG_RXLOCK         (1 << 8)  /* Bit 8:  1=Rcv PLL locked on */
#define AM79C874_DIAG_RXPASS         (1 << 9)  /* Bit 9:  1=Operating in 100Base-X mode */
#define AM79C874_DIAG_100MBPS        (1 << 10) /* Bit 10: 1=ANEG result is 100Mbps */
#define AM79C874_DIAG_FULLDPLX       (1 << 11) /* Bit 11: 1=ANEG result is full duplex */

/* LM3S6918 Vendor-Specific, address 0x10 */

#define LM3S_VSPECIFIC_RXCC          (1 << 0)  /* Bit 0:  Receive Clock Control*/
#define LM3S_VSPECIFIC_PCSBP         (1 << 1)  /* Bit 1:  PCS Bypass */
#define LM3S_VSPECIFIC_RVSPOL        (1 << 4)  /* Bit 4:  Receive Data Polarity */
#define LM3S_VSPECIFIC_APOL          (1 << 5)  /* Bit 5:  Auto-Polarity Disable */
#define LM3S_VSPECIFIC_NL10          (1 << 10) /* Bit 10: Natural Loopback Mode */
#define LM3S_VSPECIFIC_SQEI          (1 << 11) /* Bit 11: SQE Inhibit Testing */
#define LM3S_VSPECIFIC_TXHIM         (1 << 12) /* Bit 12: Transmit High Impedance Mode */
#define LM3S_VSPECIFIC_INPOL         (1 << 14) /* Bit 14: Interrupt Polarity Value*/
#define LM3S_VSPECIFIC_RPTR          (1 << 15) /* Bit 15: Repeater mode*/

/* LM3S6918 Interrupt Control/Status, address 0x11 */

#define LM3S_INTCS_ANEGCOMPINT       (1 << 0)  /* Bit 0:  Auto-Negotiation Complete Interrupt */
#define LM3S_INTCS_RFAULTINT         (1 << 1)  /* Bit 1:  Remote Fault Interrupt */
#define LM3S_INTCS_LSCHGINT          (1 << 2)  /* Bit 2:  Link Status Change Interrupt */
#define LM3S_INTCS_LPACKINT          (1 << 3)  /* Bit 3:  LP Acknowledge Interrupt */
#define LM3S_INTCS_PDFINT            (1 << 4)  /* Bit 4:  Parallel Detection Fault Interrupt */
#define LM3S_INTCS_PRXINT            (1 << 5)  /* Bit 5:  Page Receive Interrupt */
#define LM3S_INTCS_RXERINT           (1 << 6)  /* Bit 6:  Receive Error Interrupt */
#define LM3S_INTCS_JABBERINT         (1 << 7)  /* Bit 7:  Jabber Event Interrupt */
#define LM3S_INTCS_ANEGCOMPIE        (1 << 8)  /* Bit 8:  Auto-Negotiation Complete Interrupt Enable */
#define LM3S_INTCS_RFAULTIE          (1 << 9)  /* Bit 9:  Remote Fault Interrupt Enable */
#define LM3S_INTCS_LSCHGIE           (1 << 10) /* Bit 10: Link Status Change Interrupt Enable */
#define LM3S_INTCS_LPACKIE           (1 << 11) /* Bit 11: LP Acknowledge Interrupt Enable */
#define LM3S_INTCS_PDFIE             (1 << 12) /* Bit 12: Parallel Detection Fault Interrupt Enable */
#define LM3S_INTCS_PRXIE             (1 << 13) /* Bit 13: Page Received Interrupt Enable */
#define LM3S_INTCS_RXERIE            (1 << 14) /* Bit 14: Receive Error Interrupt Enable */
#define LM3S_INTCS_JABBERIE          (1 << 15) /* Bit 15: Jabber Interrupt Enable */

/* LM3S6918 Diagnostic, address 0x12 */

#define LM3S_DIAGNOSTIC_RX_LOCK      (1 << 8)  /* Bit 8:  Receive PLL Lock */
#define LM3S_DIAGNOSTIC_RXSD         (1 << 9)  /* Bit 9:  Receive Detection */
#define LM3S_DIAGNOSTIC_RATE         (1 << 10) /* Bit 10: Rate */
#define LM3S_DIAGNOSTIC_DPLX         (1 << 11) /* Bit 11: Duplex Mode */
#define LM3S_DIAGNOSTIC_ANEGF        (1 << 12) /* Bit 12: Auto-Negotiation Failure */

/* LM3S6918 Transceiver Control, address 0x13 */

#define LM3S_XCVRCONTROL_TXO_SHIFT   14        /* Bits 15-14: Transmit Amplitude Selection */
#define LM3S_XCVRCONTROL_TXO_MASK    (3 << LM3S_XCVRCONTROL_TXO_SHIFT)
#define LM3S_XCVRCONTROL_TXO_00DB    (0 << LM3S_XCVRCONTROL_TXO_SHIFT) /* Gain 0.0dB of insertion loss */
#define LM3S_XCVRCONTROL_TXO_04DB    (1 << LM3S_XCVRCONTROL_TXO_SHIFT) /* Gain 0.4dB of insertion loss */
#define LM3S_XCVRCONTROL_TXO_08DB    (2 << LM3S_XCVRCONTROL_TXO_SHIFT) /* Gain 0.8dB of insertion loss */
#define LM3S_XCVRCONTROL_TXO_12DB    (3 << LM3S_XCVRCONTROL_TXO_SHIFT) /* Gain 1.2dB of insertion loss */

/* LED Configuration, address 0x17 */

#define LM3S_LEDCONFIG_LED0_SHIFT    0         /* Bits 3-0: LED0 Source */
#define LM3S_LEDCONFIG_LED0_MASK     (0x0f << LM3S_LEDCONFIG_LED0_SHIFT)
#define LM3S_LEDCONFIG_LED0_LINKOK   (0 << LM3S_LEDCONFIG_LED0_SHIFT)  /* Link OK */
#define LM3S_LEDCONFIG_LED0_RXTX     (1 << LM3S_LEDCONFIG_LED0_SHIFT)  /* RX or TX activity */
#define LM3S_LEDCONFIG_LED0_100BASET (5 << LM3S_LEDCONFIG_LED0_SHIFT)  /* 100BASE-TX mode */
#define LM3S_LEDCONFIG_LED0_10BASET  (6 << LM3S_LEDCONFIG_LED0_SHIFT)  /* 10BASE-T mode */
#define LM3S_LEDCONFIG_LED0_FDUPLEX  (7 << LM3S_LEDCONFIG_LED0_SHIFT)  /* Full duplex */
#define LM3S_LEDCONFIG_LED0_OKRXTX   (8 << LM3S_LEDCONFIG_LED0_SHIFT)  /* Full duplex */
#define LM3S_LEDCONFIG_LED1_SHIFT    4          /* Bits 7-4: LED1 Source */
#define LM3S_LEDCONFIG_LED1_MASK     (0x0f << LM3S_LEDCONFIG_LED1_SHIFT)
#define LM3S_LEDCONFIG_LED1_LINKOK   (0 << LM3S_LEDCONFIG_LED1_SHIFT)  /* Link OK */
#define LM3S_LEDCONFIG_LED1_RXTX     (1 << LM3S_LEDCONFIG_LED1_SHIFT)  /* RX or TX activity */
#define LM3S_LEDCONFIG_LED1_100BASET (5 << LM3S_LEDCONFIG_LED1_SHIFT)  /* 100BASE-TX mode */
#define LM3S_LEDCONFIG_LED1_10BASET  (6 << LM3S_LEDCONFIG_LED1_SHIFT)  /* 10BASE-T mode */
#define LM3S_LEDCONFIG_LED1_FDUPLEX  (7 << LM3S_LEDCONFIG_LED1_SHIFT)  /* Full duplex */
#define LM3S_LEDCONFIG_LED1_OKRXTX   (8 << LM3S_LEDCONFIG_LED1_SHIFT)  /* Full duplex */

/* MDI/MDIX Control, address 0x18 */

#define LM3S_MDICONTROL_MDIXSD_SHIFT 0          /* Bits 3-0: Auto-Switching Seed */
#define LM3S_MDICONTROL_MDIXSD_MASK  (0x0f << LM3S_MDICONTROL_MDIXSD_SHIFT)
#define LM3S_MDICONTROL_MDIXCM       (1 << 4)  /* Bit 4:  Auto-Switching Complete */
#define LM3S_MDICONTROL_MDIX         (1 << 5)  /* Bit 5:  Auto-Switching Configuration */
#define LM3S_MDICONTROL_AUTOSW       (1 << 6)  /* Bit 6:  Auto-Switching Enable */
#define LM3S_MDICONTROL_PDMODE       (1 << 7)  /* Bit 7:  Parallel Detection Mode */

/****************************************************************************
 * Type Definitions
 ****************************************************************************/

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifdef __cplusplus
#define EXTERN extern "C"
extern "C" {
#else
#define EXTERN extern
#endif

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* __INCLUDE_NUTTX_MII_H */
