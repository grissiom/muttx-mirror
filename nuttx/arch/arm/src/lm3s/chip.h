/************************************************************************************
 * arch/arm/src/lm3s/chip.h
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

#ifndef __ARCH_ARM_SRC_LM3S_CHIP_H
#define __ARCH_ARM_SRC_LM3S_CHIP_H

/************************************************************************************
 * Included Files
 ************************************************************************************/

#include <nuttx/config.h>
#include <sys/types.h>

/************************************************************************************
 * Definitions
 ************************************************************************************/

/* Get customizations for each supported chip (only the LM3S6918 right now) */

#ifdef CONFIG_ARCH_CHIP_LM3S6918
#  define LM3S_NUARTS          2  /* Two UART modules */
#  define LM3S_NI2C            2  /* Two I2C modules */
#  define LM3S_NSSI            2  /* Two SSI modules */
#  define LM3S_NETHCONTROLLERS 1  /* One ethenet controller */
#else
#  error "No Ethernet support for this LM3S chip"
#endif

/* Then get all of the register definitions */

#include "lm3s_memorymap.h"      /* Memory map */
#include "lm3s_syscontrol.h"     /* System control module */
#include "lm3s_gpio.h"           /* GPIO modules */
#include "lm3s_uart.h"           /* UART modules */
#include "lm3s_i2c.h"            /* I2C modules */
#include "lm3s_ssi.h"            /* SSI modules */
#include "lm3s_ethernet.h"       /* Ethernet MAC and PHY */
#include "lm3s_flash.h"          /* FLASH */

/************************************************************************************
 * Public Types
 ************************************************************************************/

/************************************************************************************
 * Public Data
 ************************************************************************************/

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#endif /* __ARCH_ARM_SRC_LM3S_CHIP_H */
