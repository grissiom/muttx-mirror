/************************************************************************************
 * arch/arm/src/stm32/stm32_dma.h
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
 ************************************************************************************/

#ifndef __ARCH_ARM_SRC_STM32_STM32_DMA_H
#define __ARCH_ARM_SRC_STM32_STM32_DMA_H

/************************************************************************************
 * Included Files
 ************************************************************************************/

#include <nuttx/config.h>
#include <sys/types.h>
#include "chip.h"

/************************************************************************************
 * Definitions
 ************************************************************************************/

/* 7 DMA Channels */

#define DMA1 0
#define DMA2 1
#define DMA3 2
#define DMA4 3
#define DMA5 4
#define DMA6 5
#define DMA7 6

/* Register Offsets *****************************************************************/

#define STM32_DMA_ISR_OFFSET      0x0000 /* DMA interrupt status register */
#define STM32_DMA_IFCR_OFFSET     0x0004 /* DMA interrupt flag clear register */

#define STM32_DMA_CCR_OFFSET(n)   (0x0008+0x0014*(n))
#define STM32_DMA_CCR1_OFFSET     0x0008 /* DMA channel 1 configuration register */
#define STM32_DMA_CCR2_OFFSET     0x001c /* DMA channel 2 configuration register */
#define STM32_DMA_CCR3_OFFSET     0x0030 /* DMA channel 3 configuration register */
#define STM32_DMA_CCR4_OFFSET     0x0044 /* DMA channel 4 configuration register */
#define STM32_DMA_CCR5_OFFSET     0x0058 /* DMA channel 5 configuration register */
#define STM32_DMA_CCR6_OFFSET     0x006c /* DMA channel 6 configuration register */
#define STM32_DMA_CCR7_OFFSET     0x0080 /* DMA channel 7 configuration register */

#define STM32_DMA_CNDTR_OFFSET(n) (0x000c+0x0014*(n))
#define STM32_DMA_CNDTR1_OFFSET   0x000c /* DMA channel 1 number of data register */
#define STM32_DMA_CNDTR2_OFFSET   0x0020 /* DMA channel 2 number of data register */
#define STM32_DMA_CNDTR3_OFFSET   0x0034 /* DMA channel 3 number of data register */
#define STM32_DMA_CNDTR4_OFFSET   0x0048 /* DMA channel 4 number of data register */
#define STM32_DMA_CNDTR5_OFFSET   0x005c /* DMA channel 5 number of data register */
#define STM32_DMA_CNDTR6_OFFSET   0x0070 /* DMA channel 6 number of data register */
#define STM32_DMA_CNDTR7_OFFSET   0x0084 /* DMA channel 7 number of data register */

#define STM32_DMA_CPAR_OFFSET(n)  (0x0010+0x0014*(n))
#define STM32_DMA_CPAR1_OFFSET    0x0010 /* DMA channel 1 peripheral address register */
#define STM32_DMA_CPAR2_OFFSET    0x0024 /* DMA channel 2 peripheral address register */
#define STM32_DMA_CPAR3_OFFSET    0x0038 /* DMA channel 3 peripheral address register */
#define STM32_DMA_CPAR4_OFFSET    0x004c /* DMA channel 4 peripheral address register */
#define STM32_DMA_CPAR5_OFFSET    0x0060 /* DMA channel 5 peripheral address register */
#define STM32_DMA_CPAR6_OFFSET    0x0074 /* DMA channel 6 peripheral address register */
#define STM32_DMA_CPAR7_OFFSET    0x0088 /* DMA channel 7 peripheral address register */

#define STM32_DMA_CMAR_OFFSET(n)  (0x0014+0x0014*(n))
#define STM32_DMA_CMAR1_OFFSET    0x0014 /* DMA channel 1 memory address register */
#define STM32_DMA_CMAR2_OFFSET    0x0028 /* DMA channel 2 memory address register */
#define STM32_DMA_CMAR3_OFFSET    0x003c /* DMA channel 3 memory address register */
#define STM32_DMA_CMAR4_OFFSET    0x0050 /* DMA channel 4 memory address register */
#define STM32_DMA_CMAR5_OFFSET    0x0064 /* DMA channel 5 memory address register */
#define STM32_DMA_CMAR6_OFFSET    0x0078 /* DMA channel 6 memory address register */
#define STM32_DMA_CMAR7_OFFSET    0x008c /* DMA channel 7 memory address register */

/* Register Addresses ***************************************************************/

#define STM32_DMA_ISRC            (STM32_DMA_BASE+STM32_DMA_ISRC_OFFSET)
#define STM32_DMA_IFCR            (STM32_DMA_BASE+STM32_DMA_IFCR_OFFSET)

