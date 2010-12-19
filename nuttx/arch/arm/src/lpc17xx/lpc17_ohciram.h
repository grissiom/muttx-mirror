/************************************************************************************
 * arch/arm/src/lpc17xx/lpc17_ohciram.h
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
 ************************************************************************************/

#ifndef __ARCH_ARM_SRC_LPC17XX_LPC17_OHCIRAM_H
#define __ARCH_ARM_SRC_LPC17XX_LPC17_OHCIRAM_H

/************************************************************************************
 * Included Files
 ************************************************************************************/

#include <nuttx/config.h>
#include "chip.h"
#include "lpc17_memorymap.h"

/************************************************************************************
 * Pre-processor Definitions
 ************************************************************************************/
/* Default, no-OHCI Case ************************************************************/
/* Assume that all of AHB SRAM will be available for heap. If this is not true, then
 * LPC17_BANK1_HEAPSIZE will be undefined but redefined below.
 */

#undef LPC17_BANK1_HEAPBASE
#undef LPC17_BANK1_HEAPSIZE
#ifdef LPC17_HAVE_BANK1
#  define LPC17_BANK1_HEAPBASE LPC17_SRAM_BANK1
#  define LPC17_BANK1_HEAPSIZE LPC17_BANK1_SIZE
#endif

/* Is networking enabled?  Is the LPC17xx Ethernet device enabled? Does this chip have
 * and Ethernet controlloer?  Yes... then we will replace the above default definitions.
 */

#if defined(CONFIG_USBHOST) && defined(CONFIG_LPC17_USBHOST) && LPC17_NUSBHOST > 0

/* OHCI RAM Configuration ***********************************************************/
/* Is AHB SRAM available? */

#ifndef LPC17_HAVE_BANK1
#  error "AHB SRAM Bank1 is not available for OHCI RAM"
#endif

/* OHCI/Heap Memory Allocation ******************************************************/
/* Configured Size of the region at the end of AHB SRAM BANK1 set set aside for the
 * OHCI. This size must fit within AHB SRAM Bank 1 and also be a multiple of 256
 * bytes.
 */

#ifndef CONFIG_USBHOST_OHCIRAM_SIZE
#  define CONFIG_USBHOST_OHCIRAM_SIZE LPC17_BANK1_SIZE
#endif

#if CONFIG_USBHOST_OHCIRAM_SIZE > LPC17_BANK1_SIZE
#  error "OHCI RAM size cannot exceed the size of AHB SRAM Bank 1"
#endif

#if (CONFIG_USBHOST_OHCIRAM_SIZE & 0xff) != 0
#  error "OHCI RAM size must be in multiples of 256 bytes"
#endif

/* Then position the OHCI RAM at the end of AHB SRAM Bank 1 */

#define LPC17_OHCIRAM_END  (LPC17_SRAM_BANK1 + LPC17_BANK1_SIZE)
#define LPC17_OHCIRAM_BASE (LPC17_OHCIRAM_END - CONFIG_USBHOST_OHCIRAM_SIZE)
#define LPC17_OHCIRAM_SIZE  CONFIG_USBHOST_OHCIRAM_SIZE

/* Determine is there is any meaningful space left at the beginning of AHB Bank 1
 * that could be added to the heap.
 */

#undef LPC17_BANK1_HEAPBASE
#undef LPC17_BANK1_HEAPSIZE
#if LPC17_OHCIRAM_SIZE < (LPC17_BANK1_SIZE-128)
#  define LPC17_BANK1_HEAPBASE LPC17_SRAM_BANK1
#  define LPC17_BANK1_HEAPSIZE (LPC17_BANK1_SIZE - LPC17_OHCIRAM_SIZE)
#endif

/* Numbers and Sizes of Things ******************************************************/
/* Fixed size of the OHCI control area */

#define LPC17_HCCA_SIZE 256

/* Fixed endpoint and transfer descriptor sizes */

#define LPC17_TD_SIZE   16
#define LPC17_ED_SIZE   16

/* Configurable number of user endpoint descriptors (EDs).  This number excludes
 * the control endpoint that is always allocated.
 */

#ifndef CONFIG_USBHOST_NEDS
#  define CONFIG_USBHOST_NEDS 2
#endif

/* Derived size of user endpoint descriptor (ED) memory. */

#define LPC17_EDFREE_SIZE (CONFIG_USBHOST_NEDS * LPC17_ED_SIZE)

/* Configurable number of descriptor buffer (TDBUFFER) */

