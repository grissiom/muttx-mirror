/****************************************************************************
 * sched/clock_initialize.c
 *
 *   Copyright (C) 2007, 2009, 2011 Gregory Nutt. All rights reserved.
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
#include <time.h>
#include <errno.h>
#include <debug.h>

#include <nuttx/clock.h>
#include <nuttx/time.h>

#include "clock_internal.h"

/****************************************************************************
 * Definitions
 ****************************************************************************/

/* Standard time definitions (in units of seconds) */

#define SEC_PER_MIN  ((time_t)60)
#define SEC_PER_HOUR ((time_t)60 * SEC_PER_MIN)
#define SEC_PER_DAY  ((time_t)24 * SEC_PER_HOUR)

/* Defined just so the uptime counter and system timer look similar */

#define incr_systimer() g_system_timer++

/****************************************************************************
 * Private Type Declarations
 ****************************************************************************/

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/**************************************************************************
 * Public Constant Data
 **************************************************************************/

/****************************************************************************
 * Public Variables
 ****************************************************************************/

volatile clock_t g_system_timer = 0;

#if CONFIG_UPTIME
volatile time_t  g_uptime       = 0;
#endif

struct timespec  g_basetime     = {0,0};
uint32_t         g_tickbias     = 0;

/**************************************************************************
 * Private Variables
 **************************************************************************/

/* This variable is used to count ticks and to increment the one-second
 * uptime variable.
 */

#if CONFIG_UPTIME
#if TICK_PER_SEC > 32767
static uint32_t g_tickcount = 0;
#elif TICK_PER_SEC > 255
static uint16_t g_tickcount = 0;
#else
static uint8_t  g_tickcount = 0;
#endif
#endif /* CONFIG_UPTIME */

/**************************************************************************
 * Private Functions
 **************************************************************************/
/****************************************************************************
 * Function: clock_timer
 *
 * Description:
 *   This function must be called once every time the real
 *   time clock interrupt occurs.  The interval of this
 *   clock interrupt must be MSEC_PER_TICK
 *
 ****************************************************************************/

#if CONFIG_UPTIME
static inline void incr_uptime(void)
{
  g_tickcount++;
  
  if (g_tickcount >= TICK_PER_SEC)
    {
      g_uptime++;
      g_tickcount -= TICK_PER_SEC;
    }
}
#else
#  define incr_uptime()
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Function: clock_initialize
 *
 * Description:
 *   Perform one-time initialization of the timing facilities.
 *
 ****************************************************************************/

void clock_initialize(void)
{
  time_t jdn;
#ifdef CONFIG_PTIMER
  bool rtc_enabled = false;
#endif

  /* Initialize the real time close (this should be un-nesssary except on a
   * restart).
   */

  g_system_timer = 0;
#ifdef CONFIG_UPTIME
  g_uptime = 0;
#endif

  /* Do we have hardware periodic timer support? */

#ifdef CONFIG_RTC
  if (up_rtcinitialize() == OK)
    {
	  rtc_enabled = true;
	}
#endif

  /* Get the EPOCH-relative julian date from the calendar year,
   * month, and date
   */

#ifdef CONFIG_PTIMER
  if (!rtc_enabled)
#endif
    {
      jdn = clock_calendar2utc(CONFIG_START_YEAR, CONFIG_START_MONTH,
                               CONFIG_START_DAY);
    }

  /* Set the base time as seconds into this julian day. */

  g_basetime.tv_sec  = jdn * SEC_PER_DAY;
  g_basetime.tv_nsec = 0;

  /* These is no time bias from this time. */

  g_tickbias = 0;
}

/****************************************************************************
 * Function: clock_timer
 *
 * Description:
 *   This function must be called once every time the real
 *   time clock interrupt occurs.  The interval of this
 *   clock interrupt must be MSEC_PER_TICK
 *
 ****************************************************************************/

void clock_timer(void)
{
  /* Increment the per-tick system counter */

  incr_systimer();

  /* Increment the per-second uptime counter */

  incr_uptime();
}
