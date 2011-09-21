/****************************************************************************
 * apps/graphics/tiff/tiff_finalize.c
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

#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <debug.h>

#include <apps/tiff.h>

#include "tiff_internal.h"

/****************************************************************************
 * Pre-Processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: tiff_readifdentry
 *
 * Description:
 *   Read the IFD entry at the specified offset.
 *
 * Input Parameters:
 *   fd       - File descriptor to rad from
 *   offset   - Offset to read from
 *   ifdentry - Location to read the data
 *
 * Returned Value:
 *   Zero (OK) on success.  A negated errno value on failure.
 *
 ****************************************************************************/

static int tiff_readifdentry(int fd, off_t offset,
                             FAR struct tiff_ifdentry_s *ifdentry)
{
  off_t newoffs;

  /* Seek to the read position */

  newoffs = lseek(fd, SEEK_SET, offset);
  if (newoffs == (off_t)-1)
    {
      return -errno;
    }
 
    /* Then read the IFD entry */

  return tiff_read(fd, ifdentry, SIZEOF_IFD_ENTRY);
}

/****************************************************************************
 * Name: tiff_writeifdentry
 *
 * Description:
 *   Write the IFD entry at the specified offset.
 *
 * Input Parameters:
 *   fd       - File descriptor to rad from
 *   offset   - Offset to read from
 *   ifdentry - Location to read the data
 *
 * Returned Value:
 *   Zero (OK) on success.  A negated errno value on failure.
 *
 ****************************************************************************/

static int tiff_writeifdentry(int fd, off_t offset,
                              FAR struct tiff_ifdentry_s *ifdentry)
{
  off_t newoffs;

  /* Seek to the write position */

  newoffs = lseek(fd, SEEK_SET, offset);
  if (newoffs == (off_t)-1)
    {
      return -errno;
    }

  /* Then write the IFD entry */

   return tiff_write(fd, ifdentry, SIZEOF_IFD_ENTRY);
}

/****************************************************************************
 * Name: tiff_cleanup
 *
 * Description:
 *   Normal clean-up after completion of the TIFF file creation
 *
 * Input Parameters:
 *   info - A pointer to the caller allocated parameter passing/TIFF
 *          state instance.
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void tiff_cleanup(FAR struct tiff_info_s *info)
{
  /* Close all opened files */

  if (info->outfd >= 0)
    {
      (void)close(info->outfd);
    }
  info->outfd = -1;

  if (info->tmp1fd >= 0)
    {
      (void)close(info->tmp1fd);
    }
  info->tmp1fd = -1;

  if (info->tmp2fd >= 0)
    {
      (void)close(info->tmp2fd);
    }
  info->tmp2fd = -1;

  /* And remove the temporary files */

  (void)unlink(info->tmpfile1); 
  (void)unlink(info->tmpfile2); 
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: tiff_finalize
 *
 * Description:
 *   Finalize the TIFF output file, completing the TIFF file creation steps.
 *
 * Input Parameters:
 *   info - A pointer to the caller allocated parameter passing/TIFF state
 *          instance.
 *
 * Returned Value:
 *   Zero (OK) on success.  A negated errno value on failure.
 *
 ****************************************************************************/

