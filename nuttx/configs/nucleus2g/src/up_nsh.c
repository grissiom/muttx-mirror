/****************************************************************************
 * config/nucleus2g/src/up_nsh.c
 * arch/arm/src/board/up_nsh.c
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

#include <stdio.h>
#include <unistd.h>
#include <debug.h>
#include <errno.h>

#include <nuttx/spi.h>
#include <nuttx/mmcsd.h>
#include <nuttx/usb/usbhost.h>

#include "lpc17_internal.h"
#include "nucleus2g_internal.h"

/****************************************************************************
 * Pre-Processor Definitions
 ****************************************************************************/

/* Configuration ************************************************************/

/* PORT and SLOT number probably depend on the board configuration */

#ifdef CONFIG_ARCH_BOARD_NUCLEUS2G
#  define CONFIG_EXAMPLES_NSH_HAVEUSBDEV 1
#  define CONFIG_EXAMPLES_NSH_HAVEMMCSD  1
#  define CONFIG_EXAMPLES_NSH_HAVEUSBHOST  1
#  if !defined(CONFIG_EXAMPLES_NSH_MMCSDSPIPORTNO) || CONFIG_EXAMPLES_NSH_MMCSDSPIPORTNO != 0
#    error "The Nucleus-2G MMC/SD is on SSP0"
#    undef CONFIG_EXAMPLES_NSH_MMCSDSPIPORTNO
#    define CONFIG_EXAMPLES_NSH_MMCSDSPIPORTNO 0
#  endif
#  if !defined(CONFIG_EXAMPLES_NSH_MMCSDSLOTNO) || CONFIG_EXAMPLES_NSH_MMCSDSLOTNO != 0
#    error "The Nucleus-2G MMC/SD is only one slot (0)"
#    undef CONFIG_EXAMPLES_NSH_MMCSDSLOTNO
#    define CONFIG_EXAMPLES_NSH_MMCSDSLOTNO 0
#  endif
#  ifndef CONFIG_LPC17_SSP0
#    warning "CONFIG_LPC17_SSP0 is not enabled"
#  endif
#else
#  error "Unrecognized board"
#  undef CONFIG_EXAMPLES_NSH_HAVEUSBDEV
#  undef CONFIG_EXAMPLES_NSH_HAVEMMCSD
#  undef CONFIG_EXAMPLES_NSH_HAVEUSBHOST
#endif

/* Can't support USB device features if USB device is not enabled */

#ifndef CONFIG_USBDEV
#  undef CONFIG_EXAMPLES_NSH_HAVEUSBDEV
#endif

/* Can't support MMC/SD features if mountpoints are disabled */

#if defined(CONFIG_DISABLE_MOUNTPOINT)
#  undef CONFIG_EXAMPLES_NSH_HAVEMMCSD
#endif

#ifndef CONFIG_EXAMPLES_NSH_MMCSDMINOR
#  define CONFIG_EXAMPLES_NSH_MMCSDMINOR 0
#endif

/* USB Host */

#ifdef CONFIG_USBHOST
#  ifndef CONFIG_LPC17_USBHOST
#    error "CONFIG_LPC17_USBHOST is not selected"
#  endif
#endif

#ifdef CONFIG_LPC17_USBHOST
#  ifndef CONFIG_USBHOST
#    warning "CONFIG_USBHOST is not selected"
#  endif
#endif

#if !defined(CONFIG_USBHOST) || !defined(CONFIG_LPC17_USBHOST)
#  undef CONFIG_EXAMPLES_NSH_HAVEUSBHOST
#endif

#ifdef CONFIG_EXAMPLES_NSH_HAVEUSBHOST
#  ifndef CONFIG_USBHOST_DEFPRIO
#    define CONFIG_USBHOST_DEFPRIO 50
#  endif
#  ifndef CONFIG_USBHOST_STACKSIZE
#    define CONFIG_USBHOST_STACKSIZE 1024
#  endif
#endif

/* Debug ********************************************************************/

#ifdef CONFIG_CPP_HAVE_VARARGS
#  ifdef CONFIG_DEBUG
#    define message(...) lib_lowprintf(__VA_ARGS__)
#  else
#    define message(...) printf(__VA_ARGS__)
#  endif
#else
#  ifdef CONFIG_DEBUG
#    define message lib_lowprintf
#  else
#    define message printf
#  endif
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

#ifdef CONFIG_EXAMPLES_NSH_HAVEUSBHOST
static struct usbhost_driver_s *g_drvr;
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nsh_waiter
 *
 * Description:
 *   Wait for USB devices to be connected.
 *
 ****************************************************************************/

