/****************************************************************************
 * fs/romfs/fs_romfs.h
 *
 *   Copyright (C) 2008 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <spudmonkey@racsa.co.cr>
 *
 * References: Linux/Documentation/filesystems/romfs.txt
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

#ifndef __FS_ROMFS_FS_ROMFS_H
#define __FS_ROMFS_FS_ROMFS_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <sys/types.h>
#include "../fs_internal.h"

/****************************************************************************
 * Definitions
 ****************************************************************************/

/* Volume header (multi-byte values are big-endian) */

#define ROMFS_VHDR_ROM1FS   0  /*  0-7:  "-rom1fs-" */
#define ROMFS_VHDR_SIZE     8  /*  8-11: Number of accessible bytes in this fs. */
#define ROMFS_VHDR_CHKSUM  12  /* 12-15: Checksum of the first 512 bytes. */
#define ROMFS_VHDR_VOLNAME 16  /* 16-..: Zero terminated volume name, padded to
                                *        16 byte boundary. */

#define ROMFS_VHDR_MAGIC   "-rom1fs-"

/* File header offset (multi-byte values are big-endian) */

#define ROMFS_FHDR_NEXT     0  /*  0-3:  Offset of the next file header
                                *        (zero if no more files) */
#define ROMFS_FHDR_INFO     4  /*  4-7:  Info for directories/hard links/
                                *        devices */
#define ROMFS_FHDR_SIZE     8  /*  8-11: Size of this file in bytes */
#define ROMFS_FHDR_CHKSUM  12  /* 12-15: Checksum covering the meta data,
                                *        including the file name, and
                                *        padding. */
#define ROMFS_FHDR_NAME    16  /* 16-..: Zero terminated volume name, padded
                                *        to 16 byte boundary. */

/* Bits 0-3 of the rf_next offset provide mode information.  These are the
 * values specified in  */

#define RFNEXT_MODEMASK    7    /* Bits 0-2: Mode; bit 3: Executable */
#define RFNEXT_ALLMODEMASK 15   /* Bits 0-3: All mode bits */
#define RFNEXT_OFFSETMASK (~15) /* Bits n-3: Offset to next entry */

#define RFNEXT_HARDLINK    0    /* rf_info = Link destination file header */
#define RFNEXT_DIRECTORY   1    /* rf_info = First file's header */
#define RFNEXT_FILE        2    /* rf_info = Unused, must be zero */
#define RFNEXT_SOFTLINK    3    /* rf_info = Unused, must be zero */
#define RFNEXT_BLOCKDEV    4    /* rf_info = 16/16 bits major/minor number */
#define RFNEXT_CHARDEV     5    /* rf_info = 16/16 bits major/minor number */
#define RFNEXT_SOCKET      6    /* rf_info = Unused, must be zero */
#define RFNEXT_FIFO        7    /* rf_info = Unused, must be zero */
#define RFNEXT_EXEC        8    /* Modifier of RFNEXT_DIRECTORY and RFNEXT_FILE */

#define IS_MODE(rfn,mode)  ((((uint32)(rfn))&RFNEXT_MODEMASK)==(mode))
#define IS_HARDLINK(rfn)   IS_MODE(rfn,RFNEXT_HARDLINK)
#define IS_DIRECTORY(rfn)  IS_MODE(rfn,RFNEXT_DIRECTORY)
#define IS_FILE(rfn)       IS_MODE(rfn,RFNEXT_FILE)
#define IS_EXECUTABLE(rfn) (((rfn) & RFNEXT_EXEC) != 0)

/* RFNEXT_SOFTLINK, RFNEXT_BLOCKDEV, RFNEXT_CHARDEV, RFNEXT_SOCKET, and
 * RFNEXT_FIFO are not presently supported in NuttX.
 */

/* Alignment macros */

#define ROMFS_ALIGNMENT       16
#define ROMFS_MAXPADDING      (ROMFS_ALIGNMENT-1)
#define ROMFS_ALIGNMASK       (~ROMFS_MAXPADDING)
#define ROMFS_ALIGNUP(addr)   ((((uint32)(addr))+ROMFS_MAXPADDING)&ROMFS_ALIGNMASK)
#define ROMFS_ALIGNDOWN(addr) (((uint32)(addr))&ROMFS_ALIGNMASK)