#ifndef CONFIG_USBHOST_TDBUFFERS
#  define CONFIG_USBHOST_TDBUFFERS 1
#endif

#if CONFIG_USBHOST_TDBUFFERS < 1
#  error "At least one TD buffer is required"
#endif

/* Configurable size of a TD buffer */

#if CONFIG_USBHOST_TDBUFFERS > 0 && !defined(CONFIG_USBHOST_TDBUFSIZE)
#  define CONFIG_USBHOST_TDBUFSIZE 128
#endif
#define LPC17_TDBUFFER_SIZE (CONFIG_USBHOST_TDBUFFERS * CONFIG_USBHOST_TDBUFSIZE)

/* Configurable size of an IO buffer.  The number of IO buffers will be determined
 * by what is left at the end of the BANK1 memory setup aside of OHCI RAM.
 */

#ifndef CONFIG_USBHOST_IOBUFSIZE
#  define CONFIG_USBHOST_IOBUFSIZE 512
#endif

/* OHCI Memory Layout ***************************************************************/
/* Example:
 *  Hardware:
 *    LPC17_SRAM_BANK1            0x20008000
 *    LPC17_BANK1_SIZE            16384
 *
 *  Configuration:
 *    CONFIG_USBHOST_OHCIRAM_SIZE 1024
 *    CONFIG_USBHOST_NTDS         1
 *    CONFIG_USBHOST_NEDS         2
 *    CONFIG_USBHOST_TDBUFFERS    1
 *    CONFIG_USBHOST_TDBUFSIZE    128
 *    CONFIG_USBHOST_IOBUFSIZE    512
 *
 *  Sizes of things
 *    CONFIG_USBHOST_NEDS         2
 *    LPC17_EDFREE_SIZE           48
 *    LPC17_TDBUFFER_SIZE         128
 *    LPC17_TDBUFFER_SIZE         512
 *
 *  Memory Layout
 *    LPC17_OHCIRAM_END          (0x20008000 + 16384) = 0x2000c000
 *    LPC17_OHCIRAM_BASE         (0x2000c000 - 1024) = 0x2000bc00
 *    LPC17_OHCIRAM_SIZE          1024
 *    LPC17_BANK1_HEAPBASE        0x20008000
 *    LPC17_BANK1_HEAPSIZE       (16384 - 1024) = 15360
 *
 *    LPC17_HCCA_BASE             0x2000bc00
 *    LPC17_TDHEAD_ADDR           0x2000bd00
 *    LPC17_TDTAIL_ADDR           0x2000bd10
 *    LPC17_EDCTRL_ADDR           0x2000bd20
 *    LPC17_EDFREE_BASE           0x2000bd30
 *    LPC17_TDBUFFER_BASE         0x2000bd50
 *    LPC17_IOBUFFER_BASE         0x2000bdd0
 *    LPC17_IOBUFFERS            (0x2000c000 + 0x2000bdd0) / 512 = 560/512 = 1
 *
 *  Wasted memory:                560-512 = 48 bytes
 */

#define LPC17_HCCA_BASE     (LPC17_OHCIRAM_BASE)
#define LPC17_TDHEAD_ADDR   (LPC17_OHCIRAM_BASE + LPC17_HCCA_SIZE)
#define LPC17_TDTAIL_ADDR   (LPC17_TDHEAD_ADDR + LPC17_TD_SIZE)
#define LPC17_EDCTRL_ADDR   (LPC17_TDTAIL_ADDR + LPC17_TD_SIZE)
#define LPC17_EDFREE_BASE   (LPC17_EDCTRL_ADDR + LPC17_ED_SIZE)
#define LPC17_TDBUFFER_BASE (LPC17_EDFREE_BASE + LPC17_EDFREE_SIZE)
#define LPC17_IOBUFFER_BASE (LPC17_TDBUFFER_BASE + LPC17_TDBUFFER_SIZE)

/* Finally, use the remainder of the allocated OHCI for IO buffers */

#define LPC17_IOBUFFERS     ((LPC17_OHCIRAM_END - LPC17_IOBUFFER_BASE) / CONFIG_USBHOST_IOBUFSIZE)

/************************************************************************************
 * Public Types
 ************************************************************************************/

/************************************************************************************
 * Public Data
 ************************************************************************************/

/************************************************************************************
 * Public Functions
 ************************************************************************************/

#endif /* CONFIG_USBHOST && CONFIG_LPC17_USBHOST && LPC17_NUSBHOST > 0*/
#endif /* __ARCH_ARM_SRC_LPC17XX_LPC17_OHCIRAM_H */
