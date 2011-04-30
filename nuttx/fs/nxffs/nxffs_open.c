/****************************************************************************
 * fs/nxffs/nxffs_open.c
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
#include <time.h>
#include <crc32.h>
#include <assert.h>
#include <errno.h>
#include <debug.h>

#include <nuttx/kmalloc.h>
#include <nuttx/fs.h>
#include <nuttx/mtd.h>

#include "nxffs.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* Since we are limited to a single file opened for writing, it makes sense
 * to pre-allocate the write state structure.
 */

#ifdef CONFIG_NXFSS_PREALLOCATED
static struct nxffs_wrfile_s g_wrfile;
#endif

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxffs_hdrpos
 *
 * Description:
 *   Find a valid location for the inode header.  A valid location will have
 *   these properties:
 *
 *   1. It will lie in the free flash region.
 *   2. It will have enough contiguous memory to hold the entire header
 *      (excluding the file name which may lie in the next block).
 *   3. The memory at this location will be fully erased.
 *
 *   This function will only perform the checks of 1) and 2).
 *
 * Input Parameters:
 *   volume - Describes the NXFFS volume
 *   wrfile - Contains the current guess for the header position.  On
 *     successful return, this field will hold the selected header
 *     position.
 *
 * Returned Value:
 *   Zero is returned on success.  Otherwise, a negated errno value is
 *   returned indicating the nature of the failure.  Of special interest
 *   the return error of -ENOSPC which means that the FLASH volume is
 *   full and should be repacked.
 *
 *   On successful return the following are also valid:
 *
 *     wrfile->ofile.entry.hoffset - Flash offset to candidate header position
 *     volume->ioblock - Read/write block number of the block containing the
 *       header position
 *     volume->iooffset - The offset in the block to the candidate header
 *       position.
 *     volume->froffset - Updated offset to the first free FLASH block.
 *
 ****************************************************************************/

static inline int nxffs_hdrpos(FAR struct nxffs_volume_s *volume,
                               FAR struct nxffs_wrfile_s *wrfile)
{
  int ret;

  /* Reserve memory for the object */

  ret = nxffs_wrreserve(volume, SIZEOF_NXFFS_INODE_HDR);
  if (ret == OK)
    {
      /* Save the offset to the FLASH region reserved for the inode header */

      wrfile->ofile.entry.hoffset = nxffs_iotell(volume);
    }
  return OK;
}

/****************************************************************************
 * Name: nxffs_nampos
 *
 * Description:
 *   Find a valid location for the inode name.  A valid location will have
 *   these properties:
 *
 *   1. It will lie in the free flash region.
 *   2. It will have enough contiguous memory to hold the entire name
 *   3. The memory at this location will be fully erased.
 *
 *   This function will only perform the checks of 1) and 2).
 *
 * Input Parameters:
 *   volume - Describes the NXFFS volume
 *   wrfile - Contains the current guess for the name position.  On
 *     successful return, this field will hold the selected name
 *     position.
 *   namlen - The length of the name.
 *
 * Returned Value:
 *   Zero is returned on success.  Otherwise, a negated errno value is
 *   returned indicating the nature of the failure.  Of special interest
 *   the return error of -ENOSPC which means that the FLASH volume is
 *   full and should be repacked.
 *
 *   On successful return the following are also valid:
 *
 *     wrfile->ofile.entry.noffset - Flash offset to candidate name position
 *     volume->ioblock - Read/write block number of the block containing the
 *       name position
 *     volume->iooffset - The offset in the block to the candidate name
 *       position.
 *     volume->froffset - Updated offset to the first free FLASH block.
 *
 ****************************************************************************/

static inline int nxffs_nampos(FAR struct nxffs_volume_s *volume,
                               FAR struct nxffs_wrfile_s *wrfile,
                               int namlen)
{
  int ret;

  /* Reserve memory for the object */

  ret = nxffs_wrreserve(volume, namlen);
  if (ret == OK)
    {
      /* Save the offset to the FLASH region reserved for the inode name */

      wrfile->ofile.entry.noffset = nxffs_iotell(volume);
    }
  return OK;
}

