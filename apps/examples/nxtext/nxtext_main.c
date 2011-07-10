/****************************************************************************
 * examples/nxtext/nxtext_main.c
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

#include <sys/types.h>

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <sched.h>
#include <pthread.h>
#include <errno.h>
#include <debug.h>

#ifdef CONFIG_NX_LCDDRIVER
#  include <nuttx/lcd/lcd.h>
#else
#  include <nuttx/fb.h>
#endif

#include <nuttx/arch.h>
#include <nuttx/nx.h>
#include <nuttx/nxglib.h>

#include "nxtext_internal.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Configuration ************************************************************/
/* If not specified, assume that the hardware supports one video plane */

#ifndef CONFIG_EXAMPLES_NXTEXT_VPLANE
#  define CONFIG_EXAMPLES_NXTEXT_VPLANE 0
#endif

/* If not specified, assume that the hardware supports one LCD device */

#ifndef CONFIG_EXAMPLES_NXTEXT_DEVNO
#  define CONFIG_EXAMPLES_NXTEXT_DEVNO 0
#endif

#define BGMSG_LINES 24

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

#ifdef CONFIG_NX_KBD
static const uint8_t g_pumsg[] = "Pop-Up!";
static const char *g_bgmsg[BGMSG_LINES] =
{
  "\nJULIET\n",
  "Wilt thou be gone?\n",
  "  It is not yet near day:\n",
  "It was the nightingale,\n",
  "  and not the lark,\n",
  "That pierced the fearful hollow\n",
  "  of thine ear;\n",
  "Nightly she sings\n",
  "  on yon pomegranate-tree:\n",
  "Believe me, love,\n",
  "  it was the nightingale.\n",
  "\nROMEO\n",
  "It was the lark,\n",
  "  the herald of the morn,\n",
  "No nightingale:\n",
  "  look, love, what envious streaks\n",
  "Do lace the severing clouds\n",
  "  in yonder east:\n",
  "Night's candles are burnt out,\n",
  "  and jocund day\n",
  "Stands tiptoe\n",
  "  on the misty mountain tops.\n",
  "I must be gone and live,\n",
  "  or stay and die.\n"
};
#endif

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* The connecton handler */

NXHANDLE g_hnx = NULL;

/* The screen resolution */

nxgl_coord_t g_xres;
nxgl_coord_t g_yres;

bool b_haveresolution = false;
#ifdef CONFIG_NX_MULTIUSER
bool g_connected = false;
#endif
sem_t g_semevent = {0};

int g_exitcode = NXEXIT_SUCCESS;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxtext_suinitialize
 ****************************************************************************/

#ifndef CONFIG_NX_MULTIUSER
static inline int nxtext_suinitialize(void)
{
  FAR NX_DRIVERTYPE *dev;

#if defined(CONFIG_EXAMPLES_NXTEXT_EXTERNINIT)
  /* Use external graphics driver initialization */

  message("nxtext_initialize: Initializing external graphics device\n");
  dev = up_nxdrvinit(CONFIG_EXAMPLES_NXTEXT_DEVNO);
  if (!dev)
    {
      message("nxtext_initialize: up_nxdrvinit failed, devno=%d\n", CONFIG_EXAMPLES_NXTEXT_DEVNO);
      g_exitcode = NXEXIT_EXTINITIALIZE;
      return ERROR;
    }

#elif defined(CONFIG_NX_LCDDRIVER)
  int ret;

  /* Initialize the LCD device */

  message("nxtext_initialize: Initializing LCD\n");
  ret = up_lcdinitialize();
  if (ret < 0)
    {
      message("nxtext_initialize: up_lcdinitialize failed: %d\n", -ret);
      g_exitcode = NXEXIT_LCDINITIALIZE;
      return ERROR;
    }

  /* Get the device instance */

  dev = up_lcdgetdev(CONFIG_EXAMPLES_NXTEXT_DEVNO);
  if (!dev)
    {
      message("nxtext_initialize: up_lcdgetdev failed, devno=%d\n", CONFIG_EXAMPLES_NXTEXT_DEVNO);
      g_exitcode = NXEXIT_LCDGETDEV;
      return ERROR;
    }

  /* Turn the LCD on at 75% power */

  (void)dev->setpower(dev, ((3*CONFIG_LCD_MAXPOWER + 3)/4));
#else
  int ret;

  /* Initialize the frame buffer device */

  message("nxtext_initialize: Initializing framebuffer\n");
  ret = up_fbinitialize();
  if (ret < 0)
    {
      message("nxtext_initialize: up_fbinitialize failed: %d\n", -ret);
      g_exitcode = NXEXIT_FBINITIALIZE;
      return ERROR;
    }

  dev = up_fbgetvplane(CONFIG_EXAMPLES_NXTEXT_VPLANE);
  if (!dev)
    {
      message("nxtext_initialize: up_fbgetvplane failed, vplane=%d\n", CONFIG_EXAMPLES_NXTEXT_VPLANE);
      g_exitcode = NXEXIT_FBGETVPLANE;
      return ERROR;
    }
#endif

  /* Then open NX */

  message("nxtext_initialize: Open NX\n");
  g_hnx = nx_open(dev);
  if (!g_hnx)
    {
      message("user_start: nx_open failed: %d\n", errno);
      g_exitcode = NXEXIT_NXOPEN;
      return ERROR;
    }
  return OK;
}
#endif

