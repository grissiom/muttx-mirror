/****************************************************************************
 * config/sure-pic32mx/src/up_nsh.c
 * arch/arm/src/board/up_nsh.c
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

#include <stdio.h>
#include <unistd.h>
#include <debug.h>
#include <errno.h>

#include <nuttx/spi.h>
#include <nuttx/mmcsd.h>
#include <nuttx/usb/usbhost.h>

#include "pic32mx-internal.h"
#include "sure-internal.h"

/****************************************************************************
 * Pre-Processor Definitions
 ****************************************************************************/

/* Configuration ************************************************************/

/* PORT and SLOT number probably depend on the board configuration */

#ifdef CONFIG_ARCH_BOARD_SUREPIC32MX
#  define CONFIG_NSH_HAVEMMCSD   1
#  define CONFIG_NSH_HAVEUSBHOST 1
#  if !defined(CONFIG_NSH_MMCSDSPIPORTNO) || CONFIG_NSH_MMCSDSPIPORTNO != 1
#    error "The Sure PIC32MX MMC/SD is on SPI2"
#    undef CONFIG_NSH_MMCSDSPIPORTNO
#    define CONFIG_NSH_MMCSDSPIPORTNO 2
#  endif
#  if !defined(CONFIG_NSH_MMCSDSLOTNO) || CONFIG_NSH_MMCSDSLOTNO != 0
#    error "The Sure PIC32MX MMC/SD is only one slot (0)"
#    undef CONFIG_NSH_MMCSDSLOTNO
#    define CONFIG_NSH_MMCSDSLOTNO 0
#  endif
#  ifndef CONFIG_PIC32MX_SPI2
#    warning "CONFIG_PIC32MX_SPI2 is not enabled"
#    undef CONFIG_NSH_HAVEMMCSD
#  endif
#else
#  error "Unrecognized board"
#  undef CONFIG_NSH_HAVEMMCSD
#  undef CONFIG_NSH_HAVEUSBHOST
#endif

/* Can't support MMC/SD features if mountpoints are disabled */

#if defined(CONFIG_DISABLE_MOUNTPOINT)
#  undef CONFIG_NSH_HAVEMMCSD
#endif

#ifndef CONFIG_NSH_MMCSDMINOR
#  define CONFIG_NSH_MMCSDMINOR 0
#endif

/* USB Host */

#ifdef CONFIG_USBHOST
#  ifndef CONFIG_PIC32MX_USBHOST
#    error "CONFIG_PIC32MX_USBHOST is not selected"
#  endif
#endif

#ifdef CONFIG_PIC32MX_USBHOST
#  ifndef CONFIG_USBHOST
#    warning "CONFIG_USBHOST is not selected"
#  endif
#endif

#if !defined(CONFIG_USBHOST) || !defined(CONFIG_PIC32MX_USBHOST)
#  undef CONFIG_NSH_HAVEUSBHOST
#endif

#ifdef CONFIG_NSH_HAVEUSBHOST
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

#ifdef CONFIG_NSH_HAVEUSBHOST
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

#ifdef CONFIG_NSH_HAVEUSBHOST
static int nsh_waiter(int argc, char *argv[])
{
  bool connected = false;
  int ret;

  message("nsh_waiter: Running\n");
  for (;;)
    {
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
    }

  /* Keep the compiler from complaining */

  return 0;
}
#endif

/****************************************************************************
 * Name: nsh_sdinitialize
 *
 * Description:
 *   Initialize SPI-based microSD.
 *
 ****************************************************************************/

#ifdef CONFIG_NSH_HAVEMMCSD
static int nsh_sdinitialize(void)
{
  FAR struct spi_dev_s *ssp;
  int ret;

  /* Get the SPI port */

  ssp = up_spiinitialize(CONFIG_NSH_MMCSDSPIPORTNO);
  if (!ssp)
    {
      message("nsh_archinitialize: Failed to initialize SPI port %d\n",
              CONFIG_NSH_MMCSDSPIPORTNO);
      ret = -ENODEV;
      goto errout;
    }

  message("Successfully initialized SPI port %d\n",
          CONFIG_NSH_MMCSDSPIPORTNO);

  /* Bind the SPI port to the slot */

  ret = mmcsd_spislotinitialize(CONFIG_NSH_MMCSDMINOR,
                               CONFIG_NSH_MMCSDSLOTNO, ssp);
  if (ret < 0)
    {
      message("nsh_sdinitialize: "
              "Failed to bind SPI port %d to MMC/SD slot %d: %d\n",
              CONFIG_NSH_MMCSDSPIPORTNO,
              CONFIG_NSH_MMCSDSLOTNO, ret);
      goto errout;
    }

  message("Successfuly bound SPI port %d to MMC/SD slot %d\n",
          CONFIG_NSH_MMCSDSPIPORTNO,
          CONFIG_NSH_MMCSDSLOTNO);
  return OK;

errout:
  return ret;
}
#else
#  define nsh_sdinitialize() (OK)
#endif

/****************************************************************************
 * Name: nsh_usbhostinitialize
 *
 * Description:
 *   Initialize SPI-based microSD.
 *
 ****************************************************************************/

#ifdef CONFIG_NSH_HAVEUSBHOST
static int nsh_usbhostinitialize(void)
{
  int pid;
  int ret;

  /* First, register all of the class drivers needed to support the drivers
   * that we care about:
   */

  message("nsh_usbhostinitialize: Register class drivers\n");
  ret = usbhost_storageinit();
  if (ret != OK)
    {
      message("nsh_usbhostinitialize: Failed to register the mass storage class\n");
    }

  /* Then get an instance of the USB host interface */

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
  int ret;

  /* Initialize SPI-based microSD */

  ret = nsh_sdinitialize();
  if (ret == OK)
    {
      /* Initialize USB host */

      ret = nsh_usbhostinitialize();
    }
  return ret;
}
