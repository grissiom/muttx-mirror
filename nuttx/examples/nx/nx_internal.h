/****************************************************************************
 * examples/nx/nx_internal.h
 *
 *   Copyright (C) 2008 Gregory Nutt. All rights reserved.
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

#ifndef __EXAMPLES_NX_NX_INTERNAL_H
#define __EXAMPLES_NX_NX_INTERNAL_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <sys/types.h>
#include <semaphore.h>

/****************************************************************************
 * Definitions
 ****************************************************************************/

/* Configuration ************************************************************/

#ifndef CONFIG_NXGRAPHICS
#  error "NX is not enabled (CONFIG_NXGRAPHICS)"
#endif

#ifndef CONFIG_EXAMPLES_NX_VPLANE
#    define CONFIG_EXAMPLES_NX_VPLANE 0
#endif

#ifndef CONFIG_EXAMPLES_NX_BGCOLOR
#  if CONFIG_SIM_FBBPP == 24 || CONFIG_SIM_FBBPP == 32
#    define CONFIG_EXAMPLES_NX_BGCOLOR 0x007b68ee
#  elif CONFIG_SIM_FBBPP = 16
#    define CONFIG_EXAMPLES_NX_BGCOLOR 0x3088
#  else
#    define CONFIG_EXAMPLES_NX_BGCOLOR ' '
# endif
#endif

#ifndef CONFIG_EXAMPLES_NX_COLOR1
#  if CONFIG_SIM_FBBPP == 24 || CONFIG_SIM_FBBPP == 32
#    define CONFIG_EXAMPLES_NX_COLOR1 0x00e6e6fa
#  elif CONFIG_SIM_FBBPP = 16
#    define CONFIG_EXAMPLES_NX_COLOR1 0x30c8
#  else
#    define CONFIG_EXAMPLES_NX_COLOR1 '1'
# endif
#endif

#ifndef CONFIG_EXAMPLES_NX_COLOR2
#  if CONFIG_SIM_FBBPP == 24 || CONFIG_SIM_FBBPP == 32
#    define CONFIG_EXAMPLES_NX_COLOR2 0x00dcdcdc
#  elif CONFIG_SIM_FBBPP = 16
#    define CONFIG_EXAMPLES_NX_COLOR2 0x30cc
#  else
#    define CONFIG_EXAMPLES_NX_COLOR2 '2'
# endif
#endif

#ifdef CONFIG_NX_MULTIUSER
#  ifdef CONFIG_DISABLE_MQUEUE
#    error "The multi-threaded example requires MQ support (CONFIG_DISABLE_MQUEUE=n)"
#  endif
#  ifdef CONFIG_DISABLE_SIGNALS
#    error "This example requires signal support (CONFIG_DISABLE_SIGNALS=n)"
#  endif
#  ifdef CONFIG_DISABLE_PTHREAD
#    error "This example requires pthread support (CONFIG_DISABLE_PTHREAD=n)"
#  endif
#  ifndef CONFIG_NX_BLOCKING
#    error "This example depends on CONFIG_NX_BLOCKING"
#  endif
#  ifndef CONFIG_EXAMPLES_NX_STACKSIZE
#    define CONFIG_EXAMPLES_NX_STACKSIZE 2048
#  endif
#  ifndef CONFIG_EXAMPLES_NX_LISTENERPRIO
#    define CONFIG_EXAMPLES_NX_LISTENERPRIO 100
#  endif
#  ifndef CONFIG_EXAMPLES_NX_CLIENTPRIO
#    define CONFIG_EXAMPLES_NX_CLIENTPRIO 100
#  endif
#  ifndef CONFIG_EXAMPLES_NX_SERVERPRIO
#    define CONFIG_EXAMPLES_NX_SERVERPRIO 120
#  endif
#  ifndef CONFIG_EXAMPLES_NX_NOTIFYSIGNO
#    define CONFIG_EXAMPLES_NX_NOTIFYSIGNO 4
#  endif
#endif

/* Debug ********************************************************************/

#ifdef CONFIG_CPP_HAVE_VARARGS
#  ifdef CONFIG_DEBUG
#    define message(...) lib_lowprintf(__VA_ARGS__)
#    define msgflush()
#  else
#    define message(...) printf(__VA_ARGS__)
#    define msgflush() fflush(stdout)
#  endif
#else
#  ifdef CONFIG_DEBUG
#    define message lib_lowprintf
#    define msgflush()
#  else
#    define message printf
#    define msgflush() fflush(stdout)
#  endif
#endif

/****************************************************************************
 * Public Types
 ****************************************************************************/

enum exitcode_e
{
  NXEXIT_SUCCESS = 0,
  NXEXIT_SIGPROCMASK,
  NXEXIT_SCHEDSETPARAM,
  NXEXIT_EVENTNOTIFY,
  NXEXIT_TASKCREATE,
  NXEXIT_PTHREADCREATE,
  NXEXIT_FBINITIALIZE,
  NXEXIT_FBGETVPLANE,
  NXEXIT_NXOPEN,
  NXEXIT_NXCONNECT,
  NXEXIT_NXSETBGCOLOR,
  NXEXIT_NXOPENWINDOW,
  NXEXIT_NXSETSIZE,
  NXEXIT_NXSETPOSITION,
  NXEXIT_NXCLOSEWINDOW,
  NXEXIT_LOSTSERVERCONN
};

/****************************************************************************
 * Public Variables
 ****************************************************************************/

/* The connecton handler */

extern NXHANDLE g_hnx;

/* NX callback vtables */

extern const struct nx_callback_s g_nxcb1;
extern const struct nx_callback_s g_nxcb2;

/* The screen resolution */

nxgl_coord_t g_xres;
nxgl_coord_t g_yres;

extern boolean b_haveresolution;
#ifdef CONFIG_NX_MULTIUSER
extern boolean g_connected;
#endif
extern sem_t g_semevent;

/* Colors used to fill window 1 & 2 */

extern nxgl_mxpixel_t g_color1[CONFIG_NX_NPLANES];
extern nxgl_mxpixel_t g_color2[CONFIG_NX_NPLANES];

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#if defined(CONFIG_NXGRAPHICS) && defined(CONFIG_NX_MULTIUSER)
extern int nx_servertask(int argc, char *argv[]);
extern FAR void *nx_listenerthread(FAR void *arg);
#endif

#endif /* __EXAMPLES_NX_NX_INTERNAL_H */
