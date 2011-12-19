/****************************************************************************
 * arch/arm/src/stm32/stm32_pwm.c
 *
 *   Copyright (C) 2011 Gregory Nutt. All rights reserved.
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
#include <assert.h>
#include <errno.h>
#include <debug.h>

#include <nuttx/pwm.h>
#include <arch/board/board.h>

#include "up_internal.h"
#include "up_arch.h"

#include "chip.h"
#include "stm32_pwm.h"
#include "stm32_internal.h"

/* This module then only compiles if there is at least one enabled timer
 * intended for use with the PWM upper half driver.
 */

#if defined(CONFIG_STM32_TIM1_PWM)  || defined(CONFIG_STM32_TIM2_PWM)  || \
    defined(CONFIG_STM32_TIM3_PWM)  || defined(CONFIG_STM32_TIM4_PWM)  || \
    defined(CONFIG_STM32_TIM5_PWM)  || defined(CONFIG_STM32_TIM8_PWM)  || \
    defined(CONFIG_STM32_TIM9_PWM)  || defined(CONFIG_STM32_TIM10_PWM) || \
    defined(CONFIG_STM32_TIM11_PWM) || defined(CONFIG_STM32_TIM12_PWM) || \
    defined(CONFIG_STM32_TIM13_PWM) || defined(CONFIG_STM32_TIM14_PWM)

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
/* Debug ********************************************************************/

#ifdef CONFIG_DEBUG_PWM
#  define pwmdbg    dbg
#  define pwmvdbg   vdbg
#  define pwmlldbg  lldbg
#  define pwmllvdbg llvdbg
#else
#  define pwmdbg(x...)
#  define pwmvdbg(x...)
#  define pwmlldbg(x...)
#  define pwmllvdbg(x...)
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/
/* This structure representst the state of one PWM timer */

struct stm32_pwmtimer_s
{
  FAR const struct pwm_ops_s *ops;     /* PWM operations */
  uint8_t                     timid;   /* Timer ID {1,...,14} */
  uint8_t                     channel; /* Timer output channel: {1,..4} */
  uint8_t                     unused2;
  uint8_t                     unused3;
  uint32_t                    base;    /* The base address of the timer */
  uint32_t                    pincfg;  /* Output pin configuration */
  uint32_t                    pclk;    /* The frequency of the peripheral clock
                                        * that drives the timer module. */
};

/****************************************************************************
 * Static Function Prototypes
 ****************************************************************************/
/* Register access */

static uint16_t pwm_getreg(struct stm32_pwmtimer_s *priv, int offset);
static void pwm_putreg(struct stm32_pwmtimer_s *priv, int offset, uint16_t value);

/* PWM driver methods */

static int pwm_setup(FAR struct pwm_lowerhalf_s *dev);
static int pwm_shutdown(FAR struct pwm_lowerhalf_s *dev);
static int pwm_start(FAR struct pwm_lowerhalf_s *dev, FAR const struct pwm_info_s *info);
static int pwm_stop(FAR struct pwm_lowerhalf_s *dev);
static int pwm_ioctl(FAR struct pwm_lowerhalf_s *dev, int cmd, unsigned long arg);

/****************************************************************************
 * Private Data
 ****************************************************************************/
/* This is the list of lower half PWM driver methods used by the upper half driver */

static const struct pwm_ops_s g_pwmops =
{
  .setup      = pwm_setup,
  .shutdown   = pwm_shutdown,
  .start      = pwm_start,
  .stop       = pwm_stop,
  .ioctl      = pwm_ioctl,
};

#ifdef CONFIG_STM32_TIM1_PWM
static struct stm32_pwmtimer_s g_pwm1dev =
{
  .ops        = &g_pwmops,
  .timid      = 1,
  .channel    = CONFIG_STM32_TIM1_CHANNEL,
  .base       = STM32_TIM1_BASE,
  .pincfg     = PWM_TIM1_PINCFG,
  .pclk       = STM32_PCLK2_FREQUENCY,
};
#endif

#ifdef CONFIG_STM32_TIM2_PWM
static struct stm32_pwmtimer_s g_pwm2dev =
{
  .ops        = &g_pwmops,
  .timid      = 2,
  .channel    = CONFIG_STM32_TIM2_CHANNEL,
  .base       = STM32_TIM2_BASE,
  .pincfg     = PWM_TIM2_PINCFG,
  .pclk       = STM32_PCLK1_FREQUENCY,
};
#endif