/****************************************************************************
 * Name: nxffs_hdrerased
 *
 * Description:
 *   Find a valid location for the inode header.  A valid location will have
 *   these properties:
 *
 *   1. It will lie in the free flash region.
 *   2. It will have enough contiguous memory to hold the entire header
 *      (excluding the file name which may lie in the next block).
 *   3. The memory at this location will be fully erased.
 *
 *   This function will only perform the check 3).
 *
 *   On entry it assumes:
 *
 *     volume->ioblock  - Read/write block number of the block containing the
 *       header position
 *     volume->iooffset - The offset in the block to the candidate header
 *       position.
 *
 * Input Parameters:
 *   volume - Describes the NXFFS volume
 *   wrfile - Contains the current guess for the header position.  On
 *     successful return, this field will hold the selected header
 *     position.
 *
 * Returned Value:
 *   Zero is returned on success.  Otherwise, a negated errno value is
 *   returned indicating the nature of the failure.  Of special interest
 *   the return error of -ENOSPC which means that the FLASH volume is
 *   full and should be repacked.
 *
 *   On successful return the following are also valid:
 *
 *     wrfile->ofile.entry.hoffset - Flash offset to candidate header position
 *     volume->ioblock - Read/write block number of the block containing the
 *       header position
 *     volume->iooffset - The offset in the block to the candidate header
 *       position.
 *     volume->froffset - Updated offset to the first free FLASH block.
 *
 ****************************************************************************/

static inline int nxffs_hdrerased(FAR struct nxffs_volume_s *volume,
                                  FAR struct nxffs_wrfile_s *wrfile)
{
  int ret;

  /* Find a valid location to save the inode header */
  
  ret = nxffs_wrverify(volume, SIZEOF_NXFFS_INODE_HDR);
  if (ret == OK)
    {
      /* This is where we will put the header */

      wrfile->ofile.entry.hoffset = nxffs_iotell(volume);
    }
  return ret;
}

/****************************************************************************
 * Name: nxffs_namerased
 *
 * Description:
 *   Find a valid location for the inode name.  A valid location will have
 *   these properties:
 *
 *   1. It will lie in the free flash region.
 *   2. It will have enough contiguous memory to hold the entire name
 *      (excluding the file name which may lie in the next block).
 *   3. The memory at this location will be fully erased.
 *
 *   This function will only perform the check 3).
 *
 *   On entry it assumes:
 *
 *     volume->ioblock  - Read/write block number of the block containing the
 *       name position
 *     volume->iooffset - The offset in the block to the candidate name
 *       position.
 *
 * Input Parameters:
 *   volume - Describes the NXFFS volume
 *   wrfile - Contains the current guess for the name position.  On
 *     successful return, this field will hold the selected name
 *     position.
 *
 * Returned Value:
 *   Zero is returned on success.  Otherwise, a negated errno value is
 *   returned indicating the nature of the failure.  Of special interest
 *   the return error of -ENOSPC which means that the FLASH volume is
 *   full and should be repacked.
 *
 *   On successful return the following are also valid:
 *
 *     wrfile->ofile.entry.noffset - Flash offset to candidate name position
 *     volume->ioblock - Read/write block number of the block containing the
 *       name position
 *     volume->iooffset - The offset in the block to the candidate name
 *       position.
 *     volume->froffset - Updated offset to the first free FLASH block.
 *
 ****************************************************************************/

static inline int nxffs_namerased(FAR struct nxffs_volume_s *volume,
                                  FAR struct nxffs_wrfile_s *wrfile,
                                  int namlen)
{
  int ret;

  /* Find a valid location to save the inode name */
  
  ret = nxffs_wrverify(volume, namlen);
  if (ret == OK)
    {
      /* This is where we will put the name */

      wrfile->ofile.entry.hoffset = nxffs_iotell(volume);
    }
  return ret;
}

/****************************************************************************
 * Name: nxffs_wropen
 *
 * Description:
 *   Handle opening for writing.  Only a single writer is permitted and only
 *   file creation is supported.
 *
 ****************************************************************************/

