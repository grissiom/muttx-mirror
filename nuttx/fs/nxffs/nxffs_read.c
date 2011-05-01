/****************************************************************************
 * fs/nxffs/nxffs_read.c
 *
 *   Copyright (C) 2011 Gregory Nutt. All rights reserved.
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <string.h>
#include <fcntl.h>
#include <crc32.h>
#include <assert.h>
#include <errno.h>
#include <debug.h>

#include <nuttx/fs.h>
#include <nuttx/mtd.h>

#include "nxffs.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Public Types
 ****************************************************************************/

/****************************************************************************
 * Public Variables
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxffs_rdblkhdr
 *
 * Description:
 *   Read the dataa block header at this offset.  Called only from nxffs_nextblock().
 *
 * Input Parameters:
 *   volume - Describes the current volume.
 *   offset - The byte offset from the beginning of FLASH where the data block
 *     header is expected.
 *   datlen  - A memory location to return the data block length.
 *
 * Returned Value:
 *   Zero on success.  Otherwise, a negater errno value is returned
 *   indicating the nature of the failure.
 *
 ****************************************************************************/

static int nxffs_rdblkhdr(FAR struct nxffs_volume_s *volume, off_t offset,
                          FAR size_t *datlen)
{
  struct nxffs_data_s blkhdr;
  uint32_t ecrc;
  uint32_t crc;
  uint16_t doffset;
  uint16_t dlen;
  int ret;

  /* Read the header at the FLASH offset */

  nxffs_ioseek(volume, offset);
  ret = nxffs_rddata(volume, (FAR uint8_t *)&blkhdr, SIZEOF_NXFFS_DATA_HDR);
  if (ret < 0)
    {
      fdbg("Failed to read data block header, offset %d: %d\n", offset, -ret);
      return -EIO;
    }
  doffset = volume->iooffset;

  /* Extract the data length */

  dlen = nxffs_rdle16(blkhdr.datlen);
  if ((uint32_t)doffset + (uint32_t)dlen > (uint32_t)volume->geo.blocksize)
    {
      fdbg("Data length=%d is unreasonable at offset=%d\n", dlen, doffset);
      return -EIO;
    }
 
  /* Extract the expected CRC and calculate the CRC of the data block */

  ecrc = nxffs_rdle32(blkhdr.crc);

  nxffs_wrle16(blkhdr.crc, 0);
  crc = crc32((FAR const uint8_t *)&blkhdr, SIZEOF_NXFFS_DATA_HDR);
  crc = crc32part(&volume->cache[doffset], dlen, crc);
  if (crc != ecrc)
    {
      fdbg("CRC failure\n");
      return -EIO;
    }

  /* Looks good! Return the data length and success */

  *datlen = dlen;
  return OK;
}

/****************************************************************************
 * Name: nxffs_nextblock
 *
 * Description:
 *   Search for the next valid data block starting at the provided FLASH offset.
 *
 * Input Parameters:
 *   volume - Describes the NXFFS volume.
 *   offset - The FLASH memory offset to begin searching.
 *   datlen  - A memory location to return the data block length.
 *  
 * Returned Value:
 *   Zero is returned on success. Otherwise, a negated errno is returned
 *   that indicates the nature of the failure.
 *
 ****************************************************************************/

int nxffs_nextblock(FAR struct nxffs_volume_s *volume, off_t offset,
                    FAR size_t *datlen)
{
  int nmagic;
  int ch;
  int nerased;
  int ret;

  /* Seek to the first FLASH offset provided by the caller. */

  nxffs_ioseek(volume, offset);

  /* Then begin searching */
  
  nerased = 0;
  nmagic  = 0;

  for (;;)
    {
      /* Read the next character */

      ch = nxffs_getc(volume);
      if (ch < 0)
        {
          fvdbg("nxffs_getc failed: %d\n", -ch);
          return ch;
        }

      /* Check for another erased byte */

      else if (ch == CONFIG_NXFFS_ERASEDSTATE)
        {
          /* If we have encountered NXFFS_NERASED number of consecutive
           * erased bytes, then presume we have reached the end of valid
           * data.
           */

          if (++nerased >= NXFFS_NERASED)
            {
              fvdbg("No entry found\n");
              return -ENOENT;
            }
        }
      else
        {
          nerased = 0;

          /* Check for the magic sequence indicating the start of an NXFFS
           * data block or start of the next inode. There is the possibility
           * of this magic sequnce occurring in FLASH data.  However, the
           * data block CRC should distinguish between real NXFFS data blocks
           * headers and such false alarms.
           */

          if (ch != g_datamagic[nmagic])
            {
              nmagic = 0;
            }
          else if (nmagic < NXFFS_MAGICSIZE - 1)
            {
              nmagic++;
            }

          /* We have found the magic sequence in the FLASH data that may
           * indicate the beginning of an NXFFS data block.
           */

          else 
            {
              ret = nxffs_rdblkhdr(volume, offset, datlen);
              if (ret == OK)
                {
                  fdbg("Found a valid fileheader\n");
                  return OK;
                }

              /* False alarm.. keep looking */

              nmagic = 0;
            }
        }
    }

  /* We won't get here, but to keep some compilers happy: */

  return -ENOENT;
}