/****************************************************************************
 * Name: nxtext_initialize
 ****************************************************************************/

#ifdef CONFIG_NX_MULTIUSER
static inline int nxtext_muinitialize(void)
{
  struct sched_param param;
  pthread_t thread;
  pid_t servrid;
  int ret;

  /* Set the client task priority */

  param.sched_priority = CONFIG_EXAMPLES_NXTEXT_CLIENTPRIO;
  ret = sched_setparam(0, &param);
  if (ret < 0)
    {
      message("nxtext_initialize: sched_setparam failed: %d\n" , ret);
      g_exitcode = NXEXIT_SCHEDSETPARAM;
      return ERROR;
    }

  /* Start the server task */

  message("nxtext_initialize: Starting nxtext_server task\n");
  servrid = task_create("NX Server", CONFIG_EXAMPLES_NXTEXT_SERVERPRIO,
                        CONFIG_EXAMPLES_NXTEXT_STACKSIZE, nxtext_server, NULL);
  if (servrid < 0)
    {
      message("nxtext_initialize: Failed to create nxtext_server task: %d\n", errno);
      g_exitcode = NXEXIT_TASKCREATE;
      return ERROR;
    }

  /* Wait a bit to let the server get started */

  sleep(1);

  /* Connect to the server */

  g_hnx = nx_connect();
  if (g_hnx)
    {
       pthread_attr_t attr;

       /* Start a separate thread to listen for server events.  This is probably
        * the least efficient way to do this, but it makes this example flow more
        * smoothly.
        */

       (void)pthread_attr_init(&attr);
       param.sched_priority = CONFIG_EXAMPLES_NXTEXT_LISTENERPRIO;
       (void)pthread_attr_setschedparam(&attr, &param);
       (void)pthread_attr_setstacksize(&attr, CONFIG_EXAMPLES_NXTEXT_STACKSIZE);

       ret = pthread_create(&thread, &attr, nxtext_listener, NULL);
       if (ret != 0)
         {
            printf("nxtext_initialize: pthread_create failed: %d\n", ret);
            g_exitcode = NXEXIT_PTHREADCREATE;
            return ERROR;
         }

       /* Don't return until we are connected to the server */

       while (!g_connected)
         {
           /* Wait for the listener thread to wake us up when we really
            * are connected.
            */

           (void)sem_wait(&g_semevent);
         }
    }
  else
    {
      message("nxtext_initialize: nx_connect failed: %d\n", errno);
      g_exitcode = NXEXIT_NXCONNECT;
      return ERROR;
    }
  return OK;
}
#endif

/****************************************************************************
 * Name: nxtext_initialize
 ****************************************************************************/

