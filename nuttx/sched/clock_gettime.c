/************************************************************************
 * sched/clock_gettime.c
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
 ************************************************************************/

/************************************************************************
 * Included Files
 ************************************************************************/

#include <nuttx/config.h>
#include <nuttx/rtc.h>

#include <stdint.h>
#include <time.h>
#include <errno.h>
#include <debug.h>

#include "clock_internal.h"

/************************************************************************
 * Definitions
 ************************************************************************/

/************************************************************************
 * Private Type Declarations
 ************************************************************************/

/************************************************************************
 * Private Function Prototypes
 ************************************************************************/

/**********************************************************************
 * Public Constant Data
 **********************************************************************/

/************************************************************************
 * Public Variables
 ************************************************************************/

/**********************************************************************
 * Private Variables
 **********************************************************************/

/************************************************************************
 * Private Functions
 ************************************************************************/

/************************************************************************
 * Public Functions
 ************************************************************************/

/************************************************************************
 * Function:  clock_gettime
 *
 * Description:
 *   Clock Functions based on POSIX APIs
 *
 ************************************************************************/

int clock_gettime(clockid_t clock_id, struct timespec *tp)
{
#ifndef CONFIG_SYSTEM_UTC
  uint32_t msecs;
  uint32_t secs;
  uint32_t nsecs;
#endif
  int ret = OK;

  sdbg("clock_id=%d\n", clock_id);

  /* CLOCK_REALTIME - POSIX demands this to be present. This is the wall
   * time clock.
   */

  if (clock_id == CLOCK_REALTIME && tp)
    {
#ifndef CONFIG_SYSTEM_UTC
      /* Get the elapsed time since power up (in milliseconds) biased
       * as appropriate.
       */

      msecs = MSEC_PER_TICK * (clock_systimer() - g_tickbias);

      sdbg("msecs = %d g_tickbias=%d\n",
           (int)msecs, (int)g_tickbias);

      /* Get the elapsed time in seconds and nanoseconds. */

      secs  = msecs / MSEC_PER_SEC;
      nsecs = (msecs - (secs * MSEC_PER_SEC)) * NSEC_PER_MSEC;

      sdbg("secs = %d + %d nsecs = %d + %d\n",
           (int)msecs, (int)g_basetime.tv_sec,
           (int)nsecs, (int)g_basetime.tv_nsec);

      /* Add the base time to this. */

      secs  += (uint32_t)g_basetime.tv_sec;
      nsecs += (uint32_t)g_basetime.tv_nsec;

      /* Handle carry to seconds. */

      if (nsecs > NSEC_PER_SEC)
        {
          uint32_t dwCarrySecs = nsecs / NSEC_PER_SEC;
          secs  += dwCarrySecs;
          nsecs -= (dwCarrySecs * NSEC_PER_SEC);
        }

      /* And return the result to the caller. */

      tp->tv_sec  = (time_t)secs;
      tp->tv_nsec = (long)nsecs;
      
#else   /* if CONFIG_SYSTEM_UTC=y */

#ifdef CONFIG_RTC
      if (g_rtc_enabled)
        {
          tp->tv_sec  = up_rtc_gettime();
          tp->tv_nsec = (up_rtc_getclock() & (RTC_CLOCKS_PER_SEC-1) ) * (1000000000/RTC_CLOCKS_PER_SEC);
        }
      else
#endif
        {
          tp->tv_sec  = g_system_utc;
          tp->tv_nsec = g_tickcount * (1000000000/TICK_PER_SEC);
        }
#endif

      sdbg("Returning tp=(%d,%d)\n",
           (int)tp->tv_sec, (int)tp->tv_nsec);
    }

 /* CLOCK_ACTIVETIME is non-standard. Returns active UTC time, which is
  * disabled during power down modes. Unit is 1 second.
  */

#ifdef CONFIG_RTC
  else if (clock_id == CLOCK_ACTIVETIME && g_rtc_enabled && tp)
    {
      tp->tv_sec  = g_system_utc;
      tp->tv_nsec = g_tickcount * (1000000000/TICK_PER_SEC);
    }
#endif

  else
    {
      sdbg("Returning ERROR\n");

      errno = EINVAL;
      ret = ERROR;
    }

  return ret;
}