/****************************************************************************
 * Name: nxffs_rdseek
 *
 * Description:
 *   Seek to the file position before read or write access.  Note that the
 *   simplier nxffs_ioseek() cannot be used for this purpose.  File offsets
 *   are not easily mapped to FLASH offsets due to intervening block and
 *   data headers.
 *
 * Input Parameters:
 *   volume - Describes the current volume
 *   entry  - Describes the open inode
 *   fpos   - The desired file position
 *
 ****************************************************************************/

static ssize_t nxffs_rdseek(FAR struct nxffs_volume_s *volume,
                            FAR struct nxffs_entry_s *entry,
                            off_t fpos)
{
  size_t datlen = 0;
  size_t blklen = 0;
  off_t offset = fpos;
  int ret;

  /* Loop until we read the data block containing the desired position */

  do
    {
      /* Get the data offset at the start of the next data block */

      datlen += blklen;

      /* Get the length of the next data block */

      ret = nxffs_nextblock(volume, offset, &blklen);
      if (ret < 0)
        {
          fdbg("nxffs_nextblock failed: %d\n", -ret);
          return ret;
        }
    }
  while (datlen <= fpos && datlen + blklen > fpos);

  volume->iooffset += (fpos - datlen);
  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/
/****************************************************************************
 * Name: nxffs_read
 *
 * Description:
 *   This is an implementation of the NuttX standard file system read
 *   method.
 *
 ****************************************************************************/

ssize_t nxffs_read(FAR struct file *filep, FAR char *buffer, size_t buflen)
{
  FAR struct nxffs_volume_s *volume;
  FAR struct nxffs_ofile_s *ofile;
  ssize_t ret;

  fvdbg("Read %d bytes from offset %d\n", buflen, filep->f_pos);

  /* Sanity checks */

  DEBUGASSERT(filep->f_priv != NULL && filep->f_inode != NULL);

  /* Recover the open file state from the struct file instance */

  ofile = (FAR struct nxffs_ofile_s *)filep->f_priv;

  /* Recover the volume state from the open file */

  volume = (FAR struct nxffs_volume_s *)filep->f_inode->i_private;
  DEBUGASSERT(volume != NULL);

  /* Get exclusive access to the volume.  Note that the volume exclsem
   * protects the open file list.
   */

  ret = sem_wait(&volume->exclsem);
  if (ret != OK)
    {
      ret = -errno;
      fdbg("sem_wait failed: %d\n", ret);
      goto errout;
    }

  /* Check if the file was opened with read access */

  if ((ofile->mode & O_RDOK) == 0)
    {
      fdbg("File not open for read access\n");
      ret = -EACCES;
      goto errout_with_semaphore;
    }

  /* Seek to the current file offset */

  ret = nxffs_rdseek(volume, &ofile->entry, filep->f_pos);
  if (ret < 0)
    {
      fdbg("nxffs_fwseek failed: %d\n", -ret);
      ret = -EACCES;
      goto errout_with_semaphore;
    }

  /* Read data from that file offset */

  ret = nxffs_rddata(volume, (FAR uint8_t *)buffer, buflen);
  if (ret > 0)
    {
      /* Update the file offset */

      filep->f_pos += ret;
    }

errout_with_semaphore:
  sem_post(&volume->exclsem);
errout:
  return ret;
}

