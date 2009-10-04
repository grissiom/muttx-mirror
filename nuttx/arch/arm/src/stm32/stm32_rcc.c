/****************************************************************************
 * arch/arm/src/stm32/stm32_rcc.c
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

#include <debug.h>
#include <arch/board/board.h>

#include "up_internal.h"
#include "up_arch.h"

#include "chip.h"
#include "stm32_rcc.h"
#include "stm32_internal.h"

/****************************************************************************
 * Definitions
 ****************************************************************************/

#define HSERDY_TIMEOUT 256

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/* Put all RCC registers in reset state */

static inline void rcc_reset(void)
{
  uint32 regval;

  putreg32(0, STM32_RCC_APB2RSTR);			/* Disable APB2 Peripheral Reset */
  putreg32(0, STM32_RCC_APB1RSTR);			/* Disable APB1 Peripheral Reset */
  putreg32(RCC_AHBENR_FLITFEN|RCC_AHBENR_SRAMEN, STM32_RCC_AHBENR);	/* FLITF and SRAM Clock ON */
  putreg32(0, STM32_RCC_APB2ENR);			/* Disable APB2 Peripheral Clock */
  putreg32(0, STM32_RCC_APB1ENR);			/* Disable APB1 Peripheral Clock */

  regval  = getreg32(STM32_RCC_CR);			/* Set the HSION bit */
  regval |= RCC_CR_HSION;
  putreg32(regval, STM32_RCC_CR);

  regval  = getreg32(STM32_RCC_CFGR);		/* Reset SW, HPRE, PPRE1, PPRE2, ADCPRE and MCO bits */
  regval &= ~(RCC_CFGR_SW_MASK|RCC_CFGR_HPRE_MASK|RCC_CFGR_PPRE1_MASK|RCC_CFGR_PPRE2_MASK|RCC_CFGR_ADCPRE_MASK|RCC_CFGR_MCO_MASK);
  putreg32(regval, STM32_RCC_CFGR);

  regval  = getreg32(STM32_RCC_CR);			/* Reset HSEON, CSSON and PLLON bits */
  regval &= ~(RCC_CR_HSEON|RCC_CR_CSSON|RCC_CR_PLLON);
  putreg32(regval, STM32_RCC_CR);

  regval  = getreg32(STM32_RCC_CR);			/* Reset HSEBYP bit */
  regval &= ~RCC_CR_HSEBYP;
  putreg32(regval, STM32_RCC_CR);
 
  regval  = getreg32(STM32_RCC_CFGR);		/* Reset PLLSRC, PLLXTPRE, PLLMUL and USBPRE bits */
  regval &= ~(RCC_CFGR_PLLSRC|RCC_CFGR_PLLXTPRE|RCC_CFGR_PLLMUL_MASK|RCC_CFGR_USBPRE);
  putreg32(regval, STM32_RCC_CFGR);

  putreg32(0, STM32_RCC_CIR);				/* Disable all interrupts */
}

