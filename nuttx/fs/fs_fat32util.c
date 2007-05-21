/****************************************************************************
 * fs_fat32util.c
 *
 *   Copyright (C) 2007 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <spudmonkey@racsa.co.cr>
 *
 * References:
 *   Microsoft FAT documentation
 *   FAT implementation 'Copyright (C) 2007, ChaN, all right reserved.'
 *     which has an unrestricted license.
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
 * 3. Neither the name Gregory Nutt nor the names of its contributors may be
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
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <semaphore.h>
#include <assert.h>
#include <errno.h>
#include <debug.h>

#include <nuttx/fs.h>

#include "fs_internal.h"
#include "fs_fat32.h"

#if CONFIG_FS_FAT

/****************************************************************************
 * Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Private Variables
 ****************************************************************************/

/****************************************************************************
 * Public Variables
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: fat_path2dirname
 *
 * Desciption:  Convert a user filename into a properly formatted FAT
 *   (short) filname as it would appear in a directory entry.  Here are the
 *    rules for the 11 byte name in the directory:
 *
 *   The first byte:
 *   - 0xe5 = The directory is free
 *   - 0x00 = This directory and all following directories are free
 *   - 0x05 = Really 0xe5
 *   - 0x20 = May NOT be ' '
 *
 *   Any bytes
 *     0x00-0x1f = (except for 0x00 and 0x05 in the first byte)
 *     0x22      = '"'
 *     0x2a-0x2c = '*', '+', ','
 *     0x2e-0x2f = '.', '/'
 *     0x3a-0x3f = ':', ';', '<', '=', '>', '?'
 *     0x5b-0x5d = '[', '\\', ;]'
 *     0x7c      = '|'
 *
 *   Upper case characters are not allowed in directory names (without some
 *   poorly documented operatgions on the NTRes directory byte).  Lower case
 *   codes may represent different characters in other character sets ("DOS
 *   code pages".  The logic below does not, at present, support any other
 *   character sets.
 *
 ****************************************************************************/

static inline int fat_path2dirname(const char **path, struct fat_dirinfo_s *dirinfo,
                                   char *terminator)
{
#ifdef CONFIG_FAT_LCNAMES
    unsigned int ntlcenable = FATNTRES_LCNAME | FATNTRES_LCEXT;
    unsigned int ntlcfound  = 0;
#endif
    const char *node = *path;
    int endndx;
    ubyte ch;
    int ndx = 0;

    /* Initialized the name with all spaces */

    memset(dirinfo->fd_name, ' ', 8+3);
 
    /* Loop until the name is successfully parsed or an error occurs */

    endndx  = 8;
    for (;;)
      {
        /* Get the next byte from the path */

        ch = *node++;

        /* Check if this the last byte in this node of the name */

        if ((ch == '\0' || ch == '/') && ndx != 0 )
          {
            /* Return the accumulated NT flags and the terminating character */
#ifdef CONFIG_FAT_LCNAMES
            dirinfo->fd_ntflags = ntlcfound & ntlcenable;
#endif
            *terminator = ch;
            *path       = node;
            return OK;
          }

        /* Accept only the printable character set.  Note the first byte
         * of the name could be 0x05 meaning that is it 0xe5, but this is
         * not a printable character in this character in either case.
         */

        else if (!isgraph(ch))
          {
            goto errout;
          }

        /* Check for transition from name to extension */

        else if (ch == '.')
          {
            /* Starting the extension */

            ndx    = 8;
            endndx = 11;
            continue;
          }

        /* Reject printable characters forbidden by FAT */

        else if (ch == '"'  ||  (ch >= '*' && ch <= ',') ||
                 ch == '.'  ||   ch == '/'               ||
                (ch >= ':'  &&   ch <= '?')              ||
                (ch >= '['  &&   ch <= ']')              ||
                (ch == '|'))
          {
            goto errout;
          }

        /* Check for upper case charaters */

#ifdef CONFIG_FAT_LCNAMES
        else if (isupper(ch))
          {
            /* Some or all of the characters in the name or extension
             * are upper case. Force all of the characters to be interpreted
             * as upper case.
             */

              if ( endndx == 8)
                {
                  /* Clear lower case name bit in mask*/
                  ntlcenable &= FATNTRES_LCNAME;
                }
              else
                {
                  /* Clear lower case extension in mask */
                  ntlcenable &= FATNTRES_LCNAME;
                }
          }
#endif

        /* Check for lower case characters */

        else if (islower(ch))
          {
            /* Convert the character to upper case */

            ch = toupper(ch);

            /* Some or all of the characters in the name or extension
             * are lower case.  They can be interpreted as lower case if
             * only if all of the characters in the name or extension are
             * lower case.
             */

#ifdef CONFIG_FAT_LCNAMES
            if ( endndx == 8)
              {
                /* Set lower case name bit */
                ntlcfound |= FATNTRES_LCNAME;
              }
            else
              {
                /* Set lower case extension bit */
                ntlcfound |= FATNTRES_LCNAME;
              }
#endif
          }

        /* Check if the file name exceeds the size permitted (without
         * long file name support
         */

        if (ndx >= endndx)
          {
            goto errout;
          }

        /* Save next character in the accumulated name */

        dirinfo->fd_name[ndx++] = ch;
      }

 errout:
    return -EINVAL;
}

/****************************************************************************
 * Name: fat_dirname2path
 *
 * Desciption:  Convert a filename in a raw directory entry into a user
 *    filename.  This is essentially the inverse operation of that performed
 *    by fat_path2dirname.  See that function for more details.
 *
 ****************************************************************************/

static inline int fat_dirname2path(char *path, struct fat_dirinfo_s *dirinfo)
{
    const unsigned char *direntry = dirinfo->fd_entry;
    int  ch;
    int  ndx;

    /* Check if we will be doing upper to lower case conversions */

#ifdef CONFIG_FAT_LCNAMES
    dirinfo->fd_ntflags = DIR_GETNTRES(direntry);
#endif

    /* Get the 8-byte filename */

    for (ndx = 0; ndx < 8; ndx++)
      {
        /* Get the next filename character from the directory entry */

        ch = direntry[ndx];

        /* Any space (or ndx==8) terminates the filename */

        if (ch == ' ')
          {
            break;
          }

        /* In this version, we never write 0xe5 in the directoryfilenames
         * (because we do not handle any character sets where 0xe5 is valid
         * in a filaname), but we could encounted this in a filesystem
         * written by some other system
         */

        if (ndx == 0 && ch == DIR0_E5)
          {
            ch = 0xe5;
          }

        /* Check if we should perform upper to lower case conversion
         * of the (whole) filename.
         */

#ifdef CONFIG_FAT_LCNAMES
        if (dirinfo->fd_ntflags & FATNTRES_LCNAME && isupper(ch))
          {
            ch = tolower(ch);
          }
#endif
        /* Copy the next character into the filename */

        *path++ = ch;
      }

    /* Check if there is an extension */

    if (direntry[8] != ' ')
      {
        /* Yes, output the dot before the extension */

        *path++ = '.';

        /* Then output the (up to) 3 character extension */

        for (ndx = 8; ndx < 11; ndx++)
          {
            /* Get the next extensions character from the directory entry */

            ch = dirinfo->fd_name[ndx];

            /* Any space (or ndx==11) terminates the extension */

            if (ch == ' ')
              {
                break;
              }

            /* Check if we should perform upper to lower case conversion
             * of the (whole) filename.
             */

#ifdef CONFIG_FAT_LCNAMES
            if (ntflags & FATNTRES_LCEXT && isupper(ch))
              {
                ch = tolower(ch);
              }
#endif
        /* Copy the next character into the filename */

            *path++ = ch;
          }
      }

    /* Put a null terminator at the end of the filename */

    *path = '\0';
    return OK;
}