#define STM32_DMA_CCR(n)          (STM32_DMA_BASE+STM32_DMA_CCR_OFFSET(n))
#define STM32_DMA_CCR1            (STM32_DMA_BASE+STM32_DMA_CCR1_OFFSET)
#define STM32_DMA_CCR2            (STM32_DMA_BASE+STM32_DMA_CCR2_OFFSET)
#define STM32_DMA_CCR3            (STM32_DMA_BASE+STM32_DMA_CCR3_OFFSET)
#define STM32_DMA_CCR4            (STM32_DMA_BASE+STM32_DMA_CCR4_OFFSET)
#define STM32_DMA_CCR5            (STM32_DMA_BASE+STM32_DMA_CCR5_OFFSET)
#define STM32_DMA_CCR6            (STM32_DMA_BASE+STM32_DMA_CCR6_OFFSET)
#define STM32_DMA_CCR7            (STM32_DMA_BASE+STM32_DMA_CCR7_OFFSET)

#define STM32_DMA_CNDTR(n)        (STM32_DMA_BASE+STM32_DMA_CNDTR_OFFSET(n))
#define STM32_DMA_CNDTR1          (STM32_DMA_BASE+STM32_DMA_CNDTR1_OFFSET)
#define STM32_DMA_CNDTR2          (STM32_DMA_BASE+STM32_DMA_CNDTR2_OFFSET)
#define STM32_DMA_CNDTR3          (STM32_DMA_BASE+STM32_DMA_CNDTR3_OFFSET)
#define STM32_DMA_CNDTR4          (STM32_DMA_BASE+STM32_DMA_CNDTR4_OFFSET)
#define STM32_DMA_CNDTR5          (STM32_DMA_BASE+STM32_DMA_CNDTR5_OFFSET)
#define STM32_DMA_CNDTR6          (STM32_DMA_BASE+STM32_DMA_CNDTR6_OFFSET)
#define STM32_DMA_CNDTR7          (STM32_DMA_BASE+STM32_DMA_CNDTR7_OFFSET)

#define STM32_DMA_CPAR(n)         (STM32_DMA_BASE+STM32_DMA_CPAR_OFFSET(n))
#define STM32_DMA_CPAR1           (STM32_DMA_BASE+STM32_DMA_CPAR1_OFFSET)
#define STM32_DMA_CPAR2           (STM32_DMA_BASE+STM32_DMA_CPAR2_OFFSET)
#define STM32_DMA_CPAR3           (STM32_DMA_BASE+STM32_DMA_CPAR3_OFFSET)
#define STM32_DMA_CPAR4           (STM32_DMA_BASE+STM32_DMA_CPAR4_OFFSET)
#define STM32_DMA_CPAR5           (STM32_DMA_BASE+STM32_DMA_CPAR5_OFFSET)
#define STM32_DMA_CPAR6           (STM32_DMA_BASE+STM32_DMA_CPAR6_OFFSET)
#define STM32_DMA_CPAR7           (STM32_DMA_BASE+STM32_DMA_CPAR7_OFFSET)

#define STM32_DMA_CMAR(n)         (STM32_DMA_BASE+STM32_DMA_CMAR_OFFSET(n))
#define STM32_DMA_CMAR1           (STM32_DMA_BASE+STM32_DMA_CMAR1_OFFSET)
#define STM32_DMA_CMAR2           (STM32_DMA_BASE+STM32_DMA_CMAR2_OFFSET)
#define STM32_DMA_CMAR3           (STM32_DMA_BASE+STM32_DMA_CMAR3_OFFSET)
#define STM32_DMA_CMAR4           (STM32_DMA_BASE+STM32_DMA_CMAR4_OFFSET)
#define STM32_DMA_CMAR5           (STM32_DMA_BASE+STM32_DMA_CMAR5_OFFSET)
#define STM32_DMA_CMAR6           (STM32_DMA_BASE+STM32_DMA_CMAR6_OFFSET)
#define STM32_DMA_CMAR7           (STM32_DMA_BASE+STM32_DMA_CMAR7_OFFSET)

/* Register Bitfield Definitions ****************************************************/

/* DMA interrupt status register */