#ifdef CONFIG_STM32_TIM3_PWM
static struct stm32_pwmtimer_s g_pwm3dev =
{
  .ops        = &g_pwmops,
  .timid      = 3,
  .channel    = CONFIG_STM32_TIM3_CHANNEL,
  .base       = STM32_TIM3_BASE,
  .pincfg     = PWM_TIM3_PINCFG,
  .pclk       = STM32_PCLK1_FREQUENCY,
};
#endif

#ifdef CONFIG_STM32_TIM4_PWM
static struct stm32_pwmtimer_s g_pwm4dev =
{
  .ops        = &g_pwmops,
  .timid      = 4,
  .channel    = CONFIG_STM32_TIM4_CHANNEL,
  .base       = STM32_TIM4_BASE,
  .pincfg     = PWM_TIM4_PINCFG,
  .pclk       = STM32_PCLK1_FREQUENCY,
};
#endif

#ifdef CONFIG_STM32_TIM5_PWM
static struct stm32_pwmtimer_s g_pwm5dev =
{
  .ops        = &g_pwmops,
  .timid      = 5,
  .channel    = CONFIG_STM32_TIM5_CHANNEL,
  .base       = STM32_TIM5_BASE,
  .pincfg     = PWM_TIM5_PINCFG,
  .pclk       = STM32_PCLK1_FREQUENCY,
};
#endif

#ifdef CONFIG_STM32_TIM8_PWM
static struct stm32_pwmtimer_s g_pwm8dev =
{
  .ops        = &g_pwmops,
  .timid      = 8,
  .channel    = CONFIG_STM32_TIM8_CHANNEL,
  .base       = STM32_TIM8_BASE,
  .pincfg     = PWM_TIM8_PINCFG,
  .pclk       = STM32_PCLK2_FREQUENCY,
};
#endif

#ifdef CONFIG_STM32_TIM9_PWM
static struct stm32_pwmtimer_s g_pwm9dev =
{
  .ops        = &g_pwmops,
  .timid      = 9,
  .channel    = CONFIG_STM32_TIM9_CHANNEL,
  .base       = STM32_TIM9_BASE,
  .pincfg     = PWM_TIM9_PINCFG,
  .pclk       = STM32_PCLK2_FREQUENCY,
};
#endif

#ifdef CONFIG_STM32_TIM10_PWM
static struct stm32_pwmtimer_s g_pwm10dev =
{
  .ops        = &g_pwmops,
  .timid      = 10,
  .channel    = CONFIG_STM32_TIM10_CHANNEL,
  .base       = STM32_TIM10_BASE,
  .pincfg     = PWM_TIM10_PINCFG,
  .pclk       = STM32_PCLK2_FREQUENCY,
};
#endif

#ifdef CONFIG_STM32_TIM11_PWM
static struct stm32_pwmtimer_s g_pwm11dev =
{
  .ops        = &g_pwmops,
  .timid      = 11,
  .channel    = CONFIG_STM32_TIM11_CHANNEL,
  .base       = STM32_TIM11_BASE,
  .pincfg     = PWM_TIM11_PINCFG,
  .pclk       = STM32_PCLK2_FREQUENCY,
};
#endif

#ifdef CONFIG_STM32_TIM12_PWM
static struct stm32_pwmtimer_s g_pwm12dev =
{
  .ops        = &g_pwmops,
  .timid      = 12,
  .channel    = CONFIG_STM32_TIM12_CHANNEL,
  .base       = STM32_TIM12_BASE,
  .pincfg     = PWM_TIM12_PINCFG,
  .pclk       = STM32_PCLK1_FREQUENCY,
};
#endif

#ifdef CONFIG_STM32_TIM13_PWM
static struct stm32_pwmtimer_s g_pwm13dev =
{
  .ops        = &g_pwmops,
  .timid      = 13,
  .channel    = CONFIG_STM32_TIM13_CHANNEL,
  .base       = STM32_TIM13_BASE,
  .pincfg     = PWM_TIM13_PINCFG,
  .pclk       = STM32_PCLK1_FREQUENCY,
};
#endif

