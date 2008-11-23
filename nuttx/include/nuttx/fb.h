/****************************************************************************
 * include/nuttx/fb.h
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

#ifndef _INCLUDE_NUTTX_FB_H
#define _INCLUDE_NUTTX_FB_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <sys/types.h>

/****************************************************************************
 * Pre-processor definitions
 ****************************************************************************/

/* Color format definitions.  The pretty much define the color pixel processing
 * organization of the video controller.
 */

/* Monochrome Formats *******************************************************/

#define FB_FMT_Y8          0         /* BPP=8  8-bit uncompressed greyscale */
#define FB_FMT_Y16         1         /* BPP=16  16-bit uncompressed greyscale */
#define FB_FMT_GREY        FB_FMT_Y8 /* BPP=8 */
#define FB_FMT_Y800        FB_FMT_Y8 /* BPP=8 */

#define FB_ISMONO(f)  ((f) >= FB_FMT_Y8) && (f) <= FB_FMT_Y16)

/* RGB video formats ********************************************************/

/* Standard RGB */

#define FB_FMT_RGB1        2           /* BPP=1 */
#define FB_FMT_RGB4        3           /* BPP=4 */
#define FB_FMT_RGB8        4           /* BPP=8 */
#define FB_FMT_RGB16       5           /* BPP=16 */
#define FB_FMT_RGB24       6           /* BPP=24 */
#define FB_FMT_RGB32       7           /* BPP=32 */

/* Run length encoded RGB */

#define FB_FMT_RGBRLE4     8           /* BPP=4 */
#define FB_FMT_RGBRLE8     9           /* BPP=8 */

/* Raw RGB */

#define FB_FMT_RGBRAW      10          /* BPP=? */

/* Raw RGB with arbitrary sample packing within a pixel. Packing and precision
 * ov R, G and B components is determined by bit masks for each.
 */

#define FB_FMT_RGBBTFLD16  11          /* BPP=16 */
#define FB_FMT_RGBBTFLD24  12          /* BPP=24 */
#define FB_FMT_RGBBTFLD24  13          /* BPP=32 */
#define FB_FMT_RGBA16      14          /* BPP=16 Raw RGB with alpha */
#define FB_FMT_RGBA32      15          /* BPP=32 Raw RGB with alpha */

/* Raw RGB with a transparency field. Layout is as for stanadard RGB at 16 and
 * 32 bits per pixel but the msb in each pixel indicates whether the pixel is
 * transparent or not.
 */

#define FB_FMT_RGBT16      16          /* BPP=16  */
#define FB_FMT_RGBT32      17          /* BPP=32 */

#define FB_ISRGB(f)  ((f) >= FB_FMT_RGB1) && (f) <= FB_FMT_RGBT32)

/* Packed YUV Formats *******************************************************/

#define FB_FMT_AYUV        18          /* BPP=32  Combined YUV and alpha */
#define FB_FMT_CLJR        19          /* BPP=8   4 pixels packed into a uint32.
                                        * YUV 4:1:1 with l< 8 bits per YUV sample */
#define FB_FMT_CYUV        20          /* BPP=16  UYVY except that height is reversed */
#define FB_FMT_IRAW        21          /* BPP=?   Intel uncompressed YUV.
#define FB_FMT_IUYV        22          /* BPP=16  Interlaced UYVY (line order
                                        * 0,2,4,.., 1,3,5...)
#define FB_FMT_IY41        23          /* BPP=12  Interlaced Y41P (line order
                                        * 0,2,4,.., 1,3,5...)