static inline void rcc_enableapb1(void)
{
  uint32 regval;

  regval  = getreg32(STM32_RCC_APB1ENR);
#if CONFIG_STM32_TIM2
  /* Timer 2 clock enable */

  regval |= RCC_APB1ENR_TIM2EN;
#endif

#if CONFIG_STM32_TIM3
  /* Timer 3 clock enable */

  regval |= RCC_APB1ENR_TIM3EN;
#endif

#if CONFIG_STM32_TIM4
  /* Timer 4 clock enable */

  regval |= RCC_APB1ENR_TIM4EN;
#endif

#if CONFIG_STM32_TIM5
  /* Timer 5 clock enable */

  regval |= RCC_APB1ENR_TIM5EN;
#endif

#if CONFIG_STM32_TIM6
  /* Timer 6 clock enable */

  regval |= RCC_APB1ENR_TIM6EN;
#endif

#if CONFIG_STM32_TIM7
  /* Timer 7 clock enable */

  regval |= RCC_APB1ENR_TIM7EN;
#endif

#if CONFIG_STM32_WWDG
  /* Window Watchdog clock enable */

  regval |= RCC_APB1ENR_WWDGEN;
#endif

#if CONFIG_STM32_SPI2
  /* SPI 2 clock enable */

  regval |= RCC_APB1ENR_SPI2EN;
#endif
  
#if CONFIG_STM32_SPI4
  /* SPI 3 clock enable */

  regval |= RCC_APB1ENR_SPI3EN;
#endif

#if CONFIG_STM32_USART2
  /* USART 2 clock enable */

  regval |= RCC_APB1ENR_USART2EN;
#endif

#if CONFIG_STM32_USART3
  /* USART 3 clock enable */

  regval |= RCC_APB1ENR_USART3EN;
#endif

#if CONFIG_STM32_UART4
  /* UART 4 clock enable */

  regval |= RCC_APB1ENR_UART4EN;
#endif

#if CONFIG_STM32_UART5
  /* UART 5 clock enable */

  regval |= RCC_APB1ENR_UART5EN;
#endif

#if CONFIG_STM32_I2C1
  /* I2C 1 clock enable */

  regval |= RCC_APB1ENR_I2C1EN;
#endif

#if CONFIG_STM32_I2C2
  /* I2C 2 clock enable */

  regval |= RCC_APB1ENR_I2C2EN;
#endif

#if CONFIG_STM32_USB
  /* USB clock enable */

  regval |= RCC_APB1ENR_USBEN;
#endif

#if CONFIG_STM32_CAN
  /* CAN clock enable */

  regval |= RCC_APB1ENR_CANEN;
#endif

#if CONFIG_STM32_BKP
  /* Backup interface clock enable */

  regval |= RCC_APB1ENR_BKPEN;
#endif

#if CONFIG_STM32_PWR
  /*  Power interface clock enable */

  regval |= RCC_APB1ENR_PWREN;
#endif

#if CONFIG_STM32_DAC
  /* DAC interface clock enable */

  regval |= RCC_APB1ENR_DACEN;
#endif
  putreg32(regval, STM32_RCC_APB1ENR);

#if CONFIG_STM32_USB
  /* USB clock divider */

  regval  = getreg32(STM32_RCC_CFGR);
  regval &= ~RCC_CFGR_USBPRE;
  regval |= STM32_CFGR_USBPRE;
  putreg32(regval, STM32_RCC_CFGR);
#endif
}

static inline void rcc_enableapb2(void)
{
  uint32 regval;

  /* Enable GPIOA, GPIOB, ... and AFIO clocks */

  regval = getreg32(STM32_RCC_APB2ENR);
  regval |= (RCC_APB2ENR_AFIOEN
#if STM32_NGPIO > 0
             |RCC_APB2ENR_IOPAEN
#endif
#if STM32_NGPIO > 16
             |RCC_APB2ENR_IOPBEN
#endif
#if STM32_NGPIO > 32
             |RCC_APB2ENR_IOPCEN
#endif
#if STM32_NGPIO > 48
             |RCC_APB2ENR_IOPDEN
#endif
#if STM32_NGPIO > 64
             |RCC_APB2ENR_IOPEEN
#endif
#if STM32_NGPIO > 80
             |RCC_APB2ENR_IOPFEN
#endif
#if STM32_NGPIO > 96
             |RCC_APB2ENR_IOPGEN
#endif
             );

#if CONFIG_STM32_ADC1
  /* ADC 1 interface clock enable */

  regval |= RCC_APB2ENR_ADC1EN;
#endif

#if CONFIG_STM32_ADC2
  /* ADC 2 interface clock enable */

  regval |= RCC_APB2ENR_ADC2EN;
#endif

#if CONFIG_STM32_TIM1
  /* TIM1 Timer clock enable */

  regval |= RCC_APB2ENR_TIM1EN;
#endif

#if CONFIG_STM32_SPI1
  /* SPI 1 clock enable */

  regval |= RCC_APB2ENR_SPI1EN;
#endif

#if CONFIG_STM32_TIM8
  /* TIM8 Timer clock enable */

  regval |= RCC_APB2ENR_TIM8EN;
#endif

#if CONFIG_STM32_USART1
  /* USART1 clock enable */

  regval |= RCC_APB2ENR_USART1EN;
#endif

#if CONFIG_STM32_ADC3
  /*ADC3 interface clock enable */

  regval |= RCC_APB2ENR_ADC3EN;
#endif
  putreg32(regval, STM32_RCC_APB2ENR);
}

