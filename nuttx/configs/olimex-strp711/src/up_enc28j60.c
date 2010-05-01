/****************************************************************************
 * configs/olimex-strp711/src/up_enc28j60.c
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

#include <stdint.h>
#include <stdio.h>
#include <debug.h>

#include <nuttx/spi.h>
#include <nuttx/enc28j60.h>

#include <arch/board/board.h>

#include "chip.h"
#include "up_arch.h"
#include "up_internal.h"

#ifdef CONFIG_NET_ENC28J60

/****************************************************************************
 * Definitions
 ****************************************************************************/

/* Configuration ************************************************************/

#ifndef CONFIG_STR71X_BSPI1
# error "Need CONFIG_STR71X_BSP1 in the configuration"
#endif

/* SPI Assumptions **********************************************************/

#define ENC28J60_SPI_PORTNO 1 /* On SPI1 */
#define ENC28J60_DEVNO      0 /* Only one ENC28J60 */
#define ENC28J60_IRQ        0 /* NEEDED!!!!!!!!!!!!!!!! */

/* Debug ********************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: up_netinitialize
 ****************************************************************************/

void up_netinitialize(void)
{
  FAR struct spi_dev_s *spi;
  int ret;

  /* Get the SPI port */

  nvdbg("up_netinitialize: Initializing SPI port %d\n", ENC28J60_SPI_PORTNO);

  spi = up_spiinitialize(ENC28J60_SPI_PORTNO);
  if (!spi)
    {
      ndbg("up_netinitialize: Failed to initialize SPI port %d\n",
           ENC28J60_SPI_PORTNO);
      return;
    }

  nvdbg("up_netinitialize: Successfully initialized SPI port %d\n",
        ENC28J60_SPI_PORTNO);

  /* Bind the SPI port to the ENC28J60 driver */

  nvdbg("up_netinitialize: Binding SPI port %d to ENC28J60 device %d\n",
        ENC28J60_SPI_PORTNO, ENC28J60_DEVNO);

#warning "Need to implement IRQ interrupt before we can use this"
  ret = enc_initialize(spi, ENC28J60_DEVNO, ENC28J60_IRQ);
  if (ret < 0)
    {
      ndbg("up_netinitialize: Failed to bind SPI port %d ENC28J60 device %d: %d\n",
           ENC28J60_SPI_PORTNO, ENC28J60_DEVNO, ret);
      return;
    }

  nvdbg("up_netinitialize: Successfuly bound SPI port %d ENC28J60 device %d\n",
        ENC28J60_SPI_PORTNO, ENC28J60_DEVNO);
}
#endif /* CONFIG_NET_ENC28J60 */