int tiff_finalize(FAR struct tiff_info_s *info)
{
  struct tiff_ifdentry_s ifdentry;
  FAR uint8_t *ptr;
  size_t maxoffsets;
#ifdef CONFIG_DEBUG_GRAPHICS
  size_t total;
#endif
  off_t offset;
  int ret;
  int i;
  int j;

  DEBUGASSERT(info && info->outfd >= 0 && info->tmp1fd >= 0 && info->tmp2fd >= 0);
  DEBUGASSERT((info->outsize & 3) == 0 && (info->tmp1size & 3) == 0);

  /* Fix-up the count value in the StripByteCounts IFD entry in the outfile */

  ret = tiff_readifdentry(info->outfd, info->filefmt->sbcifdoffset, &ifdentry);
  if (ret < 0)
    {
      goto errout;
    }

  tiff_put32(ifdentry.count, info->nstrips);

  ret = tiff_writeifdentry(info->outfd, info->filefmt->sbcifdoffset, &ifdentry);
  if (ret < 0)
    {
      goto errout;
    }

  /* Fix-up the count and offset values in the StripOffsets IFD entry in the outfile */

  ret = tiff_readifdentry(info->outfd, info->filefmt->soifdoffset, &ifdentry);
  if (ret < 0)
    {
      goto errout;
    }

  tiff_put32(ifdentry.count, info->nstrips);
  tiff_put32(ifdentry.offset, info->outsize + info->tmp1size);

  ret = tiff_writeifdentry(info->outfd, info->filefmt->soifdoffset, &ifdentry);
  if (ret < 0)
    {
      goto errout;
    }

  /* Seek to the beginning of tmpfile2 */

  offset = lseek(info->tmp2fd, SEEK_SET, 0);
  if (offset == (off_t)-1)
    {
      ret = -errno;
      goto errout;
    }

  /* Seek to the end of the outfile */

  ret = lseek(info->outfd, SEEK_END, 0);
  if (offset == (off_t)-1)
    {
      ret = -errno;
      goto errout;
    }

  /* Now read strip offset data from tmpfile2, update the offsets, and write
   * the updated offsets to the outfile.
   */

  maxoffsets = info->iosize >> 2;
#ifdef CONFIG_DEBUG_GRAPHICS
  total      = 0;
#endif

  for (i = 0; i < info->nstrips; )
    {
      size_t noffsets;

      /* Read a group of up to 32-bit values */

      noffsets = info->nstrips - i;
      if (noffsets > maxoffsets)
        {
          noffsets = maxoffsets;
        }

      ret = tiff_read(info->tmp1fd, info->iobuffer, noffsets << 2);
      if (ret <= 0)
        {
          goto errout;
        }

      /* Fix up the offsets */

      for (j = 0, ptr = info->iobuffer;
           j < noffsets;
           j++, ptr += 4)
        {
          uint32_t stripoff = tiff_get32(ptr);
          stripoff += info->outsize;
          tiff_put32(ptr, stripoff);
        }

      /* Then write the corrected offsets to the outfile */

      ret = tiff_write(info->outfd, info->iobuffer, noffsets << 2);
      if (ret < 0)
        {
          goto errout;
        }

      /* Update the count of offsets written */

      i     += noffsets;
#ifdef CONFIG_DEBUG_GRAPHICS
      total += noffsets << 2;
#endif

    }
#ifdef CONFIG_DEBUG_GRAPHICS
  ASSERT(total == info->tmp1size);
#endif

  /* Finally, copy the tmpfile2 to the end of the outfile */

#ifdef CONFIG_DEBUG_GRAPHICS
  total = 0;
#endif
  for (;;)
    {
      ssize_t nbytes;

      /* Read a block of data from tmpfile2 */

      nbytes = tiff_read(info->tmp2fd, info->iobuffer, info->iosize);
      if (nbytes < 0)
        {
          ret = (int)nbytes;
          goto errout;
        }
      else if (ret == 0)
        {
          break;
        }

      /* Then copy the data to the outfile */

      ret = tiff_write(info->outfd, info->iobuffer, nbytes);
      if (ret < 0)
        {
          goto errout;
        }

#ifdef CONFIG_DEBUG_GRAPHICS
      total += nbytes;
#endif
    }
#ifdef CONFIG_DEBUG_GRAPHICS
  ASSERT(total == info->tmp2size);
#endif

  /* Close all files and return success */

  tiff_cleanup(info);
  return OK;

errout:
  tiff_abort(info);
  return ret;
}

/************************************************************************************
 * Name: tiff_abort
 *
 * Description:
 *   Abort the TIFF file creation and create-up resources.
 *
 * Input Parameters:
 *   info - A pointer to the caller allocated parameter passing/TIFF state instance.
 *
 * Returned Value:
 *   None
 *
 ************************************************************************************/

void tiff_abort(FAR struct tiff_info_s *info)
{
  /* Perform normal cleanup */

  tiff_cleanup(info);

  /* But then delete the output file as well */

  (void)unlink(info->outfile); 
}