static inline int nxffs_wropen(FAR struct nxffs_volume_s *volume,
                               FAR const char *name, mode_t mode,
                               FAR struct nxffs_ofile_s **ppofile)
{
  FAR struct nxffs_wrfile_s *wrfile;
  FAR struct nxffs_entry_s entry;
  bool packed;
  bool truncate = false;
  int namlen;
  int ret;

  /* Limitation: Only a single writer is permitted.  Writing may involve
   * extension of the file system in FLASH.  Since files are contiguous
   * in FLASH, only a single file may be extending the FLASH region.
   */

  if (volume->wrbusy)
    {
      fdbg("There is already a file writer\n");
      ret = -ENOSYS;
      goto errout;
    }

  /* Check if the file exists */

  ret = nxffs_findinode(volume, name, &entry);
  if (ret == OK)
    {
      /* It exists.  It would be an error if we are asked to create it
       * exclusively.
       */

      if ((mode & (O_CREAT|O_EXCL)) == (O_CREAT|O_EXCL))
        {
          fdbg("File exists, can't create O_EXCL\n");
          ret = -EEXIST;
          goto errout;
        }

      /* Were we asked to truncate the file?  NOTE: Don't truncate the
       * file if we were not also asked to created it.  See below...
       * we will not re-create the file unless O_CREAT is also specified.
       */

      else if ((mode & (O_CREAT|O_TRUNC)) == (O_CREAT|O_TRUNC))
        {
          /* Just schedule the removal the file and fall through to re-create it.
           * Note that the old file of the same name will not actually be removed
           * until the new file is successfully written.
           */

          truncate = true;
        }

      /* The file exists and we were not asked to truncate (and recreate) it.
       * Limitation: Cannot write to existing files.
       */

      else
        {
          fdbg("File %s exists and we were not asked to truncate it\n");
          ret = -ENOSYS;
          goto errout;
        }
    }

  /* Okay, the file is not open and does not exists (maybe because we deleted
   * it).  Now, make sure that we were asked to created it.
   */

  if ((mode & O_CREAT) == 0)
    {
      fdbg("Not asked to create the file\n");
      ret = -ENOENT;
      goto errout;
    }

  /* Make sure that the length of the file name will fit in a uint8_t */

  namlen = strlen(name);
  if (namlen > UINT8_MAX)
    {
      fdbg("Name is too long: %d\n", namlen);
      ret = -EINVAL;
      goto errout;
    }

  /* Yes.. Create a new structure that will describe the state of this open
   * file.  NOTE that a special variant of the open file structure is used
   * that includes additional information to support the write operation.
   */

#ifdef CONFIG_NXFSS_PREALLOCATED
  wrfile = &g_wrfile;
  memset(wrfile, 0, sizeof(struct nxffs_wrfile_s));
#else
  wrfile = (FAR struct nxffs_wrfile_s *)kzalloc(sizeof(struct nxffs_wrfile_s));
  if (!wrfile)
    {
      ret = -ENOMEM;
      goto errout;
    }
#endif

  /* Initialize the open file state structure */

  wrfile->ofile.crefs     = 1;
  wrfile->ofile.mode      = O_WROK;
  wrfile->ofile.entry.utc = time(NULL);
  wrfile->truncate        = truncate;

  /* Allocate FLASH memory for the file and set up for the write.
   *
   * Loop until the inode header is configured or until a failure occurs.
   * Note that nothing is written to FLASH.  The inode header is not
   * written until the file is closed.
   */

  packed = false;
  for (;;)
    {
      /* File a valid location to position the inode header.  Start with the
       * first byte in the free FLASH region.
       */

      ret = nxffs_hdrpos(volume, wrfile);
      if (ret == OK)
        {
          /* Find a region of memory in the block that is fully erased */

          ret = nxffs_hdrerased(volume, wrfile);
          if (ret == OK)
            {
              /* Valid memory for the inode header was found.  Break out of
               * the loop.
               */

              break;
            }
        }

      /* If no valid memory is found searching to the end of the volume,
       * then -ENOSPC will be returned.  Other errors are not handled.
       */

      if (ret != -ENOSPC || packed)
        {
          fdbg("Failed to find inode header memory: %d\n", -ret);
          goto errout_with_ofile;
        }

      /* -ENOSPC is a special case..  It means that the volume is full.
       * Try to pack the volume in order to free up some space.
       */

      ret = nxffs_pack(volume);
      if (ret < 0)
        {
          fdbg("Failed to pack the volume: %d\n", -ret);
          goto errout_with_ofile;
        }
              
      /* After packing the volume, froffset will be updated to point to the
       * new free flash region.  Try again.
       */
               
      packed = true;
    }

  /* Loop until the inode name is configured or until a failure occurs.
   * Note that nothing is written to FLASH.  The inode name is not
   * written until the file is closed.
   */

  for (;;)
    {
      /* File a valid location to position the inode name.  Start with the
       * first byte in the free FLASH region.
       */

      ret = nxffs_nampos(volume, wrfile, namlen);
      if (ret == OK)
        {
          /* Find a region of memory in the block that is fully erased */

          ret = nxffs_namerased(volume, wrfile, namlen);
          if (ret == OK)
            {
              /* Valid memory for the inode header was found.  Break out of
               * the loop.
               */

              break;
            }
        }

      /* If no valid memory is found searching to the end of the volume,
       * then -ENOSPC will be returned.  Other errors are not handled.
       */

      if (ret != -ENOSPC || packed)
        {
          fdbg("Failed to find inode name memory: %d\n", -ret);
          goto errout_with_ofile;
        }

      /* -ENOSPC is a special case..  It means that the volume is full.
       * Try to pack the volume in order to free up some space.
       */

      ret = nxffs_pack(volume);
      if (ret < 0)
        {
          fdbg("Failed to pack the volume: %d\n", -ret);
          goto errout_with_ofile;
        }
              
      /* After packing the volume, froffset will be updated to point to the
       * new free flash region.  Try again.
       */
               
      packed = true;
    }

  /* Add the open file structure to the head of the list of open files */

  wrfile->ofile.flink = volume->ofiles;
  volume->ofiles      = &wrfile->ofile;

  /* Indicate that the volume is open for writing and return the open file
   * instance.
   */

  volume->wrbusy = 1;
  *ppofile = &wrfile->ofile;
  return OK;

errout_with_ofile:
#ifndef CONFIG_NXFSS_PREALLOCATED
  kfree(wrfile);
#endif
errout:
  return ret;
}

