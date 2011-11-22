/************************************************************************************
 * arch/arm/src/stm32/chip.h
 *
 *   Copyright (C) 2009, 2011 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
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

#ifndef __ARCH_ARM_SRC_STM32_CHIP_H
#define __ARCH_ARM_SRC_STM32_CHIP_H

/************************************************************************************
 * Included Files
 ************************************************************************************/

#include <nuttx/config.h>

/* Include the chip capabilities file */

#include <arch/stm32/chip.h>

/* Include the chip pin configuration file */

#if defined(CONFIG_STM32_STM32F10XX)
#  if defined(CONFIG_ARCH_CHIP_STM32F103ZET6) 
#    include "chip/stm32f103ze_pinmap.h"
#  elif defined(CONFIG_ARCH_CHIP_STM32F103RET6)
#    include "chip/stm32f103re_pinmap.h"
#  elif defined(CONFIG_ARCH_CHIP_STM32F107VC)
#    include "chip/stm32f107vc_pinmap.h"
#  else
#    error "Unsupported STM32F10XXX chip"
#  endif
#elif defined(CONFIG_STM32_STM32F40XX)
#  include "chip/stm32f40xxx_pinmap.h"
#else
#  error "Unsupported STM32 chip"
#endif

/* Include only the mchip emory map. */

#include "chip/stm32_memorymap.h"

/************************************************************************************
 * Pre-processor Definitions
 ************************************************************************************/

#endif /* __ARCH_ARM_SRC_STM32_CHIP_H */

