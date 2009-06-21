/****************************************************************************
 * examples/nxflat/nxflat_main.c
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
#include <sys/mount.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <debug.h>
#include <errno.h>

#include <nuttx/ramdisk.h>
#include <nuttx/binfmt.h>

#include "tests/romfs.h"
#include "tests/dirlist.h"
#include "tests/symtab.h"

/****************************************************************************
 * Definitions
 ****************************************************************************/

/* Check configuration.  This is not all of the configuration settings that
 * are required -- only the more obvious.
 */

#if CONFIG_NFILE_DESCRIPTORS < 1
#  error "You must provide file descriptors via CONFIG_NFILE_DESCRIPTORS in your configuration file"
#endif

#ifndef CONFIG_NXFLAT
#  error "You must select CONFIG_NXFLAT in your configuration file"
#endif

#ifndef CONFIG_FS_ROMFS
#  error "You must select CONFIG_FS_ROMFS in your configuration file"
#endif

#ifdef CONFIG_DISABLE_MOUNTPOINT
#  error "You must not disable mountpoints via CONFIG_DISABLE_MOUNTPOINT in your configuration file"
#endif

#ifdef CONFIG_BINFMT_DISABLE
#  error "You must not disable loadable modules via CONFIG_BINFMT_DISABLE in your configuration file"
#endif

/* Describe the ROMFS file system */

#define SECTORSIZE   512
#define NSECTORS(b)  (((b)+SECTORSIZE-1)/SECTORSIZE)
#define ROMFSDEV     "/dev/ram0"
#define MOUNTPT      "/mnt/romfs"

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const char delimiter[] =
  "****************************************************************************";

static char path[128];

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: testheader
 ****************************************************************************/

static inline void testheader(FAR const char *progname)
{
  printf("\n%s\n* Executing %s\n%s\n\n", delimiter, progname, delimiter);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: user_initialize
 ****************************************************************************/

void user_initialize(void)
{
}

/****************************************************************************
 * Name: user_start
 ****************************************************************************/

int user_start(int argc, char *argv[])
{
  struct binary_s bin;
  int ret;
  int i;

  /* Create a ROM disk for the ROMFS filesystem */

  printf("Registering romdisk\n");
  ret = romdisk_register(0, romfs_img, NSECTORS(romfs_img_len), SECTORSIZE);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: romdisk_register failed: %d\n", ret);
      exit(1);
    }

  /* Mount the file system */

  printf("Mounting ROMFS filesystem at target=%s with source=%s\n",
         MOUNTPT, ROMFSDEV);

  ret = mount(ROMFSDEV, MOUNTPT, "romfs", MS_RDONLY, NULL);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: mount(%s,%s,romfs) failed: %s\n",
              ROMFSDEV, MOUNTPT, errno);
    }

  /* Now excercise every progrm in the ROMFS file system */

  for (i = 0; dirlist[i]; i++)
    {
      testheader(dirlist[i]);

      memset(&bin, 0, sizeof(struct binary_s));
      snprintf(path, 128, "%s/%s", MOUNTPT, dirlist[i]);

      bin.filename = path;
      bin.exports  = exports;
      bin.nexports = NEXPORTS;

      ret = load_module(&bin);
      if (ret < 0)
        {
          fprintf(stderr, "ERROR: Failed to load program '%s'\n", dirlist[i]);
          exit(1);
        }

      ret = exec_module(&bin, 50);
      if (ret < 0)
        {
          fprintf(stderr, "ERROR: Failed to execute program '%s'\n", dirlist[i]);
          unload_module(&bin);
        }
    }
  return 0;
}