/****************************************************************************
 * Name: nxffs_rdopen
 *
 * Description:
 *   Open an existing file for reading.
 *
 ****************************************************************************/

static inline int nxffs_rdopen(FAR struct nxffs_volume_s *volume,
                               FAR const char *name,
                               FAR struct nxffs_ofile_s **ppofile)
{
  FAR struct nxffs_ofile_s *ofile;
  int ret;

  /* Check if the file has already been opened (for reading) */

  ofile = nxffs_findofile(volume, name);
  if (ofile)
    {
      /* The file is already open.
       * Limitation:  Files cannot be open both for reading and writing.
       */

      if ((ofile->mode & O_WROK) != 0)
        {
          fdbg("File is open for writing\n");
          return -ENOSYS;
        }

      /* Just increment the reference count on the ofile */

      ofile->crefs++;
      fdbg("crefs: %d\n", ofile->crefs);
    }

  /* The file has not yet been opened.
   * Limitation: The file must exist.  We do not support creation of files
   * read-only.
   */

  else
    {
      /* Not already open.. create a new open structure */
 
      ofile = (FAR struct nxffs_ofile_s *)kzalloc(sizeof(struct nxffs_ofile_s));
      if (!ofile)
        {
          fdbg("ofile allocation failed\n");
          return -ENOMEM;
        }

      /* Initialize the open file state structure */

      ofile->crefs = 1;
      ofile->mode  = O_RDOK;

      /* Find the file on this volume associated with this file name */

      ret = nxffs_findinode(volume, name, &ofile->entry);
      if (ret != OK)
        {
          fdbg("Inode '%s' not found: %d\n", name, -ret);
          kfree(ofile);
          return ret;
        }

      /* Add the open file structure to the head of the list of open files */

      ofile->flink   = volume->ofiles;
      volume->ofiles = ofile;
    }

  /* Return the open file state structure */

  *ppofile = ofile;
  return OK;
}

/****************************************************************************
 * Name: nxffs_freeofile
 *
 * Description:
 *   Free resources held by an open file.
 *
 ****************************************************************************/

static inline void nxffs_freeofile(FAR struct nxffs_volume_s *volume,
                                   FAR struct nxffs_ofile_s *ofile)
{
  FAR struct nxffs_ofile_s *prev;
  FAR struct nxffs_ofile_s *curr;

  /* Find the open file structure to be removed */

  for (prev = NULL, curr = volume->ofiles;
       curr && curr != ofile;
       prev = curr, curr = curr->flink);

  /* Was it found? */

  if (curr)
    {
      /* Yes.. at the head of the list? */

      if (prev)
        {
          prev->flink = ofile->flink;
        }
      else
        {
          volume->ofiles = ofile->flink;
        }

      /* Release the open file entry */

      nxffs_freeentry(&ofile->entry);
 
      /* Then free the open file container (unless this the pre-alloated
       * write-only open file container)
       */

#ifdef CONFIG_NXFSS_PREALLOCATED
      if ((FAR struct nxffs_wrfile_s*)ofile != &g_wrfile)
#endif
        {
          kfree(ofile);
        }
    }
  else
    {
      fdbg("ERROR: Open inode %p not found\n", ofile);
    }
}