#ifdef CONFIG_STM32_TIM14_PWM
static struct stm32_pwmtimer_s g_pwm14dev =
{
  .ops        = &g_pwmops,
  .timid      = 14,
  .channel    = CONFIG_STM32_TIM14_CHANNEL,
  .base       = STM32_TIM14_BASE,
  .pincfg     = PWM_TIM14_PINCFG,
  .pclk       = STM32_PCLK1_FREQUENCY,
};
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: pwm_getreg
 *
 * Description:
 *   Read the value of an ADC timer register.
 *
 * Input Parameters:
 *   priv - A reference to the ADC block status
 *   offset - The offset to the register to read
 *
 * Returned Value:
 *   The current contents of the specified register
 *
 ****************************************************************************/

static uint16_t pwm_getreg(struct stm32_pwmtimer_s *priv, int offset)
{
  return getreg16(priv->base + offset);
}

/****************************************************************************
 * Name: pwm_putreg
 *
 * Description:
 *   Read the value of an ADC timer register.
 *
 * Input Parameters:
 *   priv - A reference to the ADC block status
 *   offset - The offset to the register to read
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void pwm_putreg(struct stm32_pwmtimer_s *priv, int offset, uint16_t value)
{
  putreg16(value, priv->base + offset);
}

/****************************************************************************
 * Name: pwm_setup
 *
 * Description:
 *   This method is called when the driver is opened.  The lower half driver
 *   should configure and initialize the device so that it is ready for use.
 *   It should not, however, output pulses until the start method is called.
 *
 * Input parameters:
 *   dev - A reference to the lower half PWM driver state structure
 *
 * Returned Value:
 *   Zero on success; a negated errno value on failure
 *
 * Assumptions:
 *   AHB1 or 2 clocking for the GPIOs and timer has already been configured
 *   by the RCC logic at power up.
 *
 ****************************************************************************/

static int pwm_setup(FAR struct pwm_lowerhalf_s *dev)
{
  FAR struct stm32_pwmtimer_s *priv = (FAR struct stm32_pwmtimer_s *)dev;

  /* Configure the PWM output pin, but do not start the timer yet */

  stm32_configgpio(priv->pincfg);
  return OK;
}

/****************************************************************************
 * Name: pwm_shutdown
 *
 * Description:
 *   This method is called when the driver is closed.  The lower half driver
 *   stop pulsed output, free any resources, disable the timer hardware, and
 *   put the system into the lowest possible power usage state
 *
 * Input parameters:
 *   dev - A reference to the lower half PWM driver state structure
 *
 * Returned Value:
 *   Zero on success; a negated errno value on failure
 *
 ****************************************************************************/

static int pwm_shutdown(FAR struct pwm_lowerhalf_s *dev)
{
  FAR struct stm32_pwmtimer_s *priv = (FAR struct stm32_pwmtimer_s *)dev;
  uint32_t pincfg;

  /* Make sure that the output has been stopped */

  pwm_stop(dev);

  /* Then put the GPIO pin back to the default state */

  pincfg = priv->pincfg & (GPIO_PORT_MASK|GPIO_PIN_MASK);

#if defined(CONFIG_STM32_STM32F10XX)
  pincfg |= (GPIO_INPUT|GPIO_CNF_INFLOAT|GPIO_MODE_INPUT);
#elif defined(CONFIG_STM32_STM32F40XX)
  pincfg |= (GPIO_INPUT|GPIO_FLOAT);
#else
#  error "Unrecognized STM32 chip"
#endif

  stm32_configgpio(pincfg);
  return OK;
}

/****************************************************************************
 * Name: pwm_start
 *
 * Description:
 *   (Re-)initialize the timer resources and start the pulsed output
 *
 * Input parameters:
 *   dev - A reference to the lower half PWM driver state structure
 *   info - A reference to the characteristics of the pulsed output
 *
 * Returned Value:
 *   Zero on success; a negated errno value on failure
 *
 ****************************************************************************/

