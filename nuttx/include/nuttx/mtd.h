/****************************************************************************
 * include/nuttx/mtd.h
 * Memory Technology Device (MTD) interface
 *
 *   Copyright (C) 2007-2009 Gregory Nutt. All rights reserved.
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

#ifndef __INCLUDE_NUTTX_MTD_H
#define __INCLUDE_NUTTX_MTD_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <sys/types.h>

/****************************************************************************
 * Pre-Processor Definitions
 ****************************************************************************/

/* Macros to hide implementation */

#define MTD_ERASE(d,s,n)   ((d)->erase ? (d)->erase(d,s,n)   : (-ENOSYS))
#define MTD_READ(d,s,n,b)  ((d)->read  ? (d)->read(d,s,n,b)  : (-ENOSYS))
#define MTD_WRITE(d,s,n,b) ((d)->write ? (d)->write(d,s,n,b) : (-ENOSYS))
#define MTD_IOCTL(d,c,a)   ((d)->ioctl ? (d)->ioctl(d,c,a)   : (-ENOSYS))

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* The following defines the geometry for the device.  It treats the device
 * as though it where just an array of fixed size blocks.  That is most likely
 * not true, but the client will expect the device logic to do whatever is
 * necessary to make it appear so.
 */

struct mtd_geometry_s
{
  uint16 blocksize;     /* Size of one read/write block */
  uint16 erasesize;     /* Size of one erase blocks -- must be a multiple
                         * of blocksize. */
  size_t neraseblocks; /* Number of erase blocks */
};

/* This structure defines the interface to a simple memory technology device.
 * It will likely need to be extended in the future to support more complex
 * devices.
 */

struct mtd_dev_s
{
  /* The following methods operate on the MTD: */

  /* Erase the specified erase blocks */

  int (*erase)(FAR struct mtd_dev_s *dev, off_t startblock, size_t nblocks);

  /* Read/write from the specified read/write blocks */

  int (*read)(FAR struct mtd_dev_s *dev, off_t startblock, size_t nblocks,
              FAR ubyte *buffer);
  int (*write)(FAR struct mtd_dev_s *dev, off_t startblock, size_t nblocks,
               FAR const ubyte *buffer);

  /* Support other, less frequently used commands:
   *  - MTDIOC_GEOMETRY: Get MTD geometry
   *  - MTDIOC_XIPBASE:  Convert block to physical address for eXecute-In-Place
   * (see include/nuttx/ioctl.h) 
   */

  int (*ioctl)(FAR struct mtd_dev_s *dev, int cmd, unsigned long arg);
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

#ifndef __ASSEMBLY__

#ifdef __cplusplus
#define EXTERN extern "C"
extern "C" {
#else
#define EXTERN extern
#endif

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* __ASSEMBLY__ */
#endif /* __INCLUDE_NUTTX_MTD_H */