/****************************************************************************
 * Name: nxffs_wrclose
 *
 * Description:
 *   Perform special operations when a file is closed:
 *   1. Write the file block header
 *   2. Remove any file with the same name that was discovered when the
 *      file was open for writing, and finally,
 *   3. Write the new file inode.
 *
 * Input parameters
 *   volume - Describes the NXFFS volume
 *   wrfile - Describes the state of the open file
 *
 ****************************************************************************/

static int nxffs_wrclose(FAR struct nxffs_volume_s *volume,
                         FAR struct nxffs_wrfile_s *wrfile)
{
  FAR struct nxffs_inode_s *inode;
  off_t namblock;
  uint16_t namoffset;
  uint32_t crc;
  int namlen;
  int ret;

  /* Write the final file block header */

  ret = nxffs_wrblkhdr(volume, wrfile);
  if (ret < 0)
    {
      fdbg("Failed to write the final block of the file: %d\n", -ret);
      goto errout;
    }

  if (wrfile->truncate)
    {
      fvdbg("Removing old file: %s\n", wrfile->ofile.entry.name);

      ret = nxffs_rminode(volume, wrfile->ofile.entry.name);
      if (ret < 0)
        {
          fdbg("nxffs_rminode failed: %d\n", -ret);
          goto errout;
        }
    }

  /* Write the inode header to FLASH.  First get the block where we will
   * write the file name.
   */

  nxffs_ioseek(volume, wrfile->ofile.entry.noffset);
  namblock  = volume->ioblock;
  namoffset = volume->iooffset;

  /* Now seek to the inode header position and assure that it is in the
   * volume cache.
   */

  nxffs_ioseek(volume, wrfile->ofile.entry.hoffset);
  ret = nxffs_rdcache(volume, volume->ioblock, 1);
  if (ret < 0)
    {
      fdbg("Failed to read inode header block %d: %d\n", volume->ioblock, -ret);
      goto errout;
    }

  /* Get the length of the inode name */

  namlen = strlen(wrfile->ofile.entry.name);
  DEBUGASSERT(namlen < UINT8_MAX); /* This was verified earlier */

  /* Initialize the inode header */

  inode = (FAR struct nxffs_inode_s *)&volume->cache[volume->iooffset];
  memcmp(inode->magic, g_inodemagic, NXFFS_MAGICSIZE);

  inode->state  = CONFIG_NXFFS_ERASEDSTATE;
  inode->namlen = namlen;

  nxffs_wrle32(inode->noffs, wrfile->ofile.entry.noffset);
  nxffs_wrle32(inode->doffs, wrfile->ofile.entry.doffset);
  nxffs_wrle32(inode->utc,   wrfile->ofile.entry.utc);
  nxffs_wrle32(inode->crc,   0);
  nxffs_wrle32(inode->noffs, wrfile->ofile.entry.datlen);

  /* Calculate the CRC */

  crc = crc32((FAR const uint8_t *)inode, SIZEOF_NXFFS_INODE_HDR);
  crc = crc32part((FAR const uint8_t *)wrfile->ofile.entry.name, namlen, crc);

  /* Finish the inode header */

  inode->state  = INODE_STATE_FILE;
  nxffs_wrle32(inode->crc, crc);

  /* Are the inode header and the inode name in the same block?  Normally,
   * they will be in the same block.  However, they could potentially be
   * far apart due to intervening bad blocks.
   */

  if (volume->ioblock != namblock)
    {
      /* Write the block with the inode header */

      ret = nxffs_wrcache(volume, volume->ioblock, 1);
      if (ret < 0)
        {
          fdbg("Failed to write inode header block %d: %d\n", volume->ioblock, -ret);
          goto errout;
        }

      /* Make sure the that block containing the inode name is in the cache */

      volume->ioblock  = namblock;
      volume->iooffset = namoffset;
      ret = nxffs_rdcache(volume, volume->ioblock, 1);
      if (ret < 0)
        {
          fdbg("Failed to read inode name block %d: %d\n", volume->ioblock, -ret);
          goto errout;
        }
    }

  /* Finally, copy the inode name to the cache and write the inode name block */

  memcpy(&volume->cache[namoffset], wrfile->ofile.entry.name, namlen);
  ret = nxffs_wrcache(volume, volume->ioblock, 1);
  if (ret < 0)
    {
      fdbg("Failed to write inode header block %d: %d\n", volume->ioblock, -ret);
    }
  
  /* The volume is now available for other writers */

errout:
  volume->wrbusy = 0;
  return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxffs_findofile
 *
 * Description:
 *   Search the list of already opened files to see if the inode of this
 *   name is one of the opened files.
 *
 * Input Parameters:
 *   name - The name of the inode to check.
 *
 * Returned Value:
 *   If an inode of this name is found in the list of opened inodes, then
 *   a reference to the open file structure is returned.  NULL is returned
 *   otherwise.
 *
 ****************************************************************************/

FAR struct nxffs_ofile_s *nxffs_findofile(FAR struct nxffs_volume_s *volume,
                                          FAR const char *name)
{
  FAR struct nxffs_ofile_s *ofile;

  /* Check every open file.  Note that the volume exclsem protects the
   * list of open files.
   */

  for (ofile = volume->ofiles; ofile; ofile = ofile->flink)
    {
      /* Check for a name match */

      if (strcmp(name, ofile->entry.name) == 0)
        {
          return ofile;
        }
    }

  return NULL;
}

/****************************************************************************
 * Name: nxffs_open
 *
 * Description:
 *   This is the standard mountpoint open method.
 *
 ****************************************************************************/

int nxffs_open(FAR struct file *filep, FAR const char *relpath,
               int oflags, mode_t mode)
{
  FAR struct nxffs_volume_s *volume;
  FAR struct nxffs_ofile_s *ofile = NULL;
  int ret;

  fvdbg("Open '%s'\n", relpath);

  /* Sanity checks */

  DEBUGASSERT(filep->f_priv == NULL && filep->f_inode != NULL);

  /* Get the mountpoint private data from the NuttX inode reference in the
   * file structure
   */

  volume = (FAR struct nxffs_volume_s*)filep->f_inode->i_private;
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

#ifdef CONFIG_FILE_MODE
#  warning "Missing check for privileges based on inode->i_mode"
#endif

  /* Limitation:  A file must be opened for reading or writing, but not both.
   * There is no general for extending the size of of a file.  Extending the
   * file size of possible if the file to be extended is the last in the
   * sequence on FLASH, but since that case is not the general case, no file
   * extension is supported.
   */

   switch (mode & (O_WROK|O_RDOK))
     {
       case 0:
       default:
         fdbg("One of O_WRONLY/O_RDONLY must be provided\n");
         ret = -EINVAL;
         goto errout_with_semaphore;

       case O_WROK:
         ret = nxffs_wropen(volume, relpath, mode, &ofile);
         break;

       case O_RDOK:
         ret = nxffs_rdopen(volume, relpath, &ofile);
         break;

       case O_WROK|O_RDOK:
         fdbg("O_RDWR is not supported\n");
         ret = -ENOSYS;
         goto errout_with_semaphore;
     }

  /* Save open-specific state in filep->f_priv */

  filep->f_priv = ofile;
  ret = OK;

errout_with_semaphore:
  sem_post(&volume->exclsem);
errout:
  return ret;
}

/****************************************************************************
 * Name: nxffs_close
 *
 * Description:
 *   This is the standard mountpoint close method.
 *
 ****************************************************************************/

int nxffs_close(FAR struct file *filep)
{
  FAR struct nxffs_volume_s *volume;
  FAR struct nxffs_ofile_s *ofile;
  int ret = -ENOSYS;

  fvdbg("Closing\n");

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

  /* Decrement the reference count on the open file */

  ret = OK;
  if (ofile->crefs == 1)
    {
      /* Decrementing the reference count would take it zero.  Handle
       * finalization of the write operation.
       */

      if (ofile->mode == O_WROK)
        {
          ret = nxffs_wrclose(volume, (FAR struct nxffs_wrfile_s *)ofile);
        }

      /* Delete the open file state structure */

      nxffs_freeofile(volume, ofile);
    }
  else
    {
      /* Just decrement the reference count */

      ofile->crefs--;
    }

  filep->f_priv = NULL;

  sem_post(&volume->exclsem);
errout:
  return ret;
}
