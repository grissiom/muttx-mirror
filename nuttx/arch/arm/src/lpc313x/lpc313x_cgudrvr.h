/************************************************************************
 * arch/arm/src/lpc313x/lpc313x_cgudrvr.h
 *
 *   Copyright (C) 2009 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <spudmonkey@racsa.co.cr>
 *
 * References:
 *   - UM10314 LPC3130/31 User manual Rev. 1.01 � 9 September 2009
 *   - lpc313x.cdl.drivers.zip example driver code
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
 ************************************************************************/

#ifndef __ARCH_ARM_SRC_LPC313X_CGUDRVR_H
#define __ARCH_ARM_SRC_LPC313X_CGUDRVR_H

/************************************************************************
 * Included Files
 ************************************************************************/

#include <nuttx/config.h>
#include "lpc313x_cgu.h"

/************************************************************************
 * Pre-processor Definitions
 ************************************************************************/

/* Maps a valid, x, relative to a base value, b, and converts that to a bit position */

#define _RBIT(x,b)                        (1<<((x)-(b)))

/* Clock ID ranges (see enum lpc313x_clockid_e) *************************************************/

#define CLKID_SYSBASE_FIRST               CLKID_APB0CLK           /* Domain 0: SYS_BASE */
#define CLKID_SYSBASE_LAST                CLKID_INTCCLK
#define _D0B(id)                          _RBIT(id,CLKID_SYSBASE_FIRST)

#define CLKID_AHB0APB0_FIRST              CLKID_AHB2APB0ASYNCPCLK /* Domain 1: AHB0APB0_BASE */
#define CLKID_AHB0APB0_LAST               CLKID_RNGPCLK
#define _D1B(id)                          _RBIT(id,CLKID_AHB0APB0_FIRST)

#define CLKID_AHB0APB1_FIRST              CLKID_AHB2APB1ASYNCPCLK /* Domain 2: AHB0APB1_BASE */
#define CLKID_AHB0APB1_LAST               CLKID_I2C1PCLK
#define _D2B(id)                          _RBIT(id,CLKID_AHB0APB1_FIRST)

#define CLKID_AHB0APB2_FIRST              CLKID_AHB2APB2ASYNCPCLK /* Domain 3: AHB0APB2_BASE */
#define CLKID_AHB0APB2_LAST               CLKID_SPIPCLKGATED
#define _D3B(id)                          _RBIT(id,CLKID_AHB0APB2_FIRST)

#define CLKID_AHB0APB3_FIRST              CLKID_AHB2APB3PCLK      /* Domain 4: AHB0APB3_BASE */
#define CLKID_AHB0APB3_LAST               CLKID_RESERVED70
#define _D4B(id)                          _RBIT(id,CLKID_AHB0APB3_FIRST)

#define CLKID_PCM_FIRST                   CLKID_PCMCLKIP          /* Domain 5: PCM_BASE */
#define CLKID_PCM_LAST                    CLKID_PCMCLKIP
#define _D5B(id)                          _RBIT(id,CLKID_PCM_FIRST)

#define CLKID_UART_FIRST                  CLKID_UARTUCLK          /* Domain 6: UART_BASE */
#define CLKID_UART_LAST                   CLKID_UARTUCLK
#define _D6B(id)                          _RBIT(id,CLKID_UART_FIRST)

#define CLKID_CLK1024FS_FIRST             CLKID_I2SEDGEDETECTCLK  /* Domain 7: CLK1024FS_BASE */
#define CLKID_CLK1024FS_LAST              CLKID_RESERVED86
#define _D7B(id)                          _RBIT(id,CLKID_CLK1024FS_FIRST)

#define CLKID_I2SRXBCK0_FIRST             CLKID_I2SRXBCK0         /* Domain 8: BCK0_BASE */
#define CLKID_I2SRXBCK0_LAST              CLKID_I2SRXBCK0
#define _D8B(id)                          _RBIT(id,CLKID_I2SRXBCK0_FIRST)

#define CLKID_I2SRXBCK1_FIRST             CLKID_I2SRXBCK1         /* Domain 9: BCK1_BASE */
#define CLKID_I2SRXBCK1_LAST              CLKID_I2SRXBCK1
#define _D9B(id)                          _RBIT(id,CLKID_SYSBASE_FIRST)