#define FB_FMT_IYU2        24          /* BPP=24 */
#define FB_FMT_HDYC        25          /* BPP=16  UYVY except uses the BT709 color space  */
#define FB_FMT_UYVP        26          /* BPP=24? YCbCr 4:2:2, 10-bits per component in U0Y0V0Y1 order */
#define FB_FMT_UYVY        27          /* BPP=16  YUV 4:2:2 */
#define FB_FMT_UYNV        FB_FMT_UYVY /* BPP=16  */
#define FB_FMT_Y422        FB_FMT_UYVY /* BPP=16  */
#define FB_FMT_V210        28          /* BPP=32  10-bit 4:2:2 YCrCb */
#define FB_FMT_V422        29          /* BPP=16  Upside down version of UYVY.
#define FB_FMT_V655        30          /* BPP=16? 16-bit YUV 4:2:2 */
#define FB_FMT_VYUY        31          /* BPP=?   ATI Packed YUV Data
#define FB_FMT_YUYV        32          /* BPP=16  YUV 4:2:2 */
#define FB_FMT_YUY2        FB_FMT_YUYV /* BPP=16  YUV 4:2:2 */
#define FB_FMT_YUNV        FB_FMT_YUYV /* BPP=16  YUV 4:2:2 */
#define FB_FMT_YVYU        33          /* BPP=16  YUV 4:2:2 */
#define FB_FMT_Y41P        34          /* BPP=12  YUV 4:1:1 */
#define FB_FMT_Y411        35          /* BPP=12  YUV 4:1:1 */
#define FB_FMT_Y211        36          /* BPP=8  */
#define FB_FMT_Y41T        37          /* BPP=12  Y41P LSB for transparency */
#define FB_FMT_Y42T        38          /* BPP=16  UYVY LSB for transparency */
#define FB_FMT_YUVP        39          /* BPP=24? YCbCr 4:2:2 Y0U0Y1V0 order */

#define FB_ISYUVPACKED(f)  ((f) >= FB_FMT_AYUV) && (f) <= FB_FMT_YUVP)

/* Packed Planar YUV Formats ************************************************/

#define FB_FMT_YVU9        40          /* BPP=9  8-bit Y followed by 8-bit 4x4 VU */
#define FB_FMT_YUV9        41          /* BPP=9? */
#define FB_FMT_IF09        42          /* BPP=9.5 YVU9 + 4x4 plane of delta relative to tframe. */
#define FB_FMT_YV16        43          /* BPP=16  8-bit Y followed by 8-bit 2x1 VU */
#define FB_FMT_YV12        44          /* BPP=12  8-bit Y followed by 8-bit 2x2 VU */
#define FB_FMT_I420        45          /* BPP=12  8-bit Y followed by 8-bit 2x2 UV */
#define FB_FMT_IYUV        FB_FMT_I420 /* BPP=12 */
#define FB_FMT_NV12        46          /* BPP=12  8-bit Y followed by an interleaved 2x2 UV */
#define FB_FMT_NV21        47          /* BPP=12  NV12 with UV reversed */
#define FB_FMT_IMC1        48          /* BPP=12  YV12 except UV planes ame stride as Y */
#define FB_FMT_IMC2        49          /* BPP=12  IMC1 except UV lines interleaved at half stride boundaries */
#define FB_FMT_IMC3        50          /* BPP=12  As IMC1 except that UV swapped */
#define FB_FMT_IMC4        51          /* BPP=12  As IMC2  except that UV swapped */
#define FB_FMT_CLPL        52          /* BPP=12  YV12 but including a level of indirection. */
#define FB_FMT_Y41B        53          /* BPP=12?  4:1:1 planar. */
#define FB_FMT_Y42B        54          /* BPP=16?  YUV 4:2:2 planar. */
#define FB_FMT_CXY1        55          /* BPP=12 */
#define FB_FMT_CXY2        56          /* BPP=16 */

#define FB_ISYUVPLANAR(f)  ((f) >= FB_FMT_AYUV) && (f) <= FB_FMT_YUVP)
#define FB_ISYUV(f)        (FB_ISYUVPACKED(f) || FB_ISYUVPLANAR(f))

/* Hardware cursor control **************************************************/

#ifdef CONFIG_FB_HWCURSOR
#define FB_CUR_ENABLE      0x01        /* Enable the cursor
#define FB_CUR_SETIMAGE    0x02        /* Set the cursor image */
#define FB_CUR_SETPOSITION 0x04        /* Set the position of the cursor */
#define FB_CUR_SETSIZE     0x08        /* Set the size of the cursor */
#define FB_CUR_XOR         0x10        /* Use XOR vs COPY ROP on image */
#endif

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* If any dimension of the display exceeds 65,536 pixels, then the following
 * type will need to change:
 */

typedef uint16 fb_coord_t;

/* This structure describes the overall video controller */

struct fb_videoinfo_s
{
 ubyte      fmt;          /* see FB_FMT_*  */
 fb_coord_t xres;         /* Resolution in pixels */
 fb_coord_t yres;
 ubyte      nplanes;      /* Number of color planes supported */
};