/****************************************************************************
 * Global Functions
 ****************************************************************************/

/****************************************************************************
 * Name: stm32_clockconfig
 *
 * Description:
 *   Called to change to new clock based on settings in board.h.
 *   NOTE:  This logic needs to be extended so that we can selected low-power
 *   clocking modes as well!
 *
 ****************************************************************************/

void stm32_clockconfig(void)
{
  uint32 regval;
  sint32 timeout;

  /* Make sure that we are starting in the reset state */

  rcc_reset();

  /* Enable External High-Speed Clock (HSE) */
 
  regval  = getreg32(STM32_RCC_CR);
  regval &= ~RCC_CR_HSEBYP;			/* Disable HSE clock bypass */
  regval |= RCC_CR_HSEON;			/* Enable HSE */
  putreg32(regval, STM32_RCC_CR);
   
  /* Wait until the HSE is ready (or until a timeout elapsed) */

  for (timeout = HSERDY_TIMEOUT; timeout > 0; timeout--)
  {
    /* Check if the HSERDY flag is the set in the CR */

    if ((getreg32(STM32_RCC_CR) & RCC_CR_HSERDY) != 0)
      {
        /* If so, then break-out with timeout > 0 */

        break;
      }
  }

  if( timeout > 0)
    {
#if 0
      /* Enable Prefetch Buffer */

      /* Flash 2 wait state */
#endif

    /* Set the HCLK source/divider */
 
    regval = getreg32(STM32_RCC_CFGR);
    regval &= ~RCC_CFGR_HPRE_MASK;
    regval |= STM32_RCC_CFGR_HPRE;
    putreg32(regval, STM32_RCC_CFGR);

    /* Set the PCLK2 divider */

    regval = getreg32(STM32_RCC_CFGR);
    regval &= ~RCC_CFGR_PPRE2_MASK;
    regval |= STM32_RCC_CFGR_PPRE2;
    putreg32(regval, STM32_RCC_CFGR);
  
    /* Set the PCLK1 divider */

    regval = getreg32(STM32_RCC_CFGR);
    regval &= ~RCC_CFGR_PPRE1_MASK;
    regval |= STM32_RCC_CFGR_PPRE1;
    putreg32(regval, STM32_RCC_CFGR);
 
    /* Set the PLL divider and multipler */

    regval = getreg32(STM32_RCC_CFGR);
    regval &= ~(RCC_CFGR_PLLSRC|RCC_CFGR_PLLXTPRE|RCC_CFGR_PLLMUL_MASK);
    regval |= (STM32_CFGR_PLLSRC|STM32_CFGR_PLLXTPRE|STM32_CFGR_PLLMUL);
    putreg32(regval, STM32_RCC_CFGR);
 
    /* Enable the PLL */

    regval = getreg32(STM32_RCC_CR);
    regval |= RCC_CR_PLLON;
    putreg32(regval, STM32_RCC_CR);
 
    /* Wait until the PLL is ready */
  
    while ((getreg32(STM32_RCC_CR) & RCC_CR_PLLRDY) == 0);
 
    /* Select the system clock source (probably the PLL) */
 
    regval  = getreg32(STM32_RCC_CFGR);
    regval &= ~RCC_CFGR_SW_MASK;
    regval |= STM32_SYSCLK_SW;
    putreg32(regval, STM32_RCC_CFGR);

    /* Wait until the selected source is used as the system clock source */
  
    while ((getreg32(STM32_RCC_CFGR) & RCC_CFGR_SWS_MASK) != STM32_SYSCLK_SWS);
  }

  /* Enable periperal clocking */

  rcc_enableapb2();
  rcc_enableapb1();
}
