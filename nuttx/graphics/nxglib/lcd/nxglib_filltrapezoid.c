/****************************************************************************
 * graphics/nxglib/lcd/nxglib_filltrapezoid.c
 *
 *   Copyright (C) 2010-2011 Gregory Nutt. All rights reserved.
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

#include <stdint.h>
#include <fixedmath.h>

#include <nuttx/lcd/lcd.h>
#include <nuttx/nx/nxglib.h>

#include "nxglib_bitblit.h"
#include "nxglib_fillrun.h"

/****************************************************************************
 * Pre-Processor Definitions
 ****************************************************************************/

#ifndef NXGLIB_SUFFIX
#  error "NXGLIB_SUFFIX must be defined before including this header file"
#endif

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
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxglib_filltrapezoid_*bpp
 *
 * Descripton:
 *   Fill a trapezoidal region in the LCD memory with a fixed color.
 *   Clip the trapezoid to lie within a boundng box.  This is useful for
 *   drawing complex shapes that can be broken into a set of trapezoids.
 *
 ****************************************************************************/

void NXGL_FUNCNAME(nxgl_filltrapezoid,NXGLIB_SUFFIX)
  (FAR struct lcd_planeinfo_s *pinfo,
   FAR const struct nxgl_trapezoid_s *trap,
   FAR const struct nxgl_rect_s *bounds,
   NXGL_PIXEL_T color)
{
  unsigned int ncols;
  unsigned int dy;
  unsigned int topy;
  unsigned int boty;
  unsigned int row;
  unsigned int botw;
  b16_t        topx1;
  b16_t        topx2;
  b16_t        botx1;
  b16_t        botx2;
  b16_t        dx1dy;
  b16_t        dx2dy;
  int          ix1;
  int          ix2;

  /* Get the top run endpoints */

  topx1 = trap->top.x1;
  topx2 = trap->top.x2;

  /* Calculate the number of rows to render */

  topy  = trap->top.y;
  boty  = trap->bot.y;

  /* Get the bottom run endpoints */

  botx1  = trap->bot.x1;
  botx2  = trap->bot.x2;

  /* Calculate the slope of the left and right side of the trapezoid */

  dy    = boty - topy;
  dx1dy = b16divi((botx1 - topx1), dy);
  dx2dy = b16divi((botx2 - topx2), dy);

  /* Perform vertical clipping */

  if (topy < bounds->pt1.y)
    {
      /* Calculate the x values for the new top run */

      dy      = bounds->pt1.y - topy;
      topx1  += dy * dx1dy;
      topx2  += dy * dx2dy;

      /* Clip the top row to render */

      topy    = bounds->pt1.y;
    }

  if (boty > bounds->pt2.y)
    {
      /* Calculate the x values for the new bottom run */

      dy      = boty - bounds->pt2.y;
      botx1  -= dy * dx1dy;
      botx2  -= dy * dx2dy;

      /* Clip the bottom row to render */

      boty    = bounds->pt2.y;
    }

  /* Break out now if it was completely clipped */

  if (topy > boty)
    {
      return;
    }

  /* Handle the special case where the sides cross (as in an hourglass) */

  if (botx1 > botx2)
    {
      b16_t tmp;
      ngl_swap(botx1, botx2, tmp);
    }

  /* Fill the run buffer for the maximum run that we will need */

  ix1    = ngl_clipl(b16toi(topx1), bounds->pt1.x);
  ix2    = ngl_clipl(b16toi(topx2), bounds->pt2.x);
  ncols  = ix2 - ix1 + 1;

  ix1    = ngl_clipl(b16toi(botx1), bounds->pt1.x);
  ix2    = ngl_clipl(b16toi(botx2), bounds->pt2.x);
  botw   = ix2 - ix1 + 1;

  if (ncols < botw)
    {
      ncols = botw;
    }

  NXGL_FUNCNAME(nxgl_fillrun,NXGLIB_SUFFIX)((NXGLIB_RUNTYPE*)pinfo->buffer, color, ncols);

  /* Then fill the trapezoid line-by-line */

  for (row = topy; row <= boty; row++)
    {
      /* Handle the special case where the sides cross (as in an hourglass) */

      if (topx1 > topx2)
        {
          b16_t tmp;
          ngl_swap(topx1, topx2, tmp);
          ngl_swap(dx1dy, dx2dy, tmp);
        }

      /* Convert the positions to integer and get the run width,
       * clipping to fit within the bounding box.
       */

      ix1 = ngl_clipl(b16toi(topx1), bounds->pt1.x);
      ix2 = ngl_clipl(b16toi(topx2), bounds->pt2.x);

      /* Handle some corner cases where we draw nothing.  Otherwise, we will
       * always draw at least one pixel.
       */

      if (ix1 <= ix2)
        {
          /* Then draw the run from ix1 to ix2 at row */

          ncols = ix2 - ix1 + 1;
          (void)pinfo->putrun(row, ix1, pinfo->buffer, ncols);
        }

      /* Add the dx/dy value to get the run positions on the next row */

      topx1 += dx1dy;
      topx2 += dx2dy;
    }
}