/* Offset and sector conversions */

#define SEC_NDXMASK(r)       ((r)->rm_hwsectorsize - 1)
#define SEC_NSECTORS(r,o)    ((o) / (r)->rm_hwsectorsize)

/* Maximum numbr of links that will be followed before we decide that there
 * is a problem.
 */

#define ROMF_MAX_LINKS 64

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* This structure represents the overall mountpoint state.  An instance of this
 * structure is retained as inode private data on each mountpoint that is
 * mounted with a fat32 filesystem.
 */

struct romfs_file_s;
struct romfs_mountpt_s
{
  struct inode        *rm_blkdriver; /* The block driver inode that hosts the FAT32 fs */
  struct romfs_file_s *rm_head;      /* A list to all files opened on this mountpoint */

  boolean  rm_mounted;              /* TRUE: The file system is ready */
  uint16   rm_hwsectorsize;         /* HW: Sector size reported by block driver*/
  sem_t    rm_sem;                  /* Used to assume thread-safe access */
  uint32   rm_rootoffset;           /* Saved offset to the first root directory entry */
  uint32   rm_hwnsectors;           /* HW: The number of sectors reported by the hardware */
  uint32   rm_volsize;              /* Size of the ROMFS volume */
  uint32   rm_cachesector;          /* Current sector in the rm_buffer */
  ubyte   *rm_buffer;               /* Device sector buffer */
};

/* This structure represents on open file under the mountpoint.  An instance
 * of this structure is retained as struct file specific information on each
 * opened file.
 */

struct romfs_file_s
{
  struct romfs_file_s *rf_next;     /* Retained in a singly linked list */
  boolean  rf_open;                 /* TRUE: The file is (still) open */
  uint32   rf_diroffset;            /* Offset to the parent directory entry */
  uint32   rf_startoffset;          /* Offset to the start of the file */
  uint32   rf_size;                 /* Size of the file in bytes */
  uint32   rf_cachesector;          /* Current sector in the rf_buffer */
  ubyte   *rf_buffer;               /* File sector buffer */
};

/* This structure is used internally for describing the result of
 * walking a path
 */

struct romfs_dirinfo_s
{
  /* These values describe the directory containing the terminal
   * path component (of the terminal component itself if it is
   * a directory.
   */

  struct fs_romfsdir_s rd_dir;      /* Describes directory. */

  /* Values from the ROMFS file entry */

  uint32 rd_next;                   /* Offset of the next file header+flags */
  uint32 rd_size;                   /* Size (if file) */
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

EXTERN void romfs_semtake(struct romfs_mountpt_s *rm);
EXTERN void romfs_semgive(struct romfs_mountpt_s *rm);
EXTERN int  romfs_hwread(struct romfs_mountpt_s *rm, ubyte *buffer,
                         uint32 sector, unsigned int nsectors);
EXTERN int  romfs_devcacheread(struct romfs_mountpt_s *rm, uint32 sector);
EXTERN int  romfs_filecacheread(struct romfs_mountpt_s *rm,
                                struct romfs_file_s *rf, uint32 sector);
EXTERN int  romfs_getgeometry(struct romfs_mountpt_s *rm);
EXTERN int  romfs_mount(struct romfs_mountpt_s *rm);
EXTERN int  romfs_checkmount(struct romfs_mountpt_s *rm);
EXTERN int  romfs_finddirentry(struct romfs_mountpt_s *rm,
                               struct romfs_dirinfo_s *dirinfo,
                               const char *path);
EXTERN int  romfs_parsedirentry(struct romfs_mountpt_s *rm,
                                uint32 offset, uint32 *poffset, uint32 *pnext,
                                uint32 *pinfo, uint32 *psize);
EXTERN int  romfs_parsefilename(struct romfs_mountpt_s *rm, uint32 offset,
                                char *pname);

#undef EXTERN
#if defined(__cplusplus)
}
#endif

#endif /* __FS_ROMFS_FS_ROMFS_H */