#define DMA_ISRC_CHAN_SHIFT(n)    (4*(n))
#define DMA_ISRC_CHAN_MASK(n)     (0x0f <<  DMA_ISRC_CHAN_SHIFT(n))
#define DMA_ISRC_CHAN1_SHIFT      (0)       /* Bits 3-0:  DMA Channel 1 interrupt status */
#define DMA_ISRC_CHAN1_MASK       (0x0f <<  DMA_ISRC_CHAN1_SHIFT)
#define DMA_ISRC_CHAN2_SHIFT      (4)       /* Bits 7-4:  DMA Channel 2 interrupt status */
#define DMA_ISRC_CHAN2_MASK       (0x0f <<  DMA_ISRC_CHAN2_SHIFT)
#define DMA_ISRC_CHAN3_SHIFT      (8)       /* Bits 11-8:  DMA Channel 3 interrupt status */
#define DMA_ISRC_CHAN3_MASK       (0x0f <<  DMA_ISRC_CHAN3_SHIFT)
#define DMA_ISRC_CHAN4_SHIFT      (12)      /* Bits 15-12:  DMA Channel 4 interrupt status */
#define DMA_ISRC_CHAN4_MASK       (0x0f <<  DMA_ISRC_CHAN4_SHIFT)
#define DMA_ISRC_CHAN5_SHIFT      (16)      /* Bits 19-16:  DMA Channel 5 interrupt status */
#define DMA_ISRC_CHAN5_MASK       (0x0f <<  DMA_ISRC_CHAN5_SHIFT)
#define DMA_ISRC_CHAN6_SHIFT      (20)      /* Bits 23-20:  DMA Channel 6 interrupt status */
#define DMA_ISRC_CHAN6_MASK       (0x0f <<  DMA_ISRC_CHAN6_SHIFT)
#define DMA_ISRC_CHAN7_SHIFT      (24)      /* Bits 27-24:  DMA Channel 7 interrupt status */
#define DMA_ISRC_CHAN7_MASK       (0x0f <<  DMA_ISRC_CHAN7_SHIFT)

#define DMA_ISRC_GIF_BIT          (1 << 0)  /* Bit 0: Channel Global interrupt flag */
#define DMA_ISRC_GIF(n)           (DMA_ISRC_GIF_BIT << DMA_ISRC_CHAN_SHIFT(n))
#define DMA_ISRC_TCIF_BIT         (1 << 1)  /* Bit 1: Channel Transfer Complete flag */
#define DMA_ISRC_TCIF(n)          (DMA_ISRC_TCIF_BIT << DMA_ISRC_CHAN_SHIFT(n))
#define DMA_ISRC_HTIF_BIT         (1 << 2)  /* Bit 2: Channel Half Transfer flag */
#define DMA_ISRC_HTIF(n)          (DMA_ISRC_HTIF_BIT  << DMA_ISRC_CHAN_SHIFT(n))
#define DMA_ISRC_TEIF_BIT         (1 << 3)  /* Bit 3: Channel Transfer Error flag */
#define DMA_ISRC_TEIF(n)          (DMA_ISRC_TEIF_BIT << DMA_ISRC_CHAN_SHIFT(n))

/* DMA interrupt flag clear register */

#define DMA_IFCR_CHAN_SHIFT(n)    (4*(n))
#define DMA_IFCR_CHAN_MASK(n)     (0x0f <<  DMA_IFCR_CHAN_SHIFT(n))
#define DMA_IFCR_CHAN1_SHIFT      (0)       /* Bits 3-0:  DMA Channel 1 interrupt flag clear */
#define DMA_IFCR_CHAN1_MASK       (0x0f <<  DMA_IFCR_CHAN1_SHIFT)
#define DMA_IFCR_CHAN2_SHIFT      (4)       /* Bits 7-4:  DMA Channel 2 interrupt flag clear */
#define DMA_IFCR_CHAN2_MASK       (0x0f <<  DMA_IFCR_CHAN2_SHIFT)
#define DMA_IFCR_CHAN3_SHIFT      (8)       /* Bits 11-8:  DMA Channel 3 interrupt flag clear */
#define DMA_IFCR_CHAN3_MASK       (0x0f <<  DMA_IFCR_CHAN3_SHIFT)
#define DMA_IFCR_CHAN4_SHIFT      (12)      /* Bits 15-12:  DMA Channel 4 interrupt flag clear */
#define DMA_IFCR_CHAN4_MASK       (0x0f <<  DMA_IFCR_CHAN4_SHIFT)
#define DMA_IFCR_CHAN5_SHIFT      (16)      /* Bits 19-16:  DMA Channel 5 interrupt flag clear */
#define DMA_IFCR_CHAN5_MASK       (0x0f <<  DMA_IFCR_CHAN5_SHIFT)
#define DMA_IFCR_CHAN6_SHIFT      (20)      /* Bits 23-20:  DMA Channel 6 interrupt flag clear */
#define DMA_IFCR_CHAN6_MASK       (0x0f <<  DMA_IFCR_CHAN6_SHIFT)
#define DMA_IFCR_CHAN7_SHIFT      (24)      /* Bits 27-24:  DMA Channel 7 interrupt flag clear */
#define DMA_IFCR_CHAN7_MASK       (0x0f <<  DMA_IFCR_CHAN7_SHIFT)

