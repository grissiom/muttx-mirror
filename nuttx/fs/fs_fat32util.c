/****************************************************************************
 * fs_fat32util.c
 *
 *   Copyright (C) 2007 Gregory Nutt. All rights reserved.
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
 * Name: fat_hwread
 *
 * Desciption: Read the specified sector into the sector buffer
 *
 ****************************************************************************/

static int fat_hwread(struct fat_mountpt_s *fs, ubyte *buffer,
                    size_t sector, unsigned int nsectors)
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

static int fat_hwwrite(struct fat_mountpt_s *fs, ubyte *buffer,
                     size_t sector, unsigned int nsectors)
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
 * Name: fat_cacheflush
 *
 * Desciption: Flush any dirty sectors as necessary
 *
 ****************************************************************************/

static int fat_cacheflush(struct fat_mountpt_s *fs)
{
  int ret;

  /* Check if the fs_buffer is dirty.  In this case, we will write back the
   *  contents of fs_buffer.
   */

  if (fs->fs_dirty)
  {
      /* Write the dirty sector */

      ret = fat_hwwrite(fs, fs->fs_buffer, fs->fs_sector, 1);
      if (ret < 0)
      {
          return ret;
      }

      /* Does the sector lie in the FAT region? */

      if (fs->fs_sector < fs->fs_fatbase + fs->fs_fatsize)
      {
          /* Yes, then make the change in the FAT copy as well */
          int i;

          for (i = fs->fs_fatnumfats; i >= 2; i--)
          { 
              fs->fs_sector += fs->fs_fatsize;
              ret = fat_hwwrite(fs, fs->fs_buffer, fs->fs_sector, 1);
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
 * Name: fat_cacheread
 *
 * Desciption: Read the specified sector into the sector cache, flushing any
 *   existing dirty sectors as necessary.
 *
 ****************************************************************************/

static int fat_cacheread(struct fat_mountpt_s *fs, uint32 sector)
{
  int ret;

  /* fs->sector holds the current sector that is buffered in fs->fs_buffer.
   * If the requested sector is the same as this sector, then we do nothing.
   * Otherwise, we will have to read the new sector.
   */

    if (fs->fs_sector != sector)
      {
        /* We will need to read the new sector.  First, flush the cached
         * sector if it is dirty.
         */

        ret = fat_cacheflush(fs);
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

        fs->fs_sector = sector;
    }
    return OK;
}

/****************************************************************************
 * Name: fat_clustger2sector
 *
 * Desciption: Convert a cluster number to a start sector number
 *
 ****************************************************************************/

static ssize_t fat_cluster2sector(struct fat_mountpt_s *fs,  uint32 cluster )
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
 * Desciption: Get the cluster start sector into the FAT
 *
 ****************************************************************************/

static ssize_t fat_getcluster(struct fat_mountpt_s *fs, unsigned int clusterno)
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

                if (fat_cacheread(fs, fatsector) < 0)
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

                    if (fat_cacheread(fs, fatsector) < 0)
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

                if (fat_cacheread(fs, fatsector) < 0)
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

                if (fat_cacheread(fs, fatsector) < 0)
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
    return (ssize_t)ERROR;
}

/****************************************************************************
 * Name: fat_putcluster
 *
 * Desciption: Write a new cluster start sector into the FAT
 *
 ****************************************************************************/

static int fat_putcluster(struct fat_mountpt_s *fs, unsigned int clusterno, size_t startsector)
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

                if (fat_cacheread(fs, fatsector)< 0)
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
                    if (fat_cacheread(fs, fatsector) < 0)
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

                if (fat_cacheread(fs, fatsector) < 0)
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

                if (fat_cacheread(fs, fatsector) < 0)
                  {
                    /* Read error */
                    break;
                  }
                FAT_PUTFAT32(fs->fs_buffer, fatindex, startsector & 0x0fffffff);
              }
            break;

            default:
                return ERROR;
          }

        /* Mark the modified sector as "dirty" and return success */

        fs->fs_dirty = 1;
        return OK;
      }
    return ERROR;
 }

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
 * Name: fat_nextdirentry
 *
 * Desciption: Read the next directory entry from the sector in cache,
 *   reading the next sector(s) in the cluster as necessary.
 *
 ****************************************************************************/

int fat_nextdirentry(struct fat_dirinfo_s *dirinfo)
{
  struct fat_mountpt_s *fs = dirinfo->fs;
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

int fat_finddirentry(struct fat_dirinfo_s *dirinfo, const char *path)
{
  struct fat_mountpt_s *fs = dirinfo->fs;
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

          ret = fat_cacheread(fs, dirinfo->fd_currsector);
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

          if (fat_nextdirentry(dirinfo) != OK)
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
 * Name: fat_dirtruncate
 *
 * Desciption: Truncate an existing file to zero length
 *
 ****************************************************************************/

int  fat_dirtruncate(struct fat_mountpt_s *fs, struct fat_dirinfo_s *dirinfo)
{
#warning "File truncation logic not implemented"
  return -ENOSYS;
}

/****************************************************************************
 * Name: fat_dircreate
 *
 * Desciption: Create a directory entry for a new file
 *
 ****************************************************************************/

int fat_dircreate(struct fat_mountpt_s *fs, struct fat_dirinfo_s *dirinfo)
{
#warning "File truncation logic not implemented"
  return -ENOSYS;
}

#endif /* CONFIG_FS_FAT */