#ifdef CONFIG_EXAMPLES_NSH_HAVEUSBHOST
static int nsh_waiter(int argc, char *argv[])
{
  bool connected = false;
  int ret;

  message("nsh_waiter: Running\n");
  for (;;)
    {
#ifdef CONFIG_USBHOST_HAVERHSC
      /* Wait for the device to change state */

      ret = DRVR_WAIT(g_drvr, connected);
      DEBUGASSERT(ret == OK);

      connected = !connected;
      message("nsh_waiter: %s\n", connected ? "connected" : "disconnected");

      /* Did we just become connected? */

      if (connected)
        {
          /* Yes.. enumerate the newly connected device */

          (void)DRVR_ENUMERATE(g_drvr);
        }
#else
      /* Is the device connected? */

      if (connected)
        {
          /* Yes.. wait for the disconnect event */

          ret = DRVR_WAIT(g_drvr, false);
          DEBUGASSERT(ret == OK);

          connected = false;
          message("nsh_waiter: Not connected\n");
        }
      else
        {
          /* Wait a bit */

          sleep(2);

          /* Try to enumerate the device */

          uvdbg("nsh_usbhostinitialize: Enumerate device\n");
          ret = DRVR_ENUMERATE(g_drvr);
          if (ret != OK)
            {
              uvdbg("nsh_usbhostinitialize: Enumeration failed: %d\n", ret);
            }
          else
            {
              message("nsh_usbhostinitialize: Connected\n");
            }
        }
#endif
    }

  /* Keep the compiler from complaining */

  return 0;
}
#endif

/****************************************************************************
 * Name: nsh_usbhostinitialize
 *
 * Description:
 *   Initialize SPI-based microSD.
 *
 ****************************************************************************/

#ifdef CONFIG_EXAMPLES_NSH_HAVEUSBHOST
static int nsh_usbhostinitialize(void)
{
  int pid;

  /* First, get an instance of the USB host interface */

  message("nsh_usbhostinitialize: Initialize USB host\n");
  g_drvr = usbhost_initialize(0);
  if (g_drvr)
    {
      /* Start a thread to handle device connection. */

      message("nsh_usbhostinitialize: Start nsh_waiter\n");

#ifndef CONFIG_CUSTOM_STACK
      pid = task_create("usbhost", CONFIG_USBHOST_DEFPRIO,
                        CONFIG_USBHOST_STACKSIZE,
                        (main_t)nsh_waiter, (const char **)NULL);
#else
      pid = task_create("usbhost", CONFIG_USBHOST_DEFPRIO,
                        (main_t)nsh_waiter, (const char **)NULL);
#endif
      return pid < 0 ? -ENOEXEC : OK;
    }
  return -ENODEV;
}
#else
#  define nsh_usbhostinitialize() (OK)
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nsh_archinitialize
 *
 * Description:
 *   Perform architecture specific initialization
 *
 ****************************************************************************/

int nsh_archinitialize(void)
{
  FAR struct spi_dev_s *ssp;
  int ret;

  /* Get the SSP port */

  ssp = up_spiinitialize(CONFIG_EXAMPLES_NSH_MMCSDSPIPORTNO);
  if (!ssp)
    {
      message("nsh_archinitialize: Failed to initialize SSP port %d\n",
              CONFIG_EXAMPLES_NSH_MMCSDSPIPORTNO);
      return -ENODEV;
    }

  message("Successfully initialized SSP port %d\n",
          CONFIG_EXAMPLES_NSH_MMCSDSPIPORTNO);

  /* Bind the SSP port to the slot */

  ret = mmcsd_spislotinitialize(CONFIG_EXAMPLES_NSH_MMCSDMINOR, CONFIG_EXAMPLES_NSH_MMCSDSLOTNO, ssp);
  if (ret < 0)
    {
      message("nsh_archinitialize: Failed to bind SSP port %d to MMC/SD slot %d: %d\n",
              CONFIG_EXAMPLES_NSH_MMCSDSPIPORTNO, CONFIG_EXAMPLES_NSH_MMCSDSLOTNO, ret);
      return ret;
    }

  message("Successfuly bound SSP port %d to MMC/SD slot %d\n",
          CONFIG_EXAMPLES_NSH_MMCSDSPIPORTNO, CONFIG_EXAMPLES_NSH_MMCSDSLOTNO);

  /* Initialize USB host */

  ret = nsh_usbhostinitialize();

  if (ret == OK)
    {
      message("USB host successfuly initialized!\n");
    }

  return ret;
}