#define DMA_IFCR_CGIF_BIT         (1 << 0)  /* Bit 0: Channel Global interrupt clear */
#define DMA_IFCR_CGIF(n)          (DMA_IFCR_CGIF_BIT << DMA_IFCR_CHAN_SHIFT(n))
#define DMA_IFCR_CTCIF_BIT        (1 << 1)  /* Bit 1: Channel Transfer Complete clear */
#define DMA_IFCR_CTCIF(n)         (DMA_IFCR_CTCIF_BIT << DMA_IFCR_CHAN_SHIFT(n))
#define DMA_IFCR_CHTIF_BIT        (1 << 2)  /* Bit 2: Channel Half Transfer clear */
#define DMA_IFCR_CHTIF(n)         (DMA_IFCR_CHTIF_BIT  << DMA_IFCR_CHAN_SHIFT(n))
#define DMA_IFCR_CTEIF_BIT        (1 << 3)  /* Bit 3: Channel Transfer Error clear */
#define DMA_IFCR_CTEIF(n)         (DMA_IFCR_CTEIF_BIT << DMA_IFCR_CHAN_SHIFT(n))

/* DMA channel configuration register */

#define DMA_CCR_MEM2MEM           (1 << 14) /* Bit 14: Memory to memory mode */
#define DMA_CCR_PL_SHIFT          (12)      /* Bits 13-12: Channel Priority level */
#define DMA_CCR_PL_MASK           (3 << DMA_CCR_PL_SHIFT)
#  define DMA_CCR_PRILO           (0 << DMA_CCR_PL_SHIFT) /* 00: Low */
#  define DMA_CCR_PRIMED          (1 << DMA_CCR_PL_SHIFT) /* 01: Medium */
#  define DMA_CCR_PRIHI           (2 << DMA_CCR_PL_SHIFT) /* 10: High */
#  define DMA_CCR_PRIVERYHI       (3 << DMA_CCR_PL_SHIFT) /* 11: Very high */
#define DMA_CCR_MSIZE_SHIFT       (10)      /* Bits 11-10: Memory size */
#define DMA_CCR_MSIZE_MASK        (3 << DMA_CCR_MSIZE_SHIFT)
#  define DMA_CCR_MSIZE_8BITS     (0 << DMA_CCR_MSIZE_SHIFT) /* 00: 8-bits */
#  define DMA_CCR_MSIZE_16BITS    (1 << DMA_CCR_MSIZE_SHIFT) /* 01: 16-bits */
#  define DMA_CCR_MSIZE_32BITS    (2 << DMA_CCR_MSIZE_SHIFT) /* 10: 32-bits */
#define DMA_CCR_PSIZE_SHIFT       (8)       /* Bits 9-8: Peripheral size */
#define DMA_CCR_PSIZE_MASK        (3 << DMA_CCR_PSIZE_SHIFT)
#  define DMA_CCR_PSIZE_8BITS     (0 << DMA_CCR_PSIZE_SHIFT) /* 00: 8-bits */
#  define DMA_CCR_PSIZE_16BITS    (1 << DMA_CCR_PSIZE_SHIFT) /* 01: 16-bits */
#  define DMA_CCR_PSIZE_32BITS    (2 << DMA_CCR_PSIZE_SHIFT) /* 10: 32-bits */
#define DMA_CCR_MINC              (1 << 7)  /* Bit 7: Memory increment mode */
#define DMA_CCR_PINC              (1 << 6)  /* Bit 6: Peripheral increment mode */
#define DMA_CCR_CIRC              (1 << 5)  /* Bit 5: Circular mode */
#define DMA_CCR_DIR               (1 << 4)  /* Bit 4: Data transfer direction */
#define DMA_CCR_TEIE              (1 << 3)  /* Bit 3: Transfer error interrupt enable */
#define DMA_CCR_HTIE              (1 << 2)  /* Bit 2: Half Transfer interrupt enable */
#define DMA_CCR_TCIE              (1 << 1)  /* Bit 1: Transfer complete interrupt enable */
#define DMA_CCR_EN                (1 << 0)  /* Bit 0: Channel enable */

/* DMA channel number of data register */

#define DMA_CNDTR_NDT_SHIFT       (0)       /* Bits 15-0: Number of data to Transfer */
#define DMA_CNDTR_NDT_MASK       (0xffff << DMA_CNDTR_NDT_SHIFT)

/************************************************************************************
 * Public Types
 ************************************************************************************/

/************************************************************************************
 * Public Data
 ************************************************************************************/

/************************************************************************************
 * Public Functions
 ************************************************************************************/

#endif /* __ARCH_ARM_SRC_STM32_STM32_DMA_H */
