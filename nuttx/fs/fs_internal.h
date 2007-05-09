/****************************************************************************
 * fs_internal.h
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

#ifndef __FS_INTERNAL_H
#define __FS_INTERNAL_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <nuttx/fs.h>
#include <dirent.h>
#include <nuttx/compiler.h>

/****************************************************************************
 * Definitions
 ****************************************************************************/

#define FSNODEFLAG_TYPE_MASK      0x00000003
#define   FSNODEFLAG_TYPE_DRIVER  0x00000000
#define   FSNODEFLAG_TYPE_BLOCK   0x00000001
#define   FSNODEFLAG_TYPE_MOUNTPT 0x00000002
#define FSNODEFLAG_DELETED        0x00000004

#define INODE_IS_DRIVER(i) \
  (((i)->i_flags & FSNODEFLAG_TYPE_MASK) == FSNODEFLAG_TYPE_DRIVER)
#define INODE_IS_BLOCK(i) \
  (((i)->i_flags & FSNODEFLAG_TYPE_BLOCK) == FSNODEFLAG_TYPE_BLOCK)
#define INODE_IS_MOUNTPT(i) \
  (((i)->i_flags & FSNODEFLAG_TYPE_MOUNTPT) == FSNODEFLAG_TYPE_MOUNTPT)

#define INODE_SET_DRIVER(i) \
  ((i)->i_flags &= ~FSNODEFLAG_TYPE_MASK)
#define INODE_SET_BLOCK(i) \
  ((i)->i_flags = (((i)->i_flags & ~FSNODEFLAG_TYPE_MASK) | FSNODEFLAG_TYPE_BLOCK))
#define INODE_SET_MOUNTP(i) \
  ((i)->i_flags = (((i)->i_flags & ~FSNODEFLAG_TYPE_MASK) | FSNODEFLAG_TYPE_MOUNTP))

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* The internal representation of type DIR is just a
 * container for an inode reference and a dirent structure.
 */

struct internal_dir_s
{
  struct inode *root;  /* The start inode (in case we
                        * rewind) */
  struct inode *next;  /* The inode to use for the next call
                        * to readdir() */
  struct dirent dir;   /* Populated using inode when readdir
                        * is called */
};

/****************************************************************************
 * Global Variables
 ****************************************************************************/

extern FAR struct inode *root_inode;

/****************************************************************************
 * Pulblic Function Prototypes
 ****************************************************************************/

#undef EXTERN
#if defined(__cplusplus)
#define EXTERN extern "C"
extern "C" {
#else
#define EXTERN extern
#endif

/* fs_inode.c ***************************************************************/

EXTERN void inode_semtake(void);
EXTERN void inode_semgive(void);
EXTERN FAR struct inode *inode_search(const char **path,
                                      FAR struct inode **peer,
                                      FAR struct inode **parent,
                                      const char **relpath);
EXTERN void inode_free(FAR struct inode *node);
EXTERN const char *inode_nextname(const char *name);

/* fs_inodereserver.c ********************************************************/

EXTERN FAR struct inode *inode_reserve(const char *path);

/* fs_inoderemove.c **********************************************************/

EXTERN STATUS inode_remove(const char *path);

/* fs_inodefind.c ************************************************************/

EXTERN FAR struct inode *inode_find(const char *path, const char **relpath);

/* fs_inodefinddir.c *********************************************************/

EXTERN FAR struct inode *inode_finddir(const char *path);

/* fs_inodeaddref.c **********************************************************/

EXTERN void inode_addref(FAR struct inode *inode);

/* fs_inoderelease.c *********************************************************/

EXTERN void inode_release(FAR struct inode *inode);

/* fs_files.c ***************************************************************/

#if CONFIG_NFILE_DESCRIPTORS >0
EXTERN void weak_function files_initialize(void);
EXTERN int  files_allocate(FAR struct inode *inode, int oflags, off_t pos);
EXTERN void files_release(int filedes);
#endif

#undef EXTERN
#if defined(__cplusplus)
}
#endif

#endif /* __FS_INTERNAL_H */