static int pwm_start(FAR struct pwm_lowerhalf_s *dev, FAR const struct pwm_info_s *info)
{
  FAR struct stm32_pwmtimer_s *priv = (FAR struct stm32_pwmtimer_s *)dev;

  /* Calculated values */

  uint32_t prescaler;
  uint32_t timclk;
  uint32_t reload;
  uint32_t ccr;

  /* Register contents */

  uint16_t cr1;
  uint16_t ccer;
  uint16_t cr2;
  uint16_t ccmr1;
  uint16_t ccmr2;

  /* New timer regiser bit settings */

  uint16_t ccenable;
  uint16_t ocmode1;
  uint16_t ocmode2;

  /* Caculate optimal values for the timer prescaler and for the timer reload
   * register.  If' frequency' is the desired frequency, then
   *
   *   reload = timclk / frequency
   *   timclk = pclk / presc
   *
   * Or,
   *
   *   reload = pclk / presc / frequency
   *
   * There are many solutions to this this, but the best solution will be the
   * one that has the largest reload value and the smallest prescaler value.
   * That is the solution that should give us the most accuracy in the timer
   * control.  Subject to:
   *
   *   0 <= presc  <= 65536
   *   1 <= reload <= 65535
   *
   * So presc = pclk / 65535 / frequency would be optimal.
   *
   * Example:
   *
   *  pclk      = 42 MHz
   *  frequency = 100 Hz
   *
   *  prescaler = 42,000,000 / 65,535 / 100
   *            = 6.4 (or 7 -- taking the ceiling always)
   *  timclk    = 42,000,000 / 7
   *            = 6,000,000
   *  reload    = 7,000,000 / 100
   *            = 60,000
   */

  prescaler = (priv->pclk / info->frequency + 65534) / 65535;
  if (prescaler < 1)
    {
      prescaler = 1;
    }
  else if (prescaler > 65536)
    {
      prescaler = 65536;
    }
 
  timclk = priv->pclk / prescaler;

  reload = timclk / info->frequency;
  if (reload < 1)
    {
      reload = 1;
    }
  else if (reload > 65535)
    {
      reload = 65535;
    }

  pwmvdbg("TIM%d PCLK: %d frequency: %d TIMCLK: %d prescaler: %d reload: %d\n",
          priv->timid, priv->pclk, info->frequency, timclk, prescaler, reload);

  /* Set up the timer CR1 register:
   *
   * 1-8  CKD[1:0] ARPE CMS[1:0] DIR OPM URS UDIS CEN
   * 2-5  CKD[1:0] ARPE CMS      DIR OPM URS UDIS CEN
   * 6-7           ARPE              OPM URS UDIS CEN
   * 9-14 CKD[1:0] ARPE                  URS UDIS CEN
   */

  cr1 = pwm_getreg(priv, STM32_GTIM_CR1_OFFSET);
  
  /* Disable the timer until we get it configured */

  cr1 &= ~GTIM_CR1_CEN;  

  /* Set the counter mode for the advanced timers (1,8) and most general
   * purpose timers (2-5, but not 9-14):
   */

#if defined(CONFIG_STM32_TIM1_PWM) || defined(CONFIG_STM32_TIM2_PWM)  || \
    defined(CONFIG_STM32_TIM3_PWM) || defined(CONFIG_STM32_TIM4_PWM)  || \
    defined(CONFIG_STM32_TIM5_PWM) || defined(CONFIG_STM32_TIM8_PWM)

  if ((priv->timid >= 1 && priv->timid <= 5) || priv->timid == 8)
    {
      /* Select the Counter Mode == count up:
       *
       * ATIM_CR1_EDGE: The counter counts up or down depending on the
       *   direction bit(DIR).
       * ATIM_CR1_DIR: 0: count up, 1: count down
       */
 
      cr1 &= ~(ATIM_CR1_DIR | ATIM_CR1_CMS_MASK);
      cr1 |= ATIM_CR1_EDGE;
    }
#endif

  /* Set the clock division to zero for all (but the basic timers, but there
   * should be no basic timers in this context
   */

  cr1 &= ~GTIM_CR1_CKD_MASK;
  pwm_putreg(priv, STM32_GTIM_CR1_OFFSET, cr1);

  /* Set the reload and prescaler values */

  pwm_putreg(priv, STM32_GTIM_ARR_OFFSET, reload);
  pwm_putreg(priv, STM32_GTIM_PSC_OFFSET, prescaler - 1);

  /* Clear the advanced timers repitition counter in all but the advanced timers */

#if defined(CONFIG_STM32_TIM1_PWM) || defined(CONFIG_STM32_TIM8_PWM)
  if (priv->timid == 1 || priv->timid == 8)
    {
      pwm_putreg(priv, STM32_ATIM_RCR_OFFSET, 0);
    }
#endif

  /* Generate an update event to reload the prescaler (all timers) */

  pwm_putreg(priv, STM32_GTIM_EGR_OFFSET, ATIM_EGR_UG);

  /* Duty cycle:
   *
   * duty cycle = ccr / reload (fractional value)
   */

  ccr = b16toi(info->duty * reload + b16HALF);

  /* Handle channel specific setup */

  ocmode1 = 0;
  ocmode2 = 0;
  switch (priv->channel)
    {
      case 1:  /* PWM Mode configuration: Channel 1 */
        {
          ccenable = ATIM_CCER_CC1E;
          ocmode1  = (ATIM_CCMR_CCS_CCOUT << ATIM_CCMR1_CC1S_SHIFT) |
                     (ATIM_CCMR_MODE_PWM1 << ATIM_CCMR1_OC1M_SHIFT) |
                     ATIM_CCMR1_OC1PE;
        }
        break;

      case 2:  /* PWM Mode configuration: Channel 2 */
        {
          ccenable = ATIM_CCER_CC2E;
          ocmode1  = (ATIM_CCMR_CCS_CCOUT << ATIM_CCMR1_CC2S_SHIFT) |
                     (ATIM_CCMR_MODE_PWM1 << ATIM_CCMR1_OC2M_SHIFT) |
                     ATIM_CCMR1_OC2PE;
        }
        break;

      case 3:  /* PWM Mode configuration: Channel3 */
        {
          ccenable = ATIM_CCER_CC3E;
          ocmode2  = (ATIM_CCMR_CCS_CCOUT << ATIM_CCMR2_CC3S_SHIFT) |
                     (ATIM_CCMR_MODE_PWM1 << ATIM_CCMR2_OC3M_SHIFT) |
                     ATIM_CCMR2_OC3PE;
        }
        break;

      case 4:  /* PWM1 Mode configuration: Channel4 */
        {
          ccenable = ATIM_CCER_CC4E;
          ocmode2  = (ATIM_CCMR_CCS_CCOUT << ATIM_CCMR2_CC4S_SHIFT) |
                     (ATIM_CCMR_MODE_PWM1 << ATIM_CCMR2_OC4M_SHIFT) |
                     ATIM_CCMR2_OC4PE;
        }
        break;

      default:
        return -EINVAL;
    }

  /* Disable the Channel by resetting the CCxE Bit in the CCER register */

  ccer = pwm_getreg(priv, STM32_GTIM_CCER_OFFSET);
  ccer &= ~ccenable;
  pwm_putreg(priv, STM32_GTIM_CCER_OFFSET, ccer);

  /* Fetch the CR2, CCMR1, and CCMR2 register (already have cr1 and ccer) */

  cr2   = pwm_getreg(priv, STM32_GTIM_CR2_OFFSET);
  ccmr1 = pwm_getreg(priv, STM32_GTIM_CCMR1_OFFSET);
  ccmr2 = pwm_getreg(priv, STM32_GTIM_CCMR1_OFFSET);

  /* Reset the Output Compare Mode Bits and set the select output compare mode */

  ccmr1 &= ~(ATIM_CCMR1_CC1S_MASK | ATIM_CCMR1_OC1M_MASK | ATIM_CCMR1_OC1PE |
             ATIM_CCMR1_CC2S_MASK | ATIM_CCMR1_OC2M_MASK | ATIM_CCMR1_OC2PE);
  ccmr2 &= ~(ATIM_CCMR2_CC3S_MASK | ATIM_CCMR2_OC3M_MASK | ATIM_CCMR2_OC3PE |
             ATIM_CCMR2_CC4S_MASK | ATIM_CCMR2_OC4M_MASK | ATIM_CCMR2_OC4PE);
  ccmr1 |= ocmode1;
  ccmr2 |= ocmode2;

  /* Reset the output polarity level of all channels (selects high polarity)*/

  ccer &= ~(ATIM_CCER_CC1P | ATIM_CCER_CC2P | ATIM_CCER_CC3P | ATIM_CCER_CC4P);
  
  /* Enable the output state of the selected channel (only) */

  ccer &= ~(ATIM_CCER_CC1E | ATIM_CCER_CC2E | ATIM_CCER_CC3E | ATIM_CCER_CC4E);
  ccer |= ccenable;

  /* Some special setup for advanced timers */

#if defined(CONFIG_STM32_TIM1_PWM) || defined(CONFIG_STM32_TIM8_PWM)
  if (priv->timid == 1 || priv->timid == 8)
    {
      /* Reset output N polarity level, output N state, output compre state,
       * output compare N idle state.
       */

#ifdef CONFIG_STM32_STM32F40XX
      ccer &= ~(ATIM_CCER_CC1NE | ATIM_CCER_CC1NP | ATIM_CCER_CC2NE | ATIM_CCER_CC2NP |
                ATIM_CCER_CC3NE | ATIM_CCER_CC3NP | ATIM_CCER_CC4NP);
#else
      ccer &= ~(ATIM_CCER_CC1NE | ATIM_CCER_CC1NP | ATIM_CCER_CC2NE | ATIM_CCER_CC2NP |
                ATIM_CCER_CC3NE | ATIM_CCER_CC3NP);
#endif

      /* Reset the output compare and output compare N IDLE State */

      cr2 &= ~(ATIM_CR2_OIS1 | ATIM_CR2_OIS1N | ATIM_CR2_OIS2 | ATIM_CR2_OIS2N |
               ATIM_CR2_OIS3 | ATIM_CR2_OIS3N | ATIM_CR2_OIS4);
    }
#ifdef CONFIG_STM32_STM32F40XX
  else
#endif
#endif
#ifdef CONFIG_STM32_STM32F40XX
    {
      ccer &= ~(GTIM_CCER_CC1NP | GTIM_CCER_CC2NP | GTIM_CCER_CC3NP | GTIM_CCER_CC4NP);
    }
#endif

  /* Save the modified register values */

  pwm_putreg(priv, STM32_GTIM_CR2_OFFSET, cr2);
  pwm_putreg(priv, STM32_GTIM_CCMR1_OFFSET, ccmr1);
  pwm_putreg(priv, STM32_GTIM_CCMR1_OFFSET, ccmr2);
  pwm_putreg(priv, STM32_GTIM_CCER_OFFSET, ccer);

  /* Set the ARR Preload Bit */

  cr1 = pwm_getreg(priv, STM32_GTIM_CR1_OFFSET);
  cr1 |= GTIM_CR1_ARPE;
  pwm_putreg(priv, STM32_GTIM_CR1_OFFSET, ccmr2);

  /* And, finally, enable the timer */

  cr1 |= GTIM_CR1_CEN;  
  pwm_putreg(priv, STM32_GTIM_CR1_OFFSET, ccmr2);
  return OK;
}

