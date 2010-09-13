/************************************************************************************
 * configs/ea3131/src/up_spi.c
 * arch/arm/src/board/up_spi.c
 *
 *   Copyright (C) 2009-2010 Gregory Nutt. All rights reserved.
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

/************************************************************************************
 * Included Files
 ************************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>
#include <stdbool.h>
#include <debug.h>

#include <nuttx/spi.h>
#include <arch/board/board.h>

#include "up_arch.h"
#include "chip.h"
#include "lpc313x_internal.h"
#include "ea3131_internal.h"

#ifdef CONFIG_LPC313X_SPI
#if 0 /* At present, EA3131 specific logic is hard-coded in the file lpc313x_spi.c
       * in arch/arm/src/lpc313x */

/************************************************************************************
 * Definitions
 ************************************************************************************/

/* Enables debug output from this file (needs CONFIG_DEBUG too) */

#undef SPI_DEBUG   /* Define to enable debug */
#undef SPI_VERBOSE /* Define to enable verbose debug */

#ifdef SPI_DEBUG
#  define spidbg  lldbg
#  ifdef SPI_VERBOSE
#    define spivdbg lldbg
#  else
#    define spivdbg(x...)
#  endif
#else
#  undef SPI_VERBOSE
#  define spidbg(x...)
#  define spivdbg(x...)
#endif

/************************************************************************************
 * Private Functions
 ************************************************************************************/

/************************************************************************************
 * Public Functions
 ************************************************************************************/

/************************************************************************************
 * Name: lpc313x_spiinitialize
 *
 * Description:
 *   Called to configure SPI chip select GPIO pins for the EA3131 board.
 *
 ************************************************************************************/

void weak_function lpc313x_spiinitialize(void)
{
  /* NOTE: Clocking for SPI has already been provided.
   *       Configurations of SPI pins is performed in lpc313x_spi.c.
   *       Here, we only initialize chip select pins unique to the board
   *       architecture.
   */

  gpio_outputhigh(LPC313X_IOCONFIG_SPI, SPINOR_CS);
}

/****************************************************************************
 * Name:  lpc313x_spiselect and lpc313x_spistatus
 *
 * Description:
 *   The external functions, lpc313x_spiselect and lpc313x_spistatus must be
 *   provided by board-specific logic.  They are implementations of the select
 *   and status methods of the SPI interface defined by struct spi_ops_s (see
 *   include/nuttx/spi.h). All other methods (including up_spiinitialize())
 *   are provided by common LPC313X logic.  To use this common SPI logic on your
 *   board:
 *
 *   1. Provide logic in lpc313x_boardinitialize() to configure SPI chip select
 *      pins.
 *   2. Provide lpc313x_spiselect() and lpc313x_spistatus() functions in your
 *      board-specific logic.  These functions will perform chip selection and
 *      status operations using GPIOs in the way your board is configured.
 *   3. Add a calls to up_spiinitialize() in your low level application
 *      initialization logic
 *   4. The handle returned by up_spiinitialize() may then be used to bind the
 *      SPI driver to higher level logic (e.g., calling
 *      mmcsd_spislotinitialize(), for example, will bind the SPI driver to
 *      the SPI MMC/SD driver).
 *
 ****************************************************************************/

void lpc313x_spiselect(FAR struct spi_dev_s *dev, enum spi_dev_e devid, bool selected)
{
  spidbg("devid: %d CS: %s\n", (int)devid, selected ? "assert" : "de-assert");

  if (devid == SPIDEV_FLASH)
    {
      if (selected)
        {
          gpio_outputlow(LPC313X_IOCONFIG_SPI, SPINOR_CS);
        }
      else
        {
          gpio_outputhigh(LPC313X_IOCONFIG_SPI, SPINOR_CS);
        }
    }
}

uint8_t lpc313x_spistatus(FAR struct spi_dev_s *dev, enum spi_dev_e devid)
{
  return SPI_STATUS_PRESENT;
}

#endif /* 0 */
#endif /* CONFIG_LPC313X_SPI  */

