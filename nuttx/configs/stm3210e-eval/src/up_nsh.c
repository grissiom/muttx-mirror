/****************************************************************************
 * config/stm3210e_eval/src/up_nsh.c
 * arch/arm/src/board/up_nsh.c
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

#include <stdio.h>
#include <debug.h>
#include <errno.h>

#ifdef CONFIG_STM32_SPI1
#  include <nuttx/spi.h>
#  include <nuttx/mtd.h>
#endif
#include <nuttx/mmcsd.h>

/****************************************************************************
 * Pre-Processor Definitions
 ****************************************************************************/

/* Configuration ************************************************************/

/* For now, don't build in any SPI1 support -- NSH is not using it */

#undef CONFIG_STM32_SPI1

/* PORT and SLOT number probably depend on the board configuration */

/* Can't support USB features if USB is not enabled */

#ifndef CONFIG_USBDEV
#  undef CONFIG_EXAMPLES_NSH_HAVEUSBDEV
#endif

#if defined(CONFIG_EXAMPLES_NSH_MMCSDSLOTNO) && CONFIG_EXAMPLES_NSH_MMCSDSLOTNO != 0
#  error "Only one MMC/SD slot"
#  undef CONFIG_EXAMPLES_NSH_MMCSDSLOTNO
#endif

/* Can't support MMC/SD features if mountpoints are disabled */

#if defined(CONFIG_DISABLE_MOUNTPOINT)
#  undef CONFIG_EXAMPLES_NSH_HAVEMMCSD
#endif

#ifndef CONFIG_EXAMPLES_NSH_MMCSDMINOR
#  define CONFIG_EXAMPLES_NSH_MMCSDMINOR 0
#endif

/* Debug ********************************************************************/

#ifdef CONFIG_CPP_HAVE_VARARGS
#  ifdef CONFIG_DEBUG
#    define message(...) lib_lowprintf(__VA_ARGS__)
#  else
#    define message(...) printf(__VA_ARGS__)
#  endif
#else
#  ifdef CONFIG_DEBUG
#    define message lib_lowprintf
#  else
#    define message printf
#  endif
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nsh_archinitialize
 *
 * Description:
 *   Perform architecture specific initialization
 *
 ****************************************************************************/

int nsh_archinitialize(void)
{
#ifdef CONFIG_STM32_SPI1
  FAR struct spi_dev_s *spi;
  FAR struct mtd_dev_s *mtd;
#endif

  /* Configure SPI-based devices */

#ifdef CONFIG_STM32_SPI1
  /* Get the SPI port */

  message("nsh_archinitialize: Initializing SPI port 0\n");
  spi = up_spiinitialize(0);
  if (!spi)
    {
      message("nsh_archinitialize: Failed to initialize SPI port 0\n");
      return -ENODEV;
    }
  message("nsh_archinitialize: Successfully initialized SPI port 0\n");

  /* Now bind the SPI interface to the M25P64/128 SPI FLASH driver */

  message("nsh_archinitialize: Bind SPI to the SPI flash driver\n");
  mtd = m25p_initialize(spi);
  if (!mtd)
    {
      message("nsh_archinitialize: Failed to bind SPI port 0 to the SPI FLASH driver\n");
      return -ENODEV;
    }
  message("nsh_archinitialize: Successfully bound SPI port 0 to the SPI FLASH driver\n");
#warning "Now what are we going to do with this SPI FLASH driver?"
#endif

  /* Create the SPI FLASH MTD instance */

  /* Here we will eventually need to
   * 1. Get the SDIO interface instance, and 
   * 2. Bind it to the MMC/SD driver (slot CONFIG_EXAMPLES_NSH_MMCSDSLOTNO,
   *    CONFIG_EXAMPLES_NSH_MMCSDMINOR).
   */

#warning "Missing MMC/SD device configuration"
  return OK;
}