/****************************************************************************
 * Name: pwm_stop
 *
 * Description:
 *   Stop the pulsed output and reset the timer resources
 *
 * Input parameters:
 *   dev - A reference to the lower half PWM driver state structure
 *
 * Returned Value:
 *   Zero on success; a negated errno value on failure
 *
 ****************************************************************************/

static int pwm_stop(FAR struct pwm_lowerhalf_s *dev)
{
  FAR struct stm32_pwmtimer_s *priv = (FAR struct stm32_pwmtimer_s *)dev;

  uint32_t resetbit;
  uint32_t regaddr;
  uint32_t regval;

  switch (priv->timid)
    {
#ifdef CONFIG_STM32_TIM1_PWM
      case 1:
        regaddr  = STM32_RCC_APB2RSTR;
        resetbit = RCC_APB2RSTR_TIM1RST;
        break;
#endif
#ifdef CONFIG_STM32_TIM2_PWM
      case 2:
        regaddr  = STM32_RCC_APB1RSTR;
        resetbit = RCC_APB1RSTR_TIM2RST;
        break;
#endif
#ifdef CONFIG_STM32_TIM3_PWM
      case 3:
        regaddr  = STM32_RCC_APB1RSTR;
        resetbit = RCC_APB1RSTR_TIM3RST;
        break;
#endif
#ifdef CONFIG_STM32_TIM4_PWM
      case 4:
        regaddr  = STM32_RCC_APB1RSTR;
        resetbit = RCC_APB1RSTR_TIM4RST;
        break;
#endif
#ifdef CONFIG_STM32_TIM5_PWM
      case 5:
        regaddr  = STM32_RCC_APB1RSTR;
        resetbit = RCC_APB1RSTR_TIM5RST;
        break;
#endif
#ifdef CONFIG_STM32_TIM8_PWM
      case 8:
        regaddr  = STM32_RCC_APB2RSTR;
        resetbit = RCC_APB2RSTR_TIM8RST;
        break;
#endif
#ifdef CONFIG_STM32_TIM9_PWM
      case 9:
        regaddr  = STM32_RCC_APB2RSTR;
        resetbit = RCC_APB2RSTR_TIM9RST;
        break;
#endif
#ifdef CONFIG_STM32_TIM10_PWM
      case 10:
        regaddr  = STM32_RCC_APB2RSTR;
        resetbit = RCC_APB2RSTR_TIM10RST;
        break;
#endif
#ifdef CONFIG_STM32_TIM11_PWM
      case 11:
        regaddr  = STM32_RCC_APB2RSTR;
        resetbit = RCC_APB2RSTR_TIM11RST;
        break;
#endif
#ifdef CONFIG_STM32_TIM12_PWM
      case 12:
        regaddr  = STM32_RCC_APB1RSTR;
        resetbit = RCC_APB1RSTR_TIM12RST;
        break;
#endif
#ifdef CONFIG_STM32_TIM13_PWM
      case 13:
        regaddr  = STM32_RCC_APB1RSTR;
        resetbit = RCC_APB1RSTR_TIM13RST;
        break;
#endif
#ifdef CONFIG_STM32_TIM14_PWM
      case 14:
        regaddr  = STM32_RCC_APB1RSTR;
        resetbit = RCC_APB1RSTR_TIM14RST;
        break;
#endif
    }

  /* Reset the timer - stopping the output and putting the timer back
   * into a state where pwm_start() can be called.
   */

  regval  = getreg32(regaddr);
  regval |= resetbit;
  putreg32(regval, regaddr);

  regval &= ~resetbit;
  putreg32(regval, regaddr);
  return OK;
}

