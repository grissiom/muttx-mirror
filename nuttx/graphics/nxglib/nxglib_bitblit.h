/****************************************************************************
 * graphics/nxglib/nxglib_bitblit.h
 *
 *   Copyright (C) 2008 Gregory Nutt. All rights reserved.
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

#ifndef __GRAPHICS_NXGLIB_NXGLIB_BITBLIT_H
#define __GRAPHICS_NXGLIB_NXGLIB_BITBLIT_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <sys/types.h>

#include <nuttx/nxglib.h>

/****************************************************************************
 * Pre-Processor Definitions
 ****************************************************************************/

/* Make sure the the bits-per-pixel value has been set by the includer of
 * this header file.
 */

#ifndef NXGLIB_BITSPERPIXEL
#  error "NXGLIB_BITSPERPIXEL must be defined before including this header file"
#endif

/* Set up bit blit macros for this BPP */

#if NXGLIB_BITSPERPIXEL == 1

#  define NX_PIXELSHIFT          3
#  define NX_PIXELMASK           7
#  define NX_MULTIPIXEL(p)       ((p) ? 0xff | 0x00)
#  define NX_PIXEL_T             ubyte

#elif NXGLIB_BITSPERPIXEL == 2

#  define NX_PIXELSHIFT          2
#  define NX_PIXELMASK           3
#  define NX_MULTIPIXEL(p)       ((ubyte)(p) << 6 | (ubyte)(p) << 4 | (ubyte)(p) << 2 | (p))
#  define NX_PIXEL_T             ubyte

#elif NXGLIB_BITSPERPIXEL == 4

#  define NX_PIXELSHIFT          1
#  define NX_PIXELMASK           1
#  define NX_MULTIPIXEL(p)       ((ubyte)(p) << 4 | (p))
#  define NX_PIXEL_T             ubyte

#elif NXGLIB_BITSPERPIXEL == 8

#  define NX_SCALEX(x)           (x)
#  define NX_PIXEL_T             ubyte

#elif NXGLIB_BITSPERPIXEL == 16

#  define NX_SCALEX(x)           ((x) << 1)
#  define NX_PIXEL_T             uint16

#elif NXGLIB_BITSPERPIXEL == 24

#  define NX_SCALEX(x)           (((x) << 1) + (x))
#  define NX_PIXEL_T             uint32

#elif NXGLIB_BITSPERPIXEL == 32

#  define NX_SCALEX(x)           ((x) << 2)
#  define NX_PIXEL_T             uint32

#endif

#if NXGLIB_BITSPERPIXEL < 8
#  define NX_SCALEX(x)           ((x) >> NX_PIXELSHIFT)
#  define NX_REMAINDERX(x)       ((x) & NX_PIXELMASK)
#  define NX_ALIGNDOWN(x)        ((x) & ~NX_PIXELMASK)
#  define NX_ALIGNUP(x)          (((x) + NX_PIXELMASK) & ~NX_PIXELMASK)

#  ifdef CONFIG_NX_PACKEDMSFIRST
#    define NX_MASKEDSRC1(s,r)   ((s) & (((ubyte)0xff) >> (8 - ((r) << pixelshift))))
#    define NX_MASKEDVALUE1(s,r) ((s) & (((ubyte)0xff) << ((r) << NX_PIXELSHFIT)))
#    define NX_MASKEDSRC2(s,r)   ((s) & (((ubyte)0xff) >> ((r) << pixelshift)))
#    define NX_MASKEDVALUE2(s,r) ((s) & (((ubyte)0xff) << (8 - ((r) << NX_PIXELSHFIT))))
#  else
#    define NX_MASKEDSRC1(s,r)   ((s) & (((ubyte)0xff) >> ((r) << pixelshift)))
#    define NX_MASKEDVALUE1(s,r) ((s) & (((ubyte)0xff) << (8 - ((r) << NX_PIXELSHFIT))))
#    define NX_MASKEDSRC2(s,r)   ((s) & (((ubyte)0xff) >> (8 - ((r) << pixelshift))))
#    define NX_MASKEDVALUE2(s,r) ((s) & (((ubyte)0xff) << ((r) << NX_PIXELSHFIT)))
#  endif

#  define NXGL_MEMSET(dest,value,width) \
   { \
     FAR ubyte *_ptr = (FAR ubyte*)dest; \
     int nbytes      = NX_SCALEX(width); \
     while (nbytes--) \
       { \
         *_ptr++ = value; \
       } \
   }
#  define NXGL_MEMCPY(dest,src,width) \
   { \
     FAR ubyte *_dptr = (FAR ubyte*)dest; \
     FAR ubyte *_sptr = (FAR ubyte*)src; \
     int nbytes      = NX_SCALEX(width); \
     while (npixels--) \
       { \
         *_dptr++ = *_sptr++; \
       } \
   }

#elif NXGLIB_BITSPERPIXEL == 24
#  define NXGL_MEMSET(dest,value,width) \
   { \
     FAR ubyte *_ptr = (FAR ubyte*)dest; \
     while (width--) \
       { \
         *_ptr++ = value; \
         *_ptr++ = value >> 8; \
         *_ptr++ = value >> 16; \
       } \
   }
#  define NXGL_MEMCPY(dest,src,width) \
   { \
     FAR ubyte *_dptr = (FAR ubyte*)dest; \
     FAR ubyte *_sptr = (FAR ubyte*)src; \
     while (width--) \
       { \
         *_dptr++ = *_sptr++; \
         *_dptr++ = *_sptr++; \
         *_dptr++ = *_sptr++; \
       } \
   }
#else
#  define NXGL_MEMSET(dest,value,width) \
   { \
     FAR nx_pixel_t *_ptr = (FAR ubyte*)dest; \
     while (width--) \
       { \
         *_ptr++ = value; \
       } \
   }
#  define NXGL_MEMCPY(dest,src,width) \
   { \
     FAR nx_pixel_t *_dptr = (FAR ubyte*)dest; \
     FAR nx_pixel_t *_sptr = (FAR ubyte*)src; \
     while (width--) \
       { \
         *_dptr++ = *_sptr++; \
       } \
   }
#endif

/* Form a function name by concatenating two strings */

#define _NXGL_FUNCNAME(a,b) a ## b
#define NXGL_FUNCNAME(a,b)  _NXGL_FUNCNAME(a,b)

/****************************************************************************
 * Public Types
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/

#undef EXTERN
#if defined(__cplusplus)
#define EXTERN extern "C"
extern "C" {
#else
#define EXTERN extern
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

#undef EXTERN
#if defined(__cplusplus)
}
#endif

#endif /* __GRAPHICS_NXGLIB_NXGLIB_BITBLIT_H */