/****************************************************************************
 * Name: fat_checkfsinfo
 *
 * Desciption: Read the FAT32 FSINFO sector
 *
 ****************************************************************************/

static int fat_checkfsinfo(struct fat_mountpt_s *fs)
{
  /* Verify that this is, indeed, an FSINFO sector */

  if (FSI_GETLEADSIG(fs->fs_buffer) == 0x41615252  &&
      FSI_GETSTRUCTSIG(fs->fs_buffer) == 0x61417272 &&
      FSI_GETTRAILSIG(fs->fs_buffer) == 0xaa550000)
    {
      fs->fs_fsinextfree  = FSI_GETFREECOUNT(fs->fs_buffer);
      fs->fs_fsifreecount = FSI_GETNXTFREE(fs->fs_buffer);
      return OK;
    }
  return -ENODEV;
}

/****************************************************************************
 * Name: fat_checkbootrecord
 *
 * Desciption: Read a sector and verify that it is a a FAT boot record.
 *
 ****************************************************************************/

static int fat_checkbootrecord(struct fat_mountpt_s *fs)
{
  uint32  ndatasectors;
  uint32  fatsize;
  uint16  rootdirsectors = 0;
  boolean notfat32 = FALSE;

  /* Verify the MBR signature at offset 510 in the sector (true even
   * if the sector size is greater than 512.  All FAT file systems have
   * this signature. On a FAT32 volume, the RootEntCount , FatSz16, and
   * FatSz32 values should always be zero.  The FAT sector size should
   * match the reported hardware sector size.
   */

  if (MBR_GETSIGNATURE(fs->fs_buffer) != 0xaa55 ||
      MBR_GETBYTESPERSEC(fs->fs_buffer) != fs->fs_hwsectorsize)
    {
      return -ENODEV;
    }

  /* Verify the FAT32 file system type. The determination of the file
   * system type is based on the number of clusters on the volume:  FAT12
   * volume has < 4085 cluseter, a FAT16 volume has fewer than 65,525
   * clusters, and any larger is FAT32.
   *
   * Get the number of 32-bit directory entries in root directory (zero
   * for FAT32.
   */

  fs->fs_rootentcnt = MBR_GETROOTENTCNT(fs->fs_buffer);
  if (fs->fs_rootentcnt != 0)
  {
      notfat32       = TRUE; /* Must be zero for FAT32 */
      rootdirsectors = (32 * fs->fs_rootentcnt  + fs->fs_hwsectorsize - 1) / fs->fs_hwsectorsize;
  }

  /* Determine the number of sectors in a FAT. */

  fs->fs_fatsize = MBR_GETFATSZ16(fs->fs_buffer); /* Should be zero */
  if (fs->fs_fatsize)
    {
      notfat32 = TRUE; /* Must be zero for FAT32 */
    }
  else
    {
      fs->fs_fatsize = MBR_GETFATSZ32(fs->fs_buffer);
    }

  if (!fs->fs_fatsize || fs->fs_fatsize >= fs->fs_hwnsectors)
    {
      return -ENODEV;
    }

  /* Get the total number of sectors on the volume. */

  fs->fs_fattotsec = MBR_GETTOTSEC16(fs->fs_buffer); /* Should be zero */
  if (fs->fs_fattotsec)
    {
      notfat32 = TRUE; /* Must be zero for FAT32 */
    }
  else
    {
      fs->fs_fattotsec = MBR_GETTOTSEC32(fs->fs_buffer);
    }

  if (!fs->fs_fattotsec || fs->fs_fattotsec > fs->fs_hwnsectors)
    {
      return -ENODEV;
    }

  /* Get the total number of reserved sectors */

  fs->fs_fatresvdseccount = MBR_GETRESVDSECCOUNT(fs->fs_buffer);
  if (fs->fs_fatresvdseccount > fs->fs_hwnsectors)
    {
      return -ENODEV;
    }

  /* Get the number of FATs. This is probably two but could have other values */

  fs->fs_fatnumfats = MBR_GETNUMFATS(fs->fs_buffer);
  fatsize = fs->fs_fatnumfats * fs->fs_fatsize;

  /* Get the total number of data sectors */

  ndatasectors = fs->fs_fattotsec - fs->fs_fatresvdseccount - fatsize - rootdirsectors;
  if (ndatasectors > fs->fs_hwnsectors)
    {
      return -ENODEV;
    }

  /* Get the sectors per cluster */

  fs->fs_fatsecperclus = MBR_GETSECPERCLUS(fs->fs_buffer);

  /* Calculate the number of clusters */

  fs->fs_nclusters = ndatasectors / fs->fs_fatsecperclus;

  /* Finally, the test: */

  if (fs->fs_nclusters < 4085)
    {
      fs->fs_fsinfo = 0;
      fs->fs_type   = FSTYPE_FAT12;
    }
  else if (fs->fs_nclusters < 65525)
    {
      fs->fs_fsinfo = 0;
      fs->fs_type   = FSTYPE_FAT16;
    }
  else if (!notfat32)
    {
      fs->fs_fsinfo   = fs->fs_fatbase + MBR_GETFSINFO(fs->fs_buffer);
      fs->fs_type     = FSTYPE_FAT32;
    }
  else
    {
      return -ENODEV;
    }

  /* We have what appears to be a valid FAT filesystem! Save a few more things
   * from the boot record that we will need later.
   */

  fs->fs_fatbase     += fs->fs_fatresvdseccount;

  if (fs->fs_type == FSTYPE_FAT32)
    {
      fs->fs_rootbase = MBR_GETROOTCLUS(fs->fs_buffer);
    }
  else
    {
      fs->fs_rootbase = fs->fs_fatbase + fatsize; 
    }

  fs->fs_database     = fs->fs_fatbase + fatsize + fs->fs_rootentcnt / DIRSEC_NDIRS(fs);
  fs->fs_fsifreecount = 0xffffffff;

  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: fat_getuint16
 ****************************************************************************/

uint16 fat_getuint16(ubyte *ptr)
{
#ifdef CONFIG_ARCH_BIGENDIAN
  /* The bytes always have to be swapped if the target is big-endian */

  return ((uint16)ptr[0] << 8) | ptr[1];
#else
  /* Byte-by-byte transfer is still necessary if the address is un-aligned */

  return ((uint16)ptr[1] << 8) | ptr[0];
#endif
}

/****************************************************************************
 * Name: fat_getuint32
 ****************************************************************************/

uint32 fat_getuint32(ubyte *ptr)
{
#ifdef CONFIG_ARCH_BIGENDIAN
  /* The bytes always have to be swapped if the target is big-endian */

  return ((uint32)fat_getuint16(&ptr[0]) << 16) | fat_getuint16(&ptr[2]);
#else
  /* Byte-by-byte transfer is still necessary if the address is un-aligned */

  return ((uint32)fat_getuint16(&ptr[2]) << 16) | fat_getuint16(&ptr[0]);
#endif
}

/****************************************************************************
 * Name: fat_putuint16
 ****************************************************************************/

void fat_putuint16(ubyte *ptr, uint16 value16)
{
  ubyte *val = (ubyte*)&value16;
#ifdef CONFIG_ARCH_BIGENDIAN
  /* The bytes always have to be swapped if the target is big-endian */

  ptr[0] = val[1];
  ptr[1] = val[0];
#else
  /* Byte-by-byte transfer is still necessary if the address is un-aligned */

  ptr[0] = val[0];
  ptr[1] = val[1];
#endif
}

/****************************************************************************
 * Name: fat_putuint32
 ****************************************************************************/

void fat_putuint32(ubyte *ptr, uint32 value32)
{
  uint16 *val = (uint16*)&value32;
#ifdef CONFIG_ARCH_BIGENDIAN
  /* The bytes always have to be swapped if the target is big-endian */

  fat_putuint16(&ptr[0], val[2]);
  fat_putuint16(&ptr[2], val[0]);
#else
  /* Byte-by-byte transfer is still necessary if the address is un-aligned */

  fat_putuint16(&ptr[0], val[0]);
  fat_putuint16(&ptr[2], val[2]);
#endif
}

/****************************************************************************
 * Name: fat_semtake
 ****************************************************************************/

void fat_semtake(struct fat_mountpt_s *fs)
{
  /* Take the semaphore (perhaps waiting) */

  while (sem_wait(&fs->fs_sem) != 0)
    {
      /* The only case that an error should occur here is if
       * the wait was awakened by a signal.
       */

      ASSERT(*get_errno_ptr() == EINTR);
    }
}

/****************************************************************************
 * Name: fat_semgive
 ****************************************************************************/

void fat_semgive(struct fat_mountpt_s *fs)
{
   sem_post(&fs->fs_sem);
}

/****************************************************************************
 * Name: fat_gettime
 *
 * Desciption: Get the time and date suitble for writing into the FAT FS.
 *    TIME in LS 16-bits:
 *      Bits 0:4   = 2 second count (0-29 representing 0-58 seconds)
 *      Bits 5-10  = minutes (0-59)
 *      Bits 11-15 = hours (0-23)
 *    DATE in MS 16-bits
 *      Bits 0:4   = Day of month (0-31)
 *      Bits 5:8   = Month of year (1-12)
 *      Bits 9:15  = Year from 1980 (0-127 representing 1980-2107)
 *
 *
 ****************************************************************************/

uint32 fat_gettime(void)
{
#warning "Time not implemented"
    return 0;
}

/****************************************************************************
 * Name: fat_mount
 *
 * Desciption: This function is called only when the mountpoint is first
 *   established.  It initializes the mountpoint structure and verifies
 *   that a valid FAT32 filesystem is provided by the block driver.
 *
 *   The caller should hold the mountpoint semaphore
 *
 ****************************************************************************/

int fat_mount(struct fat_mountpt_s *fs, boolean writeable)
{
  FAR struct inode *inode;
  struct geometry geo;
  int ret;

  /* Assume that the mount is successful */

  fs->fs_mounted = TRUE;

  /* Check if there is media available */

  inode = fs->fs_blkdriver;
  if (!inode || !inode->u.i_bops || !inode->u.i_bops->geometry ||
      inode->u.i_bops->geometry(inode, &geo) != OK || !geo.geo_available)
    {
      ret = -ENODEV;
      goto errout;
    }

  /* Make sure that that the media is write-able (if write access is needed) */

  if (writeable && !geo.geo_writeenabled)
    {
      ret = -EACCES;
      goto errout;
    }

  /* Save the hardware geometry */

  fs->fs_hwsectorsize = geo.geo_sectorsize;
  fs->fs_hwnsectors   = geo.geo_nsectors;

  /* Allocate a buffer to hold one hardware sector */

  fs->fs_buffer = (ubyte*)malloc(fs->fs_hwsectorsize);
  if (!fs->fs_buffer)
    {
      ret = -ENOMEM;
      goto errout;
    }

  /* Search FAT boot record on the drive.  First check at sector zero.  This
   * could be either the boot record or a partition that refers to the boot
   * record.
   *
   * First read sector zero.  This will be the first access to the drive and a
   * likely failure point.
   */

  fs->fs_fatbase = 0;
  ret = fat_hwread(fs, fs->fs_buffer, 0, 1);
  if (ret < 0)
    {
      goto errout_with_buffer;
    }

  if (fat_checkbootrecord(fs) != OK)
    {
      /* The contents of sector 0 is not a boot record.  It could be a
       * partition, however.  Assume it is a partition and get the offset
       * into the partition table.  This table is at offset MBR_TABLE and is
       * indexed by 16x the partition number.  Here we support only
       * parition 0.
       */

      ubyte *partition = &fs->fs_buffer[MBR_TABLE + 0];

      /* Check if the partition exists and, if so, get the bootsector for that
       * partition and see if we can find the boot record there.
       */
 
      if (partition[4])
        {
          /* There appears to be a partition, get the sector number of the
           * partition (LBA)
           */

          fs->fs_fatbase = MBR_GETPARTSECTOR(&partition[8]);

          /* Read the new candidate boot sector */

          ret = fat_hwread(fs, fs->fs_buffer, fs->fs_fatbase, 1);
          if (ret < 0)
          {
              goto errout_with_buffer;
          }

          /* Check if this is a boot record */

          if (fat_checkbootrecord(fs) != OK)
            {
              goto errout_with_buffer;
            }
        }
    }

  /* We have what appears to be a valid FAT filesystem! Now read the
   * FSINFO sector (FAT32 only)
   */

  if (fs->fs_type == FSTYPE_FAT32)
  {
      ret = fat_checkfsinfo(fs);
      if (ret != OK)
      {
          goto errout_with_buffer;
      }
  }

  /* We did it! */

  dbg("FAT%d:\n", fs->fs_type == 0 ? 12 : fs->fs_type == 1  ? 16 : 32);
  dbg("\tHW  sector size:     %d\n", fs->fs_hwsectorsize);
  dbg("\t    sectors:         %d\n", fs->fs_hwnsectors);
  dbg("\tFAT reserved:        %d\n", fs->fs_fatresvdseccount);
  dbg("\t    sectors:         %d\n", fs->fs_fattotsec);
  dbg("\t    start sector:    %d\n", fs->fs_fatbase);
  dbg("\t    root sector:     %d\n", fs->fs_rootbase);
  dbg("\t    root entries:    %d\n", fs->fs_rootentcnt);
  dbg("\t    data sector:     %d\n", fs->fs_database);
  dbg("\t    FSINFO sector:   %d\n", fs->fs_fsinfo);
  dbg("\t    Num FATs:        %d\n", fs->fs_fatnumfats);
  dbg("\t    FAT size:        %d\n", fs->fs_fatsize);
  dbg("\t    sectors/cluster: %d\n", fs->fs_fatsecperclus);
  dbg("\t    max clusters:    %d\n", fs->fs_nclusters);
  dbg("\tFSI free count       %d\n", fs->fs_fsifreecount);
  dbg("\t    next free        %d\n", fs->fs_fsinextfree);

  return OK;

 errout_with_buffer:
  free(fs->fs_buffer);
  fs->fs_buffer = 0;
 errout:
  fs->fs_mounted = FALSE;
  return ret;
}

/****************************************************************************
 * Name: fat_checkmount
 *
 * Desciption: Check if the mountpoint is still valid.
 *
 *   The caller should hold the mountpoint semaphore
 *
 ****************************************************************************/

int fat_checkmount(struct fat_mountpt_s *fs)
{
  /* If the fs_mounted flag is FALSE, then we have already handled the loss
   * of the mount.
   */

  if (fs && fs->fs_mounted)
    {
      struct fat_file_s *file;

      /* We still think the mount is healthy.  Check an see if this is
       * still the case
       */

      if (fs->fs_blkdriver)
        {
          struct inode *inode = fs->fs_blkdriver;
          if (inode && inode->u.i_bops && inode->u.i_bops->geometry)
            {
              struct geometry geo;
              int errcode = inode->u.i_bops->geometry(inode, &geo);
              if (errcode == OK && geo.geo_available && !geo.geo_mediachanged)
                {
                  return OK;
                }
            }
        }

      /* If we get here, the mount is NOT healthy */

      fs->fs_mounted = FALSE;

      /* Make sure that this is flagged in every opened file */

      for (file = fs->fs_head; file; file = file->ff_next)
        {
          file->ff_open = FALSE;
        }
    }
  return -ENODEV;
}

/****************************************************************************
 * Name: fat_hwread
 *
 * Desciption: Read the specified sector into the sector buffer
 *
 ****************************************************************************/

int fat_hwread(struct fat_mountpt_s *fs, ubyte *buffer,  size_t sector,
               unsigned int nsectors)
{
  int ret = -ENODEV;
  if (fs && fs->fs_blkdriver )
    {
      struct inode *inode = fs->fs_blkdriver;
      if (inode && inode->u.i_bops && inode->u.i_bops->read)
        {
          ssize_t nSectorsRead = inode->u.i_bops->read(inode, buffer,
                                                       sector, nsectors);
          if (nSectorsRead == nsectors)
            {
              ret = OK;
            }
          else if (nSectorsRead < 0)
            {
              ret = nSectorsRead;
            }
        }
    }
  return ret;
}

/****************************************************************************
 * Name: fat_hwwrite
 *
 * Desciption: Write the sector buffer to the specified sector
 *
 ****************************************************************************/

int fat_hwwrite(struct fat_mountpt_s *fs, ubyte *buffer, size_t sector,
                unsigned int nsectors)
{
  int ret = -ENODEV;
  if (fs && fs->fs_blkdriver )
    {
      struct inode *inode = fs->fs_blkdriver;
      if (inode && inode->u.i_bops && inode->u.i_bops->write)
        {
          ssize_t nSectorsWritten =
              inode->u.i_bops->write(inode, buffer, sector, nsectors);

          if (nSectorsWritten == nsectors)
            {
              ret = OK;
            }
          else if (nSectorsWritten < 0)
            {
              ret = nSectorsWritten;
            }
        }
    }
  return ret;
}

/****************************************************************************
 * Name: fat_cluster2sector
 *
 * Desciption: Convert a cluster number to a start sector number
 *
 ****************************************************************************/

ssize_t fat_cluster2sector(struct fat_mountpt_s *fs,  uint32 cluster )
{
  cluster -= 2;
  if (cluster >= fs->fs_nclusters - 2)
    {
       return -EINVAL;
    }
  return cluster * fs->fs_fatsecperclus + fs->fs_database;
}

/****************************************************************************
 * Name: fat_getcluster
 *
 * Desciption: Get the cluster start sector into the FAT.
 *
 * Return:  <0: error, >=0: sector number
 *
 ****************************************************************************/

ssize_t fat_getcluster(struct fat_mountpt_s *fs, uint32 clusterno)
{
  /* Verify that the cluster number is within range */

  if (clusterno >= 2 && clusterno < fs->fs_nclusters)
    {
      /* Okay.. Read the next cluster from the FAT.  The way we will do
       * this depends on the type of FAT filesystm we are dealing with.
       */

      switch (fs->fs_type)
        {
          case FSTYPE_FAT12 :
            {
              size_t       fatsector;
              unsigned int fatoffset;
              unsigned int startsector;
              unsigned int fatindex;

              /* FAT12 is more complex because it has 12-bits (1.5 bytes)
               * per FAT entry. Get the offset to the first byte:
               */

              fatoffset = (clusterno * 3) / 2;
              fatsector = fs->fs_fatbase + SEC_NSECTORS(fs, fatoffset);

              /* Read the sector at this offset */

              if (fat_fscacheread(fs, fatsector) < 0)
                {
                  /* Read error */
                  break;
                }

              /* Get the first, LS byte of the cluster from the FAT */

              fatindex    = fatoffset & SEC_NDXMASK(fs);
              startsector = fs->fs_buffer[fatindex];

              /* With FAT12, the second byte of the cluster number may lie in
               * a different sector than the first byte.
               */

              fatindex++;
              if (fatindex >= fs->fs_hwsectorsize)
                {
                  fatsector++;
                  fatindex = 0;

                  if (fat_fscacheread(fs, fatsector) < 0)
                    {
                      /* Read error */
                      break;
                    }
                }

              /* Get the second, MS byte of the cluster for 16-bits.  The
               * does not depend on the endian-ness of the target, but only
               * on the fact that the byte stream is little-endian.
               */

              startsector |= (unsigned int)fs->fs_buffer[fatindex] << 8;

              /* Now, pick out the correct 12 bit cluster start sector value */

              if ((clusterno & 1) != 0)
                {
                  /* Odd.. take the MS 12-bits */
                  startsector >>= 4;
                }
              else
                {
                  /* Even.. take the LS 12-bits */
                  startsector &= 0x0fff;
                }
              return startsector;
            }

          case FSTYPE_FAT16 :
            {
              unsigned int fatoffset = 2 * clusterno;
              size_t       fatsector = fs->fs_fatbase + SEC_NSECTORS(fs, fatoffset);
              unsigned int fatindex  = fatoffset & SEC_NDXMASK(fs);

              if (fat_fscacheread(fs, fatsector) < 0)
                {
                  /* Read error */
                  break;
                }
              return FAT_GETFAT16(fs->fs_buffer, fatindex);
            }

          case FSTYPE_FAT32 :
            {
              unsigned int fatoffset = 4 * clusterno;
              size_t       fatsector = fs->fs_fatbase + SEC_NSECTORS(fs, fatoffset);
              unsigned int fatindex  = fatoffset & SEC_NDXMASK(fs);

              if (fat_fscacheread(fs, fatsector) < 0)
                {
                  /* Read error */
                  break;
                }
              return FAT_GETFAT16(fs->fs_buffer, fatindex) & 0x0fffffff;
            }
          default:
              break;
        }
    }

  /* There is no cluster information, or an error occured */

  return (ssize_t)-EINVAL;
}

/****************************************************************************
 * Name: fat_putcluster
 *
 * Desciption: Write a new cluster start sector into the FAT
 *
 ****************************************************************************/

int fat_putcluster(struct fat_mountpt_s *fs, uint32 clusterno, size_t startsector)
{
  /* Verify that the cluster number is within range.  Zero erases the cluster. */

  if (clusterno == 0 || (clusterno >= 2 && clusterno < fs->fs_nclusters))
    {
      /* Okay.. Write the next cluster into the FAT.  The way we will do
       * this depends on the type of FAT filesystm we are dealing with.
       */

      switch (fs->fs_type)
        {
          case FSTYPE_FAT12 :
            {
              size_t       fatsector;
              unsigned int fatoffset;
              unsigned int fatindex;
              ubyte        value;

              /* FAT12 is more complex because it has 12-bits (1.5 bytes)
               * per FAT entry. Get the offset to the first byte:
               */

              fatoffset = (clusterno * 3) / 2;
              fatsector = fs->fs_fatbase + SEC_NSECTORS(fs, fatoffset);

              /* Make sure that the sector at this offset is in the cache */

              if (fat_fscacheread(fs, fatsector)< 0)
                {
                  /* Read error */
                  break;
                }

              /* Output the LS byte first handling the 12-bit alignment within
               * the 16-bits
               */

              fatindex = fatoffset & SEC_NDXMASK(fs);
              if ((clusterno & 1) != 0)
                {
                  value = (fs->fs_buffer[fatindex] & 0x0f) | startsector << 4;
                }
              else
                {
                  value = (ubyte)startsector;
                }
              fs->fs_buffer[fatindex] = value;

              /* With FAT12, the second byte of the cluster number may lie in
               * a different sector than the first byte.
               */

              fatindex++;
              if (fatindex >= fs->fs_hwsectorsize)
                {
                  /* Read the next sector */

                  fatsector++;
                  fatindex = 0;

                  /* Set the dirty flag to make sure the sector that we
                   * just modified is written out.
                   */

                  fs->fs_dirty = TRUE;
                  if (fat_fscacheread(fs, fatsector) < 0)
                    {
                      /* Read error */
                      break;
                    }
                }

              /* Output the MS byte first handling the 12-bit alignment within
               * the 16-bits
               */

              if ((clusterno & 1) != 0)
                {
                  value = (ubyte)(startsector >> 4);
                }
              else
                {
                  value = (fs->fs_buffer[fatindex] & 0xf0) | (startsector & 0x0f);
                }
              fs->fs_buffer[fatindex] = value;
            }
          break;

          case FSTYPE_FAT16 :
            {
              unsigned int fatoffset = 2 * clusterno;
              size_t       fatsector = fs->fs_fatbase + SEC_NSECTORS(fs, fatoffset);
              unsigned int fatindex  = fatoffset & SEC_NDXMASK(fs);

              if (fat_fscacheread(fs, fatsector) < 0)
                {
                  /* Read error */
                  break;
                }
              FAT_PUTFAT16(fs->fs_buffer, fatindex, startsector & 0xffff);
            }
          break;

          case FSTYPE_FAT32 :
            {
              unsigned int fatoffset = 4 * clusterno;
              size_t       fatsector = fs->fs_fatbase + SEC_NSECTORS(fs, fatoffset);
              unsigned int fatindex  = fatoffset & SEC_NDXMASK(fs);

              if (fat_fscacheread(fs, fatsector) < 0)
                {
                  /* Read error */
                  break;
                }
              FAT_PUTFAT32(fs->fs_buffer, fatindex, startsector & 0x0fffffff);
            }
          break;

          default:
              return -EINVAL;
        }

      /* Mark the modified sector as "dirty" and return success */

      fs->fs_dirty = 1;
      return OK;
    }

  return -EINVAL;
}

/****************************************************************************
 * Name: fat_removechain
 *
 * Desciption: Remove an entire chain of clusters, starting with 'cluster'
 *
 ****************************************************************************/

int fat_removechain(struct fat_mountpt_s *fs, uint32 cluster)
{
  sint32 nextcluster;
  int    ret;

  /* Loop while there are clusters in the chain */

  while (cluster >= 2 && cluster < fs->fs_nclusters)
    {
      /* Get the next cluster after the current one */

      nextcluster = fat_getcluster(fs, cluster);
      if (nextcluster < 0)
        {
          /* Error! */
          return nextcluster;
        }

      /* Then nullify current cluster -- removing it from the chain */

      ret = fat_putcluster(fs, cluster, 0);
      if (ret < 0)
        {
          return ret;
        }

      /* Update FSINFINFO data */

      if (fs->fs_fsifreecount != 0xffffffff)
        {
          fs->fs_fsifreecount++;
          fs->fs_fsidirty = 1;
        }

      /* Then set up to remove the next cluster */

      cluster = nextcluster;
  }

  return OK;
}

/****************************************************************************
 * Name: fat_extendchain
 *
 * Desciption: Add a new cluster to the chain following cluster (if cluster
 *   is non-NULL).  if cluster is zero, then a new chain is created.
 *
 * Return: <0:error, 0: no free cluster, >=2: new cluster number
 *
 ****************************************************************************/

sint32 fat_extendchain(struct fat_mountpt_s *fs, uint32 cluster)
{
  ssize_t startsector;
  uint32  newcluster;
  uint32  startcluster;
  int     ret;

  /* The special value 0 is used when the new chain should start */

  if (cluster == 0)
    {
      /* The FSINFO NextFree entry should be a good starting point
       * in the search for a new cluster
       */

      startcluster = fs->fs_fsinextfree;
      if (startcluster == 0 || startcluster >= fs->fs_nclusters)
        {
          /* But it is bad.. we have to start at the beginning */
          startcluster = 1;
        }
    }
  else
    {
      /* We are extending an existing chain. Verify that this
       * is a valid cluster by examining its start sector.
       */

      startsector = fat_getcluster(fs, cluster);
      if (startsector < 0)
        {
          /* An error occurred, return the error value */
          return startsector;
        }
      else if (startsector < 2)
        {
          /* Oops.. this cluster does not exist. */
          return 0;
        }
      else if (startsector < fs->fs_nclusters)
        {
          /* It is already followed by next cluster */
          return startsector;
        }

      /* Okay.. it checks out */

      startcluster = cluster;
    }

  /* Loop until (1) we discover that there are not free clusters
   * (return 0), an errors occurs (return -errno), or (3) we find
   * the next cluster (return the new cluster number).
   */

  newcluster = startcluster;
  for (;;)
    {
      /* Examine the next cluster in the FAT */

      newcluster++;
      if (newcluster >= fs->fs_nclusters)
        {
          /* If we hit the end of the available clusters, then
           * wrap back to the beginning because we might have
           * started at a non-optimal place.  But don't continue
           * past the start cluster.
           */

          newcluster = 2;
          if (newcluster > startcluster)
            {
              /* We are back past the starting cluster, then there
               * is no free cluster.
               */

              return 0;
            }
        }

      /* We have a candidate cluster.  Check if the cluster number is
       * mapped to a group of sectors.
       */

      startsector = fat_getcluster(fs, newcluster);
      if (startsector == 0)
        {
          /* Found have found a free cluster break out*/
          break;
        }
      else if (startsector < 0)
        {
          /* Some error occurred, return the error number */
          return startsector;
        }

      /* We wrap all the back to the starting cluster?  If so, then
       * there are no free clusters.
       */

      if (newcluster == startcluster)
        {
          return 0;
        }
    }

  /* We get here only if we break out with an available cluster
   * number in 'newcluster'  Now mark that cluster as in-use.
   */

  ret = fat_putcluster(fs, newcluster, 0x0fffffff);
  if (ret < 0)
    {
      /* An error occurred */ 
      return ret;
    }

  /* And link if to the start cluster (if any)*/

  if (cluster)
    {
      /* There is a start cluster -- link it */

      ret = fat_putcluster(fs, cluster, newcluster);
      if (ret < 0)
        {
          return ret;
        }
    }

  /* And update the FINSINFO for the next time we have to search */

  fs->fs_fsinextfree = newcluster;
  if (fs->fs_fsifreecount != 0xffffffff)
    {
      fs->fs_fsifreecount--;
      fs->fs_fsidirty = 1;
    }

  /* Return then number of the new cluster that was added to the chain */

  return newcluster;
}

/****************************************************************************
 * Name: fat_nextdirentry
 *
 * Desciption: Read the next directory entry from the sector in cache,
 *   reading the next sector(s) in the cluster as necessary.
 *
 ****************************************************************************/

int fat_nextdirentry(struct fat_mountpt_s *fs, struct fat_dirinfo_s *dirinfo)
{
  unsigned int cluster;
  unsigned int ndx;

  /* Increment the index to the next 32-byte directory entry */

  ndx = dirinfo->fd_index + 1;

  /* Check if all of the directory entries in this sectory have
   * been examined.
   */

  if (ndx >= DIRSEC_NDIRS(fs))
    {
      /* Yes, then we will have to read the next sector */

      dirinfo->fd_currsector++;

      /* For FAT12/16, the root directory is a group of sectors relative
       * to the first sector of the fat volume.
       */

      if (!dirinfo->fd_currcluster)
        {
          /* For FAT12/13, the boot record tells us number of 32-bit directories
           * that are contained in the root directory.  This should correspond to
           * an even number of sectors.
           */

          if (ndx >= fs->fs_rootentcnt)
            {
              /* When we index past this count, we have examined all of the entries in
               * the root directory.
               */

              return ERROR;
            }
        }
      else
        {
          /* Not a FAT12/16 root directory, check if we have examined the entire
           * cluster comprising the directory.
           *
           * The current sector within the cluster is the entry number divided
           * byte the number of entries per sector
           */

          int sector = ndx / DIRSEC_NDIRS(fs);

          /* We are finished with the cluster when the last sector of the cluster
           * has been examined.
           */

          if (sector >= fs->fs_fatsecperclus)
            {
              /* Get next cluster */

              cluster = fat_getcluster(fs, dirinfo->fd_currcluster);

              /* Check if a valid cluster was obtained. */

              if (cluster < 2 || cluster >= fs->fs_nclusters)
                {
                  /* No, we have probably reached the end of the cluster list */
                  return ERROR;
                }

              /* Initialize for new cluster */

              dirinfo->fd_currcluster = cluster;
              dirinfo->fd_currsector  = fat_cluster2sector(fs, cluster);
            }
        }
    }

  /* Save the new index into dirinfo->fd_currsector */

  dirinfo->fd_index = ndx;
  return OK;
}

/****************************************************************************
 * Name: fat_finddirentry
 *
 * Desciption: Given a path to something that may or may not be in the file
 *   system, return the directory entry of the item.
 *
 ****************************************************************************/

int fat_finddirentry(struct fat_mountpt_s *fs, struct fat_dirinfo_s *dirinfo,
                     const char *path)
{
  size_t cluster;
  ubyte *direntry = NULL;
  char terminator;
  int ret;

  /* Initialize to traverse the chain.  Set it to the cluster of
   * the root directory
   */

  cluster = fs->fs_rootbase;
  if (fs->fs_type == FSTYPE_FAT32)
    {
      /* For FAT32, the root directory is variable sized and is a
       * cluster chain like any other directory.  fs_rootbase holds
       * the first cluster of the root directory.
       */

      dirinfo->fd_startcluster = cluster;
      dirinfo->fd_currcluster  = cluster;
      dirinfo->fd_currsector   = fat_cluster2sector(fs, cluster);
    }
  else
    {
      /* For FAT12/16, the first sector of the root directory is a sector
       * relative to the first sector of the fat volume.
       */

      dirinfo->fd_startcluster = 0;
      dirinfo->fd_currcluster  = 0;
      dirinfo->fd_currsector   = cluster;
    }

  /* fd_index is the index into the current directory table */

  dirinfo->fd_index = 0;

  /* If no path was provided, then the root directory must be exactly
   * what the caller is looking for.
   */

  if (*path == '\0')
    {
      dirinfo->fd_entry = NULL;
      return OK;
    }

  /* Otherwise, loop until the path is found */

  for (;;)
    {
      /* Convert the next the path segment name into the kind of
       * name that we would see in the directory entry.
       */

      ret = fat_path2dirname(&path, dirinfo, &terminator);
      if (ret < 0)
        {
          /* ERROR:  The filename contains invalid characters or is
           * too long.
           */

          return ret;
        }

      /* Now search the current directory entry for an entry with this
       * matching name.
       */

      for (;;)
        {
          /* Read the next sector into memory */

          ret = fat_fscacheread(fs, dirinfo->fd_currsector);
          if (ret < 0)
            {
              return ret;
            }

          /* Get a pointer to the directory entry */

          direntry = &fs->fs_buffer[DIRSEC_BYTENDX(fs, dirinfo->fd_index)];

          /* Check if we are at the end of the directory */

          if (direntry[DIR_NAME] == DIR0_ALLEMPTY)
            {
              return -ENOENT;
            }

          /* Check if we have found the directory entry that we are looking for */

          if (direntry[DIR_NAME] != DIR0_EMPTY &&
              !(DIR_GETATTRIBUTES(direntry) & FATATTR_VOLUMEID) &&
              !memcmp(&direntry[DIR_NAME], dirinfo->fd_name, 8+3) )
            {
              /* Yes.. break out of the loop */
              break;
            }

          /* No... get the next directory index and try again */

          if (fat_nextdirentry(fs, dirinfo) != OK)
            {
              return -ENOENT;
            }
        }

      /* We get here only if we have found a directory entry that matches
       * the path element that we are looking for.
       *
       * If the terminator character in the path was the end of the string
       * then we have successfully found the directory entry that describes
       * the path.
       */

      if (!terminator)
        {
          /* Return the pointer to the matching directory entry */
          dirinfo->fd_entry = direntry;
          return OK;
        }

      /* No.. then we have found one of the intermediate directories on
       * the way to the final path target.  In this case, make sure
       * the thing that we found is, indeed, a directory.
       */

      if (!(DIR_GETATTRIBUTES(direntry) & FATATTR_DIRECTORY))
        {
          /* Ooops.. we found something else */
          return -ENOTDIR;
        }

      /* Get the cluster number of this directory */

      cluster =
          ((uint32)DIR_GETFSTCLUSTHI(direntry) << 16) |
          DIR_GETFSTCLUSTLO(direntry);

      /* The restart scanning at the new directory */

      dirinfo->fd_currcluster = dirinfo->fd_startcluster = cluster;
      dirinfo->fd_currsector  = fat_cluster2sector(fs, cluster);
      dirinfo->fd_index       = 2;
    }
}

/****************************************************************************
 * Name: fat_allocatedirentry
 *
 * Desciption: Find a free directory entry
 *
 ****************************************************************************/

int fat_allocatedirentry(struct fat_mountpt_s *fs, struct fat_dirinfo_s *dirinfo)
{
  sint32 cluster;
  size_t sector;
  ubyte *direntry;
  ubyte  ch;
  int    ret;
  int    i;

  /* Re-initialize directory object */

  cluster = dirinfo->fd_startcluster;
  if (cluster)
    {
     /* Cluster chain can be extended */

      dirinfo->fd_currcluster = cluster;
      dirinfo->fd_currsector  = fat_cluster2sector(fs, cluster);
    }
  else
    {
      /* Fixed size FAT12/16 root directory is at fixxed offset/size */

      dirinfo->fd_currsector = fs->fs_rootbase;
    }
  dirinfo->fd_index = 0;

  for (;;)
    {
      unsigned int dirindex;

      /* Read the directory sector into fs_buffer */

      ret = fat_fscacheread(fs, dirinfo->fd_currsector);
      if (ret < 0)
        {
          return ret;
        }

      /* Get a pointer to the entry at fd_index */

      dirindex = (dirinfo->fd_index & DIRSEC_NDXMASK(fs)) * 32;
      direntry = &fs->fs_buffer[dirindex];

      /* Check if this directory entry is empty */

      ch = direntry[DIR_NAME];
      if (ch == DIR0_ALLEMPTY || ch == DIR0_EMPTY)
        {
          /* It is empty -- we have found a directory entry */

          dirinfo->fd_entry = direntry;
          return OK;
        }

      ret = fat_nextdirentry(fs, dirinfo);
      if (ret < 0)
        {
          return ret;
        }
    }

  /* If we get here, then we have reached the end of the directory table
   * in this sector without finding a free directory enty.
   *
   * It this is a fixed size dirctory entry, then this is an error.
   * Otherwise, we can try to extend the directory cluster chain to
   * make space for the new directory entry.
   */

  if (!cluster)
    {
      /* The size is fixed */
      return -ENOSPC;
    }

  /* Try to extend the cluster chain for this directory */

  cluster = fat_extendchain(fs, dirinfo->fd_currcluster);
  if (cluster < 0)
    {
      return cluster;
    }

 /* Flush out any cached date in fs_buffer.. we are going to use
  * it to initialize the new directory cluster.
  */

  ret = fat_fscacheflush(fs);
  if (ret < 0)
    {
      return ret;
    }

  /* Clear all sectors comprising the new directory cluster */

  fs->fs_currentsector = fat_cluster2sector(fs, cluster);
  memset(fs->fs_buffer, 0, fs->fs_hwsectorsize);

  sector = sector;
  for (i = fs->fs_fatsecperclus; i; i--)
    {
      ret = fat_hwwrite(fs, fs->fs_buffer, sector, 1);
      if ( ret < 0)
        {
          return ret;
        }
      sector++;
    }

  dirinfo->fd_entry = fs->fs_buffer;
  return OK;
}

/****************************************************************************
 * Name: fat_dirtruncate
 *
 * Desciption: Truncate an existing file to zero length
 *
 * Assumptions:  The caller holds mountpoint semaphore, fs_buffer holds
 *   the directory entry, dirinfo refers to the current fs_buffer content.
 *
 ****************************************************************************/

int  fat_dirtruncate(struct fat_mountpt_s *fs, struct fat_dirinfo_s *dirinfo)
{
  unsigned int startcluster;
  uint32       writetime;
  size_t       savesector;
  int          ret;

  /* Get start cluster of the file to truncate */

  startcluster =
      ((uint32)DIR_GETFSTCLUSTHI(dirinfo->fd_entry) << 16) |
      DIR_GETFSTCLUSTLO(dirinfo->fd_entry);

  /* Clear the cluster start value in the directory and set the file size
   * to zero.  This makes the file look empty but also have to dispose of
   * all of the clusters in the chain.
   */

  DIR_PUTFSTCLUSTHI(dirinfo->fd_entry, 0);
  DIR_PUTFSTCLUSTLO(dirinfo->fd_entry, 0);
  DIR_PUTFILESIZE(dirinfo->fd_entry, 0);

  /* Set the ARCHIVE attribute and update the write time */

  DIR_PUTATTRIBUTES(dirinfo->fd_entry, FATATTR_ARCHIVE);
 
  writetime = fat_gettime();
  DIR_PUTWRTTIME(dirinfo->fd_entry, writetime & 0xffff);
  DIR_PUTWRTDATE(dirinfo->fd_entry, writetime > 16);

  /* This sector needs to be written back to disk eventually */

  fs->fs_dirty = TRUE;

  /* Now remove the entire cluster chain comprising the file */

  savesector = fs->fs_currentsector;
  ret = fat_removechain(fs, startcluster);
  if (ret < 0)
  {
    return ret;
  }

  /* Setup FSINFO to resuse this cluster next */

  fs->fs_fsinextfree = startcluster - 1;

  /* Make sure that the directory is still in the cache */

  return fat_fscacheread(fs, savesector);
}

/****************************************************************************
 * Name: fat_dircreate
 *
 * Desciption: Create a directory entry for a new file
 *
 ****************************************************************************/

int fat_dircreate(struct fat_mountpt_s *fs, struct fat_dirinfo_s *dirinfo)
{
  ubyte  *direntry;
  uint32  time;
  int     ret;

  /* Set up the directory entry */

  ret = fat_allocatedirentry(fs, dirinfo);
  if (ret != OK)
    {
      /* Failed to set up directory entry */
      return ret;
    }

  /* Initialize the 32-byte directory entry */

  direntry = dirinfo->fd_entry;
  memset(direntry, 0, 32);

  /* Directory name info */

  memcpy(&direntry[DIR_NAME], dirinfo->fd_name, 8+3);
#ifdef CONFIG_FLAT_LCNAMES
  DIR_PUTNTRES(dirinfo->fd_entry, dirinfo->fd_ntflags);
#else
  DIR_PUTNTRES(dirinfo->fd_entry, 0);
#endif

  /* ARCHIVE attribute, write time, creation time */
  DIR_PUTATTRIBUTES(dirinfo->fd_entry, FATATTR_ARCHIVE);
 
  time = fat_gettime();
  DIR_PUTWRTTIME(dirinfo->fd_entry, time & 0xffff);
  DIR_PUTCRTIME(dirinfo->fd_entry, time & 0xffff);
  DIR_PUTWRTDATE(dirinfo->fd_entry, time >> 16);
  DIR_PUTCRDATE(dirinfo->fd_entry, time >> 16);

  fs->fs_dirty = TRUE;
  return OK;
}

/****************************************************************************
 * Name: fat_remove
 *
 * Desciption: Remove a directory or file from the file system.  This
 *   implements both rmdir() and unlink().
 *
 ****************************************************************************/

int fat_remove(struct fat_mountpt_s *fs, const char *relpath, boolean directory)
{
  struct fat_dirinfo_s dirinfo;
  uint32  dircluster;
  size_t  dirsector;
  int     ret;

  /* Find the directory entry referring to the entry to be deleted */

  ret = fat_finddirentry(fs, &dirinfo, relpath);
  if (ret != OK)
    {
      /* No such path */

      return -ENOENT;
    }

  /* Check if this is a FAT12/16 root directory */

  if (dirinfo.fd_entry == NULL)
    {
      /* The root directory cannot be removed */

      return -EPERM;
    }

  /* The object has to have write access to be deleted */

  if ((DIR_GETATTRIBUTES(dirinfo.fd_entry) & FATATTR_READONLY) != 0)
    {
      /* It is a read-only entry */

      return -EACCES;
    }

  /* Get the directory sector and cluster containing the
   * entry to be deleted
   */

  dirsector  = fs->fs_currentsector;
  dircluster =
      ((uint32)DIR_GETFSTCLUSTHI(dirinfo.fd_entry) << 16) |
      DIR_GETFSTCLUSTLO(dirinfo.fd_entry);

  /* Is this entry a directory? */

  if (DIR_GETATTRIBUTES(dirinfo.fd_entry) & FATATTR_DIRECTORY)
    {
      /* It is a sub-directory. Check if we are be asked to remove
       * a directory or a file.
       */

      if (!directory)
        {
          /* We are asked to delete a file */

          return -EISDIR;
        }

      /* We are asked to delete a directory. Check if this
       * sub-directory is empty
       */

      dirinfo.fd_currcluster = dircluster;
      dirinfo.fd_currsector  = fat_cluster2sector(fs, dircluster);
      dirinfo.fd_index       = 2;

      /* Loop until either (1) an entry is found in the directory
       * (error), (2) the directory is found to be empty, or (3) some
       * error occurs.
       */

      for (;;)
        {
          unsigned int subdirindex;
          ubyte       *subdirentry;

          /* Make sure that the sector containing the of the
           * subdirectory sector is in the cache
           */

          ret = fat_fscacheread(fs, dirinfo.fd_currsector);
          if (ret < 0)
            {
              return ret;
            }

          /* Get a reference to the next entry in the directory */

          subdirindex = (dirinfo.fd_index & DIRSEC_NDXMASK(fs)) * 32;
          subdirentry = &fs->fs_buffer[subdirindex];

          /* Is this the last entry in the direcory? */

          if (subdirentry[DIR_NAME] == DIR0_ALLEMPTY)
            {
              /* Yes then the directory is empty.  Break out of the
               * loop and delete the directory.
               */

              break;
            }

          /* Check if the next entry refers to a file or directory */

          if (subdirentry[DIR_NAME] != DIR0_EMPTY &&
              !(DIR_GETATTRIBUTES(subdirentry) & FATATTR_VOLUMEID))
            {
              /* The directory is not empty */

              return -ENOTEMPTY;
            }

          /* Get the next directgory entry */

          ret = fat_nextdirentry(fs, &dirinfo);
          if (ret < 0)
            {
              return ret;
            }
        }
    }
  else
    {
      /* It is a file. Check if we are be asked to remove a directory
       * or a file.
       */

      if (directory)
        {
          /* We are asked to remove a directory */

          return -ENOTDIR;
        }
    }

  /* Make sure that the directory containing the entry to be deleted is
   * in the cache.
   */

  ret = fat_fscacheread(fs, dirsector);
  if (ret < 0)
    {
      return ret;
    }

  /* Mark the directory entry 'deleted' */

  dirinfo.fd_entry[DIR_NAME] = DIR0_EMPTY;
  fs->fs_dirty = TRUE;

  /* And remove the cluster chain making up the subdirectory */

  ret = fat_removechain(fs, dircluster);
  if (ret < 0)
    {
      return ret;
    }

  /* Update the FSINFO sector (FAT32) */

  ret = fat_updatefsinfo(fs);
  if (ret < 0)
    {
      return ret;
    }

  return OK;
}

/****************************************************************************
 * Name: fat_fscacheflush
 *
 * Desciption: Flush any dirty sector if fs_buffer as necessary
 *
 ****************************************************************************/

int fat_fscacheflush(struct fat_mountpt_s *fs)
{
  int ret;

  /* Check if the fs_buffer is dirty.  In this case, we will write back the
   * contents of fs_buffer.
   */

  if (fs->fs_dirty)
  {
      /* Write the dirty sector */

      ret = fat_hwwrite(fs, fs->fs_buffer, fs->fs_currentsector, 1);
      if (ret < 0)
      {
          return ret;
      }

      /* Does the sector lie in the FAT region? */

      if (fs->fs_currentsector < fs->fs_fatbase + fs->fs_fatsize)
      {
          /* Yes, then make the change in the FAT copy as well */
          int i;

          for (i = fs->fs_fatnumfats; i >= 2; i--)
          { 
              fs->fs_currentsector += fs->fs_fatsize;
              ret = fat_hwwrite(fs, fs->fs_buffer, fs->fs_currentsector, 1);
              if (ret < 0)
              {
                  return ret;
              }
          }
      }

      /* No longer dirty */

      fs->fs_dirty = FALSE;
  }
  return OK;
}

/****************************************************************************
 * Name: fat_fscacheread
 *
 * Desciption: Read the specified sector into the sector cache, flushing any
 *   existing dirty sectors as necessary.
 *
 ****************************************************************************/

int fat_fscacheread(struct fat_mountpt_s *fs, size_t sector)
{
  int ret;

  /* fs->fs_currentsector holds the current sector that is buffered in
   * fs->fs_buffer. If the requested sector is the same as this sector, then
   * we do nothing. Otherwise, we will have to read the new sector.
   */

    if (fs->fs_currentsector != sector)
      {
        /* We will need to read the new sector.  First, flush the cached
         * sector if it is dirty.
         */

        ret = fat_fscacheflush(fs);
        if (ret < 0)
          {
              return ret;
          }

        /* Then read the specified sector into the cache */

        ret = fat_hwread(fs, fs->fs_buffer, sector, 1);
        if (ret < 0)
          {
            return ret;
          }

        /* Update the cached sector number */

        fs->fs_currentsector = sector;
    }

    return OK;
}

/****************************************************************************
 * Name: fat_ffcacheflush
 *
 * Desciption: Flush any dirty sectors as necessary
 *
 ****************************************************************************/

int fat_ffcacheflush(struct fat_mountpt_s *fs, struct fat_file_s *ff)
{
  int ret;

  /* Check if the ff_buffer is dirty.  In this case, we will write back the
   * contents of ff_buffer.
   */

  if (ff->ff_bflags && (FFBUFF_DIRTY|FFBUFF_VALID) == (FFBUFF_DIRTY|FFBUFF_VALID))
  {
      /* Write the dirty sector */

      ret = fat_hwwrite(fs, ff->ff_buffer, ff->ff_currentsector, 1);
      if (ret < 0)
      {
          return ret;
      }

      /* No longer dirty */

      ff->ff_bflags &= ~FFBUFF_DIRTY;
  }

  return OK;
}

/****************************************************************************
 * Name: fat_ffcacheread
 *
 * Desciption: Read the specified sector into the sector cache, flushing any
 *   existing dirty sectors as necessary.
 *
 ****************************************************************************/

int fat_ffcacheread(struct fat_mountpt_s *fs, struct fat_file_s *ff, size_t sector)
{
  int ret;

  /* ff->ff_currentsector holds the current sector that is buffered in
   * ff->ff_buffer. If the requested sector is the same as this sector, then
   * we do nothing. Otherwise, we will have to read the new sector.
   */

  if (ff->ff_currentsector != sector || (ff->ff_bflags & FFBUFF_VALID) == 0)
      {
        /* We will need to read the new sector.  First, flush the cached
         * sector if it is dirty.
         */

        ret = fat_ffcacheflush(fs, ff);
        if (ret < 0)
          {
              return ret;
          }

        /* Then read the specified sector into the cache */

        ret = fat_hwread(fs, ff->ff_buffer, sector, 1);
        if (ret < 0)
          {
            return ret;
          }

        /* Update the cached sector number */

        ff->ff_currentsector = sector;
        ff->ff_bflags |= FFBUFF_VALID;
    }
    return OK;
}

/****************************************************************************
 * Name: fat_ffcacheread
 *
 * Desciption: Invalidate the current file buffer contents
 *
 ****************************************************************************/

int fat_ffcacheinvalidate(struct fat_mountpt_s *fs, struct fat_file_s *ff)
{
  int ret;

  /* Is there anything valid in the buffer now? */

  if ((ff->ff_bflags & FFBUFF_VALID) != 0)
      {
        /* We will invalidate the buffered sector */

        ret = fat_ffcacheflush(fs, ff);
        if (ret < 0)
          {
              return ret;
          }

        /* Then discard the current cache contents */

        ff->ff_bflags &= ~FFBUFF_VALID;
    }
    return OK;
}

/****************************************************************************
 * Name: fat_updatefsinfo
 *
 * Desciption: Flush evertyhing buffered for the mountpoint and update
 *   the FSINFO sector, if appropriate
 *
 ****************************************************************************/

int fat_updatefsinfo(struct fat_mountpt_s *fs)
{
  int ret;

  /* Flush the fs_buffer if it is dirty */

  ret = fat_fscacheflush(fs);
  if (ret == OK)
    {
      /* The FSINFO sector only has to be update for the case of a FAT32 file
       * system.  Check if the file system type.. If this is a FAT32 file
       * system then the fs_fsidirty flag will indicate if the FSINFO sector
       * needs to be re-written.
       */

      if (fs->fs_type == FSTYPE_FAT32 && fs->fs_fsidirty)
        {
          /* Create an image of the FSINFO sector in the fs_buffer */

          memset(fs->fs_buffer, 0, fs->fs_hwsectorsize);
          FSI_PUTLEADSIG(fs->fs_buffer, 0x41615252);
          FSI_PUTSTRUCTSIG(fs->fs_buffer, 0x61417272);
          FSI_PUTFREECOUNT(fs->fs_buffer, fs->fs_fsifreecount);
          FSI_PUTNXTFREE(fs->fs_buffer, fs->fs_fsinextfree);
          FSI_PUTTRAILSIG(fs->fs_buffer, 0xaa550000);
        
          /* Then flush this to disk */

          fs->fs_currentsector = fs->fs_fsinfo;
          fs->fs_dirty         = TRUE;
          ret                  = fat_fscacheflush(fs);

          /* No longer dirty */

          fs->fs_fsidirty = FALSE;
        }
    }
  return ret;
}

#endif /* CONFIG_FS_FAT */