/****************************************************************************
 * Name:
 *
 * Description:
 *   Lower-half logic may support platform-specific ioctl commands
 *
 * Input parameters:
 *   dev - A reference to the lower half PWM driver state structure
 *   cmd - The ioctl command
 *   arg - The argument accompanying the ioctl command
 *
 * Returned Value:
 *   Zero on success; a negated errno value on failure
 *
 ****************************************************************************/

static int pwm_ioctl(FAR struct pwm_lowerhalf_s *dev, int cmd, unsigned long arg)
{
  /* There are no platform-specific ioctl commands */

  return -ENOTTY;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: stm32_pwminitialize
 *
 * Description:
 *   Initialize one timer for use with the upper_level PWM driver.
 *
 * Input Parameters:
 *   timer - A number identifying the timer use.  The number of valid timer
 *     IDs varies with the STM32 MCU and MCU family but is somewhere in
 *     the range of {1,..,14}.
 *
 * Returned Value:
 *   On success, a pointer to the STM32 lower half PWM driver is returned.
 *   NULL is returned on any failure.
 *
 ****************************************************************************/

FAR struct pwm_lowerhalf_s *stm32_pwminitialize(int timer)
{
  FAR struct stm32_pwmtimer_s *lower;

  switch (timer)
    {
#ifdef CONFIG_STM32_TIM1_PWM
      case 1:
        lower = &g_pwm1dev;
        break;
#endif
#ifdef CONFIG_STM32_TIM2_PWM
      case 2:
        lower = &g_pwm2dev;
        break;
#endif
#ifdef CONFIG_STM32_TIM3_PWM
      case 3:
        lower = &g_pwm3dev;
        break;
#endif
#ifdef CONFIG_STM32_TIM4_PWM
      case 4:
        lower = &g_pwm4dev;
        break;
#endif
#ifdef CONFIG_STM32_TIM5_PWM
      case 5:
        lower = &g_pwm5dev;
        break;
#endif
#ifdef CONFIG_STM32_TIM8_PWM
      case 8:
        lower = &g_pwm8dev;
        break;
#endif
#ifdef CONFIG_STM32_TIM9_PWM
      case 9:
        lower = &g_pwm9dev;
        break;
#endif
#ifdef CONFIG_STM32_TIM10_PWM
      case 10:
        lower = &g_pwm10dev;
        break;
#endif
#ifdef CONFIG_STM32_TIM11_PWM
      case 11:
        lower = &g_pwm11dev;
        break;
#endif
#ifdef CONFIG_STM32_TIM12_PWM
      case 12:
        lower = &g_pwm12dev;
        break;
#endif
#ifdef CONFIG_STM32_TIM13_PWM
      case 13:
        lower = &g_pwm13dev;
        break;
#endif
#ifdef CONFIG_STM32_TIM14_PWM
      case 14:
        lower = &g_pwm14dev;
        break;
#endif
      default:
        return NULL;
    }

  return (FAR struct pwm_lowerhalf_s *)lower;
}

#endif /* CONFIG_STM32_TIMn_PWM, n = 1,...,14 */