/* This structure describes one color plane.  Some YUV formats may support
 * up to 4 planes
 */

struct fb_planeinfo_s
{
  FAR void  *fbmem;       /* Start of frame buffer memory */
  uint32     fblen;       /* Length of frame buffer memory in bytes */
  fb_coord_t stride;      /* Length of a line in bytes */
  ubyte      bpp;         /* Bits per pixel */
};

/* On video controllers that support mapping of a pixel palette value
 * to an RGB encoding, the following structure may be used to define
 * that mapping.
 */

#if CONFIG_FB_CMAP
struct fb_cmap_s
{
 uint16  len;              /* Number of color entries */

 /* Each color component.  Any may be NULL if not used */

 uint16 *red;              /* Table of red values */
 uint16 *green;            /* Table of red values */
 uint16 *blue;             /* Table of red values */
#ifdef CONFIG_FB_TRANSPARENCY
 uint16 *transp;           /* Table of transparency */
#endif
};
#endif

/* If the video controller hardware supports a hardware cursor and
 * that hardware cursor supports user-provided images, then the
 * following structure may be used to provide the cursor image
 */

#ifdef CONFIG_FB_HWCURSOR
#ifdef CONFIG_FB_HWCURSORIMAGE
struct fb_cursorimage_s
{
 fb_coord_t   width;       /* Width of the cursor image in pixels */
 fb_coord_t   height       /* Height of the curor image in pixels */
 const ubyte *image;       /* Pointer to image data */
};
#endif

/* The following structure defines the cursor position/size */

struct fb_cursorpos_s
{
 fb_coord_t x;             /* X position in pixels */
 fb_coord_t y;             /* Y position in rows */
};

/* If the hardware supports setting the cursor size, then this structure
 * is used to provide the size.
 */

#ifdef CONFIG_FB_HWCURSORSIZE
struct fb_cursorsize_s
{
 fb_coord_t h;             /* Height in rows */
 fb_coord_t w;             /* Width in pixels */
};
#endif

/* The following is used to get the cursor attributes */

struct fb_cursorattrib_s
{
#ifdef CONFIG_FB_HWCURSORIMAGE
  ubyte fmt;                     /* Video format of cursor */
#endif
  struct fb_cursorpos_s pos;     /* Current cursor position */
#ifdef CONFIG_FB_HWCURSORSIZE
  struct fb_cursorsize_s mxsize; /* Maximum cursor size */
  struct fb_cursorsize_s size;   /* Current size */
#endif
};

struct fb_setcursor_s
{
  ubyte flags;                  /* See FB_CUR_* definitions */
  struct fb_cursorpos_s pos;    /* Cursor position */
#ifdef CONFIG_FB_HWCURSORSIZE
  struct fb_cursorsize_s  size; /* Cursor size */
#endif
#ifdef CONFIG_FB_HWCURSORIMAGE
  struct fb_cursorimage_s img;  /* Cursor image */
#endif
};
#endif

/* The framebuffer "driver" under NuttX is not a driver at all, but simply
 * a driver "object" that is accessed through the following vtable:
 */

struct fb_vtable_s
{
  /* Get information about the video controller configuration and the configuration
   * of each color plane.
   */

  int (*getvideoinfo)(FAR struct fb_vtable_s *vtable, FAR struct fb_videoinfo_s *vinfo);
  int (*getplaneinfo)(FAR struct fb_vtable_s *vtable, int planeno, FAR struct fb_planeinfo_s *pinfo);

  /* The following is provided only if the video hardware supports RGB color mapping */

#if CONFIG_FB_CMAP
  int (*getcmap)(FAR struct fb_vtable_s *vtable, FAR struct fb_cmap_s *cmap);
  int (*putcmap)(FAR struct fb_vtable_s *vtable, FAR struct fb_cmap_s *cmap);
#endif
  /* The following is provided only if the video hardware supports a hardware cursor */

#ifdef CONFIG_FB_HWCURSOR
  int (*getcursor)(FAR struct fb_vtable_s *vtable, FAR struct fb_cursorattrib_s *attrib);
  int (*setcursor)(FAR struct fb_vtable_s *vtable, FAR struct fb_setcursor_s *setttings);
#endif
};
#endif /* _INCLUDE_NUTTX_FB_H */
