/****************************************************************************
 * fs/fat/fs_mkfat.h
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

#ifndef __FS_FAT_FS_MKATFS_H
#define __FS_FAT_FS_MKATFS_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <sys/types.h>

/****************************************************************************
 * Definitions
 ****************************************************************************/

/* Only the "hard drive" media type is used */

#define FAT_DEFAULT_MEDIA_TYPE         0xf8

/*  Default hard driver geometry */

#define FAT_DEFAULT_SECPERTRK          63
#define FAT_DEFAULT_NUMHEADS           255

/* FSINFO is always at this sector */

#define FAT_DEFAULT_FSINFO_SECTOR      1

/* FAT32 foot cluster number */

#define FAT32_DEFAULT_ROOT_CLUSTER     2

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* This structure (plus the user-provided struct fat_format_s) describes
 * the format FAT file system.  All "global" variables used in the format
 * logic are contained in this structure so that is possible to format two
 * block devices concurrently.
 */

struct fat_var_s
{
   ubyte        fv_jump[3];         /* 3-byte boot jump instruction */
   ubyte        fv_sectshift;       /* Log2 of fv_sectorsize */
   uint16       fv_bootcodesize;    /* Size of array at fv_bootcode */
   uint32       fv_createtime;      /* Creation time */
   uint32       fv_sectorsize;      /* Size of one hardware sector */
   uint32       fv_nsectors;        /* Number of sectors */
   uint32       fv_fatlen;          /* Size of the FAT */
   ubyte       *fv_rootdir;         /* Allocated root directory sector */
   ubyte       *fv_mbr;             /* Allocated master boot record image */
   ubyte       *fv_info;            /* FAT32 info sector */
   const ubyte *fv_bootcode;        /* Points to boot code to put into MBR */
};

/****************************************************************************
 * Global Variables
 ****************************************************************************/

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#undef EXTERN
#if defined(__cplusplus)
#define EXTERN extern "C"
extern "C" {
#else
#define EXTERN extern
#endif

EXTERN void mkfatfs_initmbr(FAR struct fat_format_s *fmt, FAR struct fat_var_s *var, ubyte *sect)
;

#undef EXTERN
#if defined(__cplusplus)
}
#endif

#endif /* __FS_FAT_FS_MKATFS_H */
