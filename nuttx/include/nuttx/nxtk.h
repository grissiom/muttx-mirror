/****************************************************************************
 * include/nuttx/nxtk.h
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

#ifndef __INCLUDE_NUTTX_NXTK_H
#define __INCLUDE_NUTTX_NXTK_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <sys/types.h>

#include <nuttx/nx.h>

/****************************************************************************
 * Pre-processor definitions
 ****************************************************************************/

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* This is the handle that can be used to access the window data region */

typedef FAR void *NXTKWINDOW;

/* This is the handle that can be used to access the window toolbar */

typedef FAR void *NXTKTOOLBAR;

/****************************************************************************
 * Public Data
 ****************************************************************************/

#undef EXTERN
#if defined(__cplusplus)
# define EXTERN extern "C"
extern "C" {
#else
# define EXTERN extern
#endif

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: nxtk_openwindow
 *
 * Description:
 *   Create a new, framed window.
 *
 * Input Parameters:
 *   handle - The handle returned by nx_connect
 *   cb     - Callbacks used to process window events
 *   arg    - User provided value that will be returned with NXTK callbacks.
 *
 * Return:
 *   Success: A non-NULL handle used with subsequent NXTK window accesses
 *   Failure:  NULL is returned and errno is set appropriately
 *
 ****************************************************************************/

EXTERN NXTKWINDOW nxtk_openwindow(NXHANDLE handle,
                                  FAR const struct nx_callback_s *cb,
                                  FAR void *arg);

/****************************************************************************
 * Name: nxtk_closewindow
 *
 * Description:
 *   Close the window opened by nxtk_openwindow
 *
 * Input Parameters:
 *   hfwnd - The handle returned by nxtk_openwindow
 *
 * Return:
 *   OK on success; ERROR on failure with errno set appropriately
 *
 ****************************************************************************/

EXTERN int nxtk_closewindow(NXTKWINDOW hfwnd);

/****************************************************************************
 * Name: nxtk_getposition
 *
 * Description:
 *  Request the position and size information for the selected framed window.
 *  The size/position for the client window and toolbar will be return
 *  asynchronously through the client callback function pointer.
 *
 * Input Parameters:
 *   hfwnd - The window handle returned by nxtk_openwindow.
 *
 * Return:
 *   OK on success; ERROR on failure with errno set appropriately
 *
 ****************************************************************************/

EXTERN int nxtk_getposition(NXTKWINDOW hfwnd);

/****************************************************************************
 * Name: nxtk_setposition
 *
 * Description:
 *  Set the position for the selected client window.  This position does not
 *  include the offsets for the borders nor for any toolbar.  Those offsets
 *  will be added in to set the full window position.
 *
 * Input Parameters:
 *   hfwnd - The window handle returned by nxtk_openwindow
 *   pos   - The new position of the client sub-window
 *
 * Return:
 *   OK on success; ERROR on failure with errno set appropriately
 *
 ****************************************************************************/

EXTERN int nxtk_setposition(NXTKWINDOW hfwnd, FAR struct nxgl_point_s *pos);

/****************************************************************************
 * Name: nxtk_setsize
 *
 * Description:
 *  Set the size for the selected client window.  This size does not
 *  include the sizes of the borders nor for any toolbar.  Those sizes
 *  will be added in to set the full window size.
 *
 * Input Parameters:
 *   hfwnd - The window handle returned by nxtk_openwindow
 *   size  - The new size of the client sub-window.
 *
 * Return:
 *   OK on success; ERROR on failure with errno set appropriately
 *
 ****************************************************************************/

EXTERN int nxtk_setsize(NXTKWINDOW hfwnd, FAR struct nxgl_size_s *size);

/****************************************************************************
 * Name: nxtk_raise
 *
 * Description:
 *   Bring the window containing the specified client sub-window to the top
 *   of the display.
 *
 * Input parameters:
 *   hfwnd - the window to be raised.  This must have been previously created
 *           by nxtk_openwindow().
 *
 * Returned value:
 *   OK on success; ERROR on failure with errno set appropriately
 *
 ****************************************************************************/

EXTERN int nxtk_raise(NXTKWINDOW hfwnd);

/****************************************************************************
 * Name: nxtk_fillwindow
 *
 * Description:
 *  Fill the specified rectangle in the client window with the specified color
 *
 * Input Parameters:
 *   hfwnd - The window handle returned by nxtk_openwindow
 *   rect  - The location within the client window to be filled
 *   color - The color to use in the fill
 *
 * Return:
 *   OK on success; ERROR on failure with errno set appropriately
 *
 ****************************************************************************/

EXTERN int nxtk_fillwindow(NXTKWINDOW hfwnd, FAR const struct nxgl_rect_s *rect,
                           nxgl_mxpixel_t color[CONFIG_NX_NPLANES]);

/****************************************************************************
 * Name: nxtk_filltrapwindow
 *
 * Description:
 *  Fill the specified rectangle in the client window with the specified color
 *
 * Input Parameters:
 *   hfwnd - The window handle returned by nxtk_openwindow
 *   trap  - The trapezoidal region to be filled
 *   color - The color to use in the fill
 *
 * Return:
 *   OK on success; ERROR on failure with errno set appropriately
 *
 ****************************************************************************/

EXTERN int nxtk_filltrapwindow(NXTKWINDOW hfwnd,
                               FAR const struct nxgl_trapezoid_s *trap,
                               nxgl_mxpixel_t color[CONFIG_NX_NPLANES]);

/****************************************************************************
 * Name: nxtk_movewindow
 *
 * Description:
 *   Move a rectangular region within the client sub-window of a framed window
 *
 * Input Parameters:
 *   hfwnd   - The client sub-window within which the move is to be done.
 *            This must have been previously created by nxtk_openwindow().
 *   rect   - Describes the rectangular region relative to the client
 *            sub-window to move
 *   offset - The offset to move the region
 *
 * Return:
 *   OK on success; ERROR on failure with errno set appropriately
 *
 ****************************************************************************/

EXTERN int nxtk_movewindow(NXTKWINDOW hfwnd, FAR const struct nxgl_rect_s *rect,
                           FAR const struct nxgl_point_s *offset);

/****************************************************************************
 * Name: nxtk_bitmapwindow
 *
 * Description:
 *   Copy a rectangular region of a larger image into the rectangle in the
 *   specified client sub-window.
 *
 * Input Parameters:
 *   hfwnd    The client sub0window that will receive the bitmap image
 *   dest   - Describes the rectangular region on in the client sub-window
 *            will receive the bit map.
 *   src    - The start of the source image.
 *   origin - The origin of the upper, left-most corner of the full bitmap.
 *            Both dest and origin are in window coordinates, however, origin
 *            may lie outside of the display.
 *   stride - The width of the full source image in pixels.
 *
 * Return:
 *   OK on success; ERROR on failure with errno set appropriately
 *
 ****************************************************************************/

EXTERN int nxtk_bitmapwindow(NXTKWINDOW hfwnd,
                             FAR const struct nxgl_rect_s *dest,
                             FAR const void *src[CONFIG_NX_NPLANES],
                             FAR const struct nxgl_point_s *origin,
                             unsigned int stride);

/****************************************************************************
 * Name: nxtk_opentoolbar
 *
 * Description:
 *   Create a tool bar at the top of the specified framed window
 *
 * Input Parameters:
 *   hfwnd   - The handle returned by nxtk_openwindow
 *   height - The request height of the toolbar in pixels
 *   cb     - Callbacks used to process toolbar events
 *   arg    - User provided value that will be returned with toolbar callbacks.
 *
 * Return:
 *   Success: A non-NULL handle used with subsequent NXTK toolbar accesses
 *   Failure:  NULL is returned and errno is set appropriately
 *
 ****************************************************************************/

EXTERN NXTKTOOLBAR nxtk_opentoolbar(NXTKWINDOW hfwnd, nxgl_coord_t height,
                                    FAR const struct nx_callback_s *cb,
                                    FAR void *arg);

/****************************************************************************
 * Name: nxtk_closetoolbar
 *
 * Description:
 *   Create a tool bar at the top of the specified framed window
 *
 * Input Parameters:
 *   htb - The toolbar handle returned by nxtk_opentoolbar
 *
 * Return:
 *   OK on success; ERROR on failure with errno set appropriately
 *
 ****************************************************************************/

EXTERN int nxtk_closetoolbar(NXTKTOOLBAR htb);

/****************************************************************************
 * Name: nxtk_filltoolbar
 *
 * Description:
 *  Fill the specified rectangle in the client window with the specified color
 *
 * Input Parameters:
 *   htb  -  The toolbar handle returned by nxtk_opentoolbar
 *   rect  - The location within the toolbar window to be filled
 *   color - The color to use in the fill
 *
 * Return:
 *   OK on success; ERROR on failure with errno set appropriately
 *
 ****************************************************************************/

EXTERN int nxtk_filltoolbar(NXTKTOOLBAR htb, FAR const struct nxgl_rect_s *rect,
                            nxgl_mxpixel_t color[CONFIG_NX_NPLANES]);

/****************************************************************************
 * Name: nxtk_filltraptoolbar
 *
 * Description:
 *  Fill the specified rectangle in the toolbar with the specified color
 *
 * Input Parameters:
 *   htb - The window handle returned by nxtk_openwindow
 *   trap  - The trapezoidal region to be filled
 *   color - The color to use in the fill
 *
 * Return:
 *   OK on success; ERROR on failure with errno set appropriately
 *
 ****************************************************************************/

EXTERN int nxtk_filltraptoolbar(NXTKTOOLBAR htb, FAR const struct nxgl_trapezoid_s *trap,
                                nxgl_mxpixel_t color[CONFIG_NX_NPLANES]);
#undef EXTERN
#if defined(__cplusplus)
}
#endif

#endif /* __INCLUDE_NUTTX_NXTK_H */
