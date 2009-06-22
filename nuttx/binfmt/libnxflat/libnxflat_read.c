/****************************************************************************
 * binfmt/libnxflat/libnxflat_read.c
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

#include <unistd.h>
#include <string.h>
#include <nxflat.h>
#include <debug.h>
#include <errno.h>

#include <arpa/inet.h>
#include <nuttx/nxflat.h>

/****************************************************************************
 * Pre-Processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Constant Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxflat_read
 *
 * Description:
 *   Read 'readsize' bytes from the object file at 'offset'
 *
 * Returned Value:
 *   0 (OK) is returned on success and a negated errno is returned on
 *   failure.
 *
 ****************************************************************************/

int nxflat_read(struct nxflat_loadinfo_s *loadinfo, char *buffer, int readsize, int offset)
{
  ssize_t nbytes;      /* Number of bytes read */
  off_t   rpos;        /* Position returned by lseek */
  char   *bufptr;      /* Next buffer location to read into */
  int     bytesleft;   /* Number of bytes of .data left to read */
  int     bytesread;   /* Total number of bytes read */

  /* Seek to the position in the object file where the initialized
   * data is saved.
   */

  bytesread = 0;
  bufptr    = buffer;
  bytesleft = readsize;
  do
    {
      rpos = lseek(loadinfo->filfd, offset, SEEK_SET);
      if (rpos != offset)
        {
          bdbg("Failed to seek to position %d: %d\n", offset, errno);
          return -errno;
        }

      /* Read the file data at offset into the user buffer */

       nbytes = read(loadinfo->filfd, bufptr, bytesleft);
       if (nbytes < 0)
         {
           if (errno != EINTR)
             {
               bdbg("Read of .data failed: %d\n", errno);
               return -errno;
             }
         }
       else if (nbytes == 0)
         {
           bdbg("Unexpected end of file\n");
           return -ENODATA;
         }
       else
         {
           bytesread += nbytes;
           bytesleft -= nbytes;
           bufptr    += nbytes;
           offset    += nbytes;
         }
    }
  while (bytesread < readsize);
  return OK;
}

