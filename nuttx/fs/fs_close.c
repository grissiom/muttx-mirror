/****************************************************************************
 * fs_close.c
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
#include <unistd.h>
#include <sched.h>
#include <errno.h>
#include <nuttx/fs.h>
#include "fs_internal.h"

/****************************************************************************
 * Global Functions
 ****************************************************************************/

#if CONFIG_NFILE_DESCRIPTORS > 0

int close(int fd)
{
  FAR struct filelist *list;

  /* Get the thread-specific file list */

  list = sched_getfiles();
  if (!list)
    {
      *get_errno_ptr() = EMFILE;
      return ERROR;
    }

  if ((unsigned int)fd < CONFIG_NFILE_DESCRIPTORS)
    {
      FAR struct inode *inode = list->fl_files[fd].f_inode;
      if (inode)
        {
          int ret = OK;

          /* Close the driver or mountpoint.  NOTES: (1) there is no
           * exclusion mechanism here , the driver or mountpoint must be
           * able to handle concurrent operations internally, (2) The driver
           * may have been opened numerous times (for different file
           * descriptors) and must also handle being closed numerous times.
           * (3) for the case of the mountpoint, we depend on the close
           *  methods bing identical in signature and position in the operations
           * vtable.
           */

          if (inode->u.i_ops && inode->u.i_ops->close)
            {
              /* Perform the close operation (by the driver) */

              int status = inode->u.i_ops->close(&list->fl_files[fd]);
              if (status < 0)
                {
                  /* An error occurred while closing the driver */

                  *get_errno_ptr() = -status;
                  ret = ERROR;
                }
            }

          /* Release the file descriptor */

          files_release(fd);

          /* Decrement the reference count on the inode. This may remove the inode and
           * eliminate the name from the namespace
           */

          inode_release(inode);
          return ret;
        }
    }

  *get_errno_ptr() = EBADF;
  return ERROR;
}

#endif /* CONFIG_NFILE_DESCRIPTORS */