#define CLKID_SPI_FIRST                   CLKID_SPICLK            /* Domain 10: SPI_BASE */
#define CLKID_SPI_LAST                    CLKID_SPICLKGATED
#define _D10B(id)                         _RBIT(id,CLKID_I2SRXBCK1_FIRST)

#define CLKID_SYSCLKO_FIRST               CLKID_SYSCLKO           /* Domain 11: SYSCLKO_BASE */
#define CLKID_SYSCLKO_LAST                CLKID_SYSCLKO
#define _D11B(id)                         _RBIT(id,CLKID_SYSCLKO_FIRST)

#define CGU_NDOMAINS                      12 /* The number of clock domains */
#define CLKID_INVALIDCLK                  -1 /* Indicates and invalid clock ID */
#define DOMAINID_INVALID                  -1 /* Indicates an invalid domain ID */
#define ESRNDX_INVALID                    -1 /* Indicates an invalid ESR register index */

/* There are 24 fractional dividers, indexed 0 to 23.  The following definitions
 * provide (1) the number of fractional dividers available for each base frequency,
 * (2) start and end indices, and (3) extraction info for sub elements from the
 * fractional divider configuration register
 */

#define FRACDIV_BASE0_CNT                 7  /* 7 fractional dividers available */
#define FRACDIV_BASE0_LOW                 0  /* First is index 0 */
#define FRACDIV_BASE0_HIGH                6  /* Last is index 6 */
#define FRACDIV_BASE0_FDIV0W              8

#define FRACDIV_BASE1_CNT                 2  /* 2 fractional dividers available */
#define FRACDIV_BASE1_LOW                 7  /* First is index 7 */
#define FRACDIV_BASE1_HIGH                8  /* Last is index 8 */
#define FRACDIV_BASE1_FDIV0W              8

#define FRACDIV_BASE2_CNT                 2  /* 2 fractional dividers available */
#define FRACDIV_BASE2_LOW                 9  /* First is index 9 */
#define FRACDIV_BASE2_HIGH                10 /* Last is index 10 */
#define FRACDIV_BASE2_FDIV0W              8

#define FRACDIV_BASE3_CNT                 3  /* 3 fractional dividers available */
#define FRACDIV_BASE3_LOW                 11 /* First is index 11 */
#define FRACDIV_BASE3_HIGH                13 /* Last is index 12 */
#define FRACDIV_BASE3_FDIV0W              8

#define FRACDIV_BASE4_CNT                 1  /* 1 fractional divider available */
#define FRACDIV_BASE4_LOW                 14 /* First is index 14 */
#define FRACDIV_BASE4_HIGH                14 /* Last is index 14 */
#define FRACDIV_BASE4_FDIV0W              8

#define FRACDIV_BASE5_CNT                 1  /* 1 fractional divider available */
#define FRACDIV_BASE5_LOW                 15 /* First is index 15 */
#define FRACDIV_BASE5_HIGH                15 /* Last is index 15 */
#define FRACDIV_BASE5_FDIV0W              8

#define FRACDIV_BASE6_CNT                 1  /* 1 fractional divider available */
#define FRACDIV_BASE6_LOW                 16 /* First is index 16 */
#define FRACDIV_BASE6_HIGH                16 /* Last is index 16 */
#define FRACDIV_BASE6_FDIV0W              8

#define FRACDIV_BASE7_CNT                 6  /* 6 fractional dividers available */
#define FRACDIV_BASE7_LOW                 17 /* First is index 17 */
#define FRACDIV_BASE7_HIGH                22 /* Last is index 22 */
#define FRACDIV_BASE7_FDIV0W              13

#define FRACDIV_BASE8_CNT                 0  /* No fractional divider available */
#define FRACDIV_BASE9_CNT                 0  /* No fractional divider available */

#define FRACDIV_BASE10_CNT                1  /* 1 fractional divider available */
#define FRACDIV_BASE10_LOW                23 /* First is index 23 */
#define FRACDIV_BASE10_HIGH               23 /* Last is index 23 */
#define FRACDIV_BASE10_FDIV0W             8

#define FRACDIV_BASE11_CNT                0  /* No fractional divider available */

#define FDCNDX_INVALID                   -1 /* Indicates an invalid fractional
                                              * divider index */