static int nxtext_initialize(void)
{
#ifdef CONFIG_NX_MULTIUSER
  return nxtext_muinitialize();
#else
  return nxtext_suinitialize();
#endif
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: user_start
 ****************************************************************************/

int user_start(int argc, char *argv[])
{
  FAR struct nxtext_state_s *bgstate;
  NXWINDOW hwnd = NULL;
  nxgl_mxpixel_t color;
  int popcnt;
  int bkgndx;
  int ret;

  /* Initialize NX */

  ret = nxtext_initialize();
  message("user_start: NX handle=%p\n", g_hnx);
  if (!g_hnx || ret < 0)
    {
      message("user_start: Failed to get NX handle: %d\n", errno);
      g_exitcode = NXEXIT_NXOPEN;
      goto errout;
    }

  /* Set the background to the configured background color */

  message("user_start: Set background color=%d\n", CONFIG_EXAMPLES_NXTEXT_BGCOLOR);
  color = CONFIG_EXAMPLES_NXTEXT_BGCOLOR;
  ret = nx_setbgcolor(g_hnx, &color);
  if (ret < 0)
    {
      message("user_start: nx_setbgcolor failed: %d\n", errno);
      g_exitcode = NXEXIT_NXSETBGCOLOR;
      goto errout_with_nx;
    }

  /* Get the background window */

  bgstate = nxbg_getstate();
  ret = nx_requestbkgd(g_hnx, &g_bgcb, bgstate);
  if (ret < 0)
    {
      message("user_start: nx_setbgcolor failed: %d\n", errno);
      g_exitcode = NXEXIT_NXREQUESTBKGD;
      goto errout_with_nx;
    }

  /* Wait until we have the screen resolution.  We'll have this immediately
   * unless we are dealing with the NX server.
   */

  while (!b_haveresolution)
    {
      (void)sem_wait(&g_semevent);
    }
  message("user_start: Screen resolution (%d,%d)\n", g_xres, g_yres);

  /* Now loop, adding text to the background and periodically presenting
   * a pop-up window.
   */

  popcnt = 0;
  bkgndx = 0;
  for (;;)
    {
      /* Sleep for one second */

      sleep(1);
      popcnt++;

      /* Each three seconds, create a pop-up window.  Destroy the pop-up
       * window after two more seconds.
       */

      if (popcnt == 3)
        {
          /* Create a pop-up window */

          hwnd = nxpu_open();

          /* Give keyboard input to the top window (which should be the pop-up) */

#ifdef CONFIG_NX_KBD
          message("user_start: Send keyboard input: %s\n", g_pumsg);
          ret = nx_kbdin(g_hnx, strlen((FAR const char *)g_pumsg), g_pumsg);
          if (ret < 0)
           {
             message("user_start: nx_kbdin failed: %d\n", errno);
             goto errout_with_hwnd;
           }
#endif
        }
      else if (popcnt == 5)
        {
          /* Destroy the pop-up window and restart the sequence */
 
          message("user_start: Close pop-up\n");
          (void)nxpu_close(hwnd);
          popcnt = 0;
        }

      /* Give another line of text to the background window.  Force this
       * text to go the background by calling the kbdin method directly.
       */

      nxbg_write(g_bgwnd, (FAR const uint8_t *)g_bgmsg[bkgndx], strlen(g_bgmsg[bkgndx]));
      if (++bkgndx >= BGMSG_LINES)
        {
          bkgndx = 0;
        }
    }

  /* Close the pop-up window */

errout_with_hwnd:
  if (popcnt >= 3)
    {
      message("user_start: Close pop-up\n");
     (void)nxpu_close(hwnd);
    }

//errout_with_bkgd:
  (void)nx_releasebkgd(g_bgwnd);

errout_with_nx:
#ifdef CONFIG_NX_MULTIUSER
  /* Disconnect from the server */

  message("user_start: Disconnect from the server\n");
  nx_disconnect(g_hnx);
#else
  /* Close the server */

  message("user_start: Close NX\n");
  nx_close(g_hnx);
#endif
errout:
  return g_exitcode;
}