/************************************************************************
 * Public Types
 ************************************************************************/

#ifndef __ASSEMBLY__
#ifdef __cplusplus
#define EXTERN extern "C"
extern "C" {
#else
#define EXTERN extern
#endif

/* Clock domains */

enum lpc313x_domainid_e
{
  DOMAINID_SYS = 0,    /* Domain 0: SYS_BASE */
  DOMAINID_AHB0APB0,   /* Domain 1: AHB0APB0_BASE */
  DOMAINID_AHB0APB1,   /* Domain 2: AHB0APB1_BASE */
  DOMAINID_AHB0APB2,   /* Domain 3: AHB0APB2_BASE */
  DOMAINID_AHB0APB3,   /* Domain 4: AHB0APB3_BASE */
  DOMAINID_PCM,        /* Domain 5: PCM_BASE */
  DOMAINID_UART,       /* Domain 6: UART_BASE */
  DOMAINID_CLK1024FS,  /* Domain 7: CLK1024FS_BASE */
  DOMAINID_BCK0,       /* Domain 8: BCK0_BASE */
  DOMAINID_BCK1,       /* Domain 9: BCK1_BASE */
  DOMAINID_SPI,        /* Domain 10: SPI_BASE */
  DOMAINID_SYSCLKO     /* Domain 11: SYSCLKO_BASE */
};

/* Clock IDs -- These are indices must correspond to the register
 * offsets in lpc313x_cgu.h
 */

enum lpc313x_clockid_e
{
  /* Domain 0: SYS_BASE */

  CLKID_APB0CLK = 0,      /*  0 APB0_CLK */
  CLKID_SBAPB1CLK,        /*  1 APB1_CLK */
  CLKID_APB2CLK,          /*  2 APB2_CLK */
  CLKID_APB3CLK,          /*  3 APB3_CLK */
  CLKID_APB4CLK,          /*  4 APB4_CLK */
  CLKID_AHB2INTCCLK,      /*  5 AHB_TO_INTC_CLK */
  CLKID_AHB0CLK,          /*  6 AHB0_CLK */
  CLKID_EBICLK,           /*  7 EBI_CLK */
  CLKID_DMAPCLK,          /*  8 DMA_PCLK */
  CLKID_DMACLKGATED,      /*  9 DMA_CLK_GATED */
  CLKID_NANDFLASHS0CLK,   /* 10 NANDFLASH_S0_CLK */
  CLKID_NANDFLASHECCCLK,  /* 11 NANDFLASH_ECC_CLK */
  CLKID_NANDFLASHAESCLK,  /* 12 NANDFLASH_AES_CLK (Reserved on LPC313x) */ 
  CLKID_NANDFLASHNANDCLK, /* 13 NANDFLASH_NAND_CLK */
  CLKID_NANDFLASHPCLK,    /* 14 NANDFLASH_PCLK */
  CLKID_CLOCKOUT,         /* 15 CLOCK_OUT */
  CLKID_ARM926CORECLK,    /* 16 ARM926_CORE_CLK */
  CLKID_ARM926BUSIFCLK,   /* 17 ARM926_BUSIF_CLK */
  CLKID_ARM926RETIMECLK,  /* 18 ARM926_RETIME_CLK */
  CLKID_SDMMCHCLK,        /* 19 SD_MMC_HCLK */
  CLKID_SDMMCCCLKIN,      /* 20 SD_MMC_CCLK_IN */
  CLKID_USBOTGAHBCLK,     /* 21 USB_OTG_AHB_CLK */
  CLKID_ISRAM0CLK,        /* 22 ISRAM0_CLK */
  CLKID_REDCTLRSCLK,      /* 23 RED_CTL_RSCLK */
  CLKID_ISRAM1CLK,        /* 24 ISRAM1_CLK (LPC313x only) */
  CLKID_ISROMCLK,         /* 25 ISROM_CLK */
  CLKID_MPMCCFGCLK,       /* 26 MPMC_CFG_CLK */
  CLKID_MPMCCFGCLK2,      /* 27 MPMC_CFG_CLK2 */
  CLKID_MPMCCFGCLK3,      /* 28 MPMC_CFG_CLK3 */
  CLKID_INTCCLK,          /* 29 INTC_CLK */

  /* Domain 1: AHB0APB0BASE */

  CLKID_AHB2APB0PCLK,     /* 30 AHB_TO_APB0_PCLK */
  CLKID_EVENTROUTERPCLK,  /* 31 EVENT_ROUTER_PCLK */
  CLKID_ADCPCLK,          /* 32 ADC_PCLK */
  CLKID_ADCCLK,           /* 33 ADC_CLK */
  CLKID_WDOGPCLK,         /* 34 WDOG_PCLK */
  CLKID_IOCONFPCLK,       /* 35 IOCONF_PCLK */
  CLKID_CGUPCLK,          /* 36 CGU_PCLK */
  CLKID_SYSCREGPCLK,      /* 37 SYSCREG_PCLK */
  CLKID_OTPPCLK,          /* 38 OTP_PCLK (Reserved on LPC313X) */
  CLKID_RNGPCLK,          /* 39 RNG_PCLK */

  /* Domain 2: AHB0APB1BASE */

  CLKID_AHB2APB1PCLK,     /* 40 AHB_TO_APB1_PCLK */
  CLKID_TIMER0PCLK,       /* 41 TIMER0_PCLK */
  CLKID_TIMER1PCLK,       /* 42 TIMER1_PCLK */
  CLKID_TIMER2PCLK,       /* 43 TIMER2_PCLK */
  CLKID_TIMER3PCLK,       /* 44 TIMER3_PCLK */
  CLKID_PWMPCLK,          /* 45 PWM_PCLK */
  CLKID_PWMPCLKREGS,      /* 46 PWM_PCLK_REGS */
  CLKID_PWMCLK,           /* 47 PWM_CLK */
  CLKID_I2C0PCLK,         /* 48 I2C0_PCLK */
  CLKID_I2C1PCLK,         /* 49 I2C1_PCLK */

  /* Domain 3: AHB0APB2BASE */

  CLKID_AHB2APB2PCLK,     /* 50 AHB_TO_APB2_PCLK */
  CLKID_PCMPCLK,          /* 51 PCM_PCLK */
  CLKID_PCMAPBPCLK,       /* 52 PCM_APB_PCLK */
  CLKID_UARTAPBCLK,       /* 53 UART_APB_CLK */
  CLKID_LCDPCLK,          /* 54 LCD_PCLK */
  CLKID_LCDCLK,           /* 55 LCD_CLK */
  CLKID_SPIPCLK,          /* 56 SPI_PCLK */
  CLKID_SPIPCLKGATED,     /* 57 SPI_PCLK_GATED */

  /* Domain 4: AHB0APB3BASE */
  CLKID_AHB2APB3PCLK,     /* 58 AHB_TO_APB3_PCLK */
  CLKID_I2SCFGPCLK,       /* 59 I2S_CFG_PCLK */
  CLKID_EDGEDETPCLK,      /* 60 EDGE_DET_PCLK */
  CLKID_I2STXFIFO0PCLK,   /* 61 I2STX_FIFO_0_PCLK */
  CLKID_I2STXIF0PCLK,     /* 62 I2STX_IF_0_PCLK */
  CLKID_I2STXFIFO1PCLK,   /* 63 I2STX_FIFO_1_PCLK */
  CLKID_I2STXIF1PCLK,     /* 64 I2STX_IF_1_PCLK */
  CLKID_I2SRXFIFO0PCLK,   /* 65 I2SRX_FIFO_0_PCLK */
  CLKID_I2SRXIF0PCLK,     /* 66 I2SRX_IF_0_PCLK */
  CLKID_I2SRXFIFO1PCLK,   /* 67 I2SRX_FIFO_1_PCLK */
  CLKID_I2SRXIF1PCLK,     /* 68 I2SRX_IF_1_PCLK */
  CLKID_RESERVED69,       /* 69 Reserved */
  CLKID_RESERVED70,       /* 70 Reserved */

  /* Domain 5: PCM_BASE */

  CLKID_PCMCLKIP,         /* 71 PCM_CLK_IP */

  /* Domain 6: UART_BASE */

  CLKID_UARTUCLK,         /* 72 UART_U_CLK */
  
  /* Domain 7: CLK1024FS_BASE */

  CLKID_I2SEDGEDETECTCLK, /* 73 I2S_EDGE_DETECT_CLK */
  CLKID_I2STXBCK0N,       /* 74 I2STX_BCK0_N */
  CLKID_I2STXWS0,         /* 75 I2STX_WS0 */
  CLKID_I2STXCLK0,        /* 76 I2STX_CLK0 */
  CLKID_I2STXBCK1N,       /* 77 I2STX_BCK1_N */
  CLKID_I2STXWS1,         /* 78 I2STX_WS1 */
  CLKID_CLK256FS,         /* 79 CLK_256FS */
  CLKID_I2SRXBCK0N,       /* 80 I2SRX_BCK0_N */
  CLKID_I2SRXWS0,         /* 81 I2SRX_WS0 */
  CLKID_I2SRXBCK1N,       /* 82 I2SRX_BCK1_N */
  CLKID_I2SRXWS1,         /* 83 I2SRX_WS1 */
  CLKID_RESERVED84,       /* 84 Reserved */
  CLKID_RESERVED85,       /* 85 Reserved */
  CLKID_RESERVED86,       /* 86 Reserved */

  /* Domain 8: BCK0_BASE */

  CLKID_I2SRXBCK0,        /* 87 I2SRX_BCK0 */

  /* Domain 9: BCK1_BASE */

  CLKID_I2SRXBCK1,        /* 88 I2SRX_BCK1 */

  /* Domain 10: SPI_BASE */

  CLKID_SPICLK,           /* 89 SPI_CLK */
  CLKID_SPICLKGATED,      /* 90 SPI_CLK_GATED */

  /* Domain 11: SYSCLKO_BASE */

  CLKID_SYSCLKO           /* 91 SYSCLK_O */
};

/************************************************************************
 * Public Data
 ************************************************************************/

/************************************************************************
 * Inline Functions
 ************************************************************************/

/* Enable the specified clock */

static inline void lpc313x_enableclock(enum lpc313x_clockid_e clkid)
{
  uint32_t address = LPC313X_CGU_PCR((int)clkid);
  uint32_t regval  = getreg32(address);

  regval          |= CGU_PCR_RUN;
  putreg32(regval, address);
}

/* Disable the specified clock */

static inline void lpc313x_disableclock(enum lpc313x_clockid_e clkid)
{
  uint32_t address = LPC313X_CGU_PCR((int)clkid);
  uint32_t regval  = getreg32(address);

  regval          &= ~CGU_PCR_RUN;
  putreg32(regval, address);
}

/************************************************************************
 * Public Functions
 ************************************************************************/

/************************************************************************
 * Name: lpc313x_clkdomain
 *
 * Description:
 *   Given a clock ID, return the ID of the domain in which the clock
 *   resides.
 ************************************************************************/

EXTERN enum lpc313x_domainid_e lpc313x_clkdomain(enum lpc313x_clockid_e clkid);

/************************************************************************
 * Name: lp313x_esrndx
 *
 * Description:
 *   Given a clock ID, return the index of the corresponding ESR
 *   register (or ESRNDX_INVALID if there is no ESR associated with
 *   this clock ID).  Indexing of ESRs differs slightly from the clock
 *   ID:  There are 92 clock IDs but only 89 ESR regisers. There are no
 *  ESR registers for :
 *
 *
 *  CLKID_I2SRXBCK0         Clock ID 87: I2SRX_BCK0
 *  CLKID_I2SRXBCK1,        Clock ID 88: I2SRX_BCK1
 *
 * and
 *
 *  CLKID_SYSCLKO           Clock ID 91: SYSCLK_O
 *
 ************************************************************************/

EXTERN int lp313x_esrndx(enum lpc313x_clockid_e clkid);

/************************************************************************
 * Name: lpc313x_fdcndx
 *
 * Description:
 *   Given a clock ID and its domain ID, return the index of the
 *   corresponding fractional divider register (or FDCNDX_INVALID if
 *   there is no fractional divider associated with this clock).
 *
 ************************************************************************/

EXTERN int lpc313x_fdcndx(enum lpc313x_clockid_e clkid,
                          enum lpc313x_domainid_e dmnid);

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* __ASSEMBLY__ */
#endif /* __ARCH_ARM_SRC_LPC313X_CGUDRVR_H */
