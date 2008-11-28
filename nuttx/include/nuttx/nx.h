/****************************************************************************
 * include/nuttx/nx.h
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

#ifndef _INCLUDE_NUTTX_NX_H
#define _INCLUDE_NUTTX_NX_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <sys/types.h>
#include <nuttx/fb.h>
#include <nuttx/nxglib.h>

/****************************************************************************
 * Pre-Processor Definitions
 ****************************************************************************/

/* Default server MQ name used by nx_run() macro */

#define NX_DEFAULT_SERVER_MQNAME "/dev/nxs"

/* Mouse button bits */

#define NX_MOUSE_NOBUTTONS    0x00
#define NX_MOUSE_LEFTBUTTON   0x01
#define NX_MOUSE_CENTERBUTTON 0x02
#define NX_MOUSE_RIGHTBUTTON  0x04

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* Configuration ************************************************************/

#ifndef CONFIG_NX_NPLANES
#  define CONFIG_NX_NPLANES      1  /* Max number of color planes supported */
#endif

/* Handles ******************************************************************/

/* The interface to the NX server is managed using a opaque handle: */

typedef FAR void *NXHANDLE;

/* The interface to a specific window is managed using an opaque handle: */

typedef FAR void *NXWINDOW;

/* NX server callbacks ******************************************************/

/* These define callbacks that must be provided to nx_openwindow.  These
 * callbacks will be invoked as part of the processing performed by
 * nx_eventhandler()
 */

struct nx_callback_s
{
  /**************************************************************************
   * Name: redraw
   *
   * Descripton:
   *   NX requests that the client re-draw the portion of the window within
   *   with rectangle.
   *
   * Input Parameters:
   *   hwnd - Window handle
   *   rect - The rectangle that needs to be re-drawn (in window relative
   *          coordinates
   *   more - TRUE:  More re-draw requests will follow
   *
   * Returned Value:
   *   None
   *
   **************************************************************************/

  void (*redraw)(NXWINDOW hwnd, FAR const struct nxgl_rect_s *rect, boolean more);

  /**************************************************************************
   * Name: position
   *
   * Descripton:
   *   The size or position of the window has changed (or the window was
   *   just created with zero size.
   *
   * Input Parameters:
   *   hwnd   - Window handle
   *   size   - The size of the window (pt1 should always be zero)
   *   pos    - The position of the upper left hand corner of the window on
   *            the overalll display
   *   bounds - The bounding rectangle that the describes the entire
   *            display
   *
   * Returned Value:
   *   None
   *
   **************************************************************************/

  void (*position)(NXWINDOW hwnd, FAR const struct nxgl_rect_s *size,
                   FAR const struct nxgl_point_s *pos,
                   FAR const struct nxgl_rect_s *bounds);

  /**************************************************************************
   * Name: mousein
   *
   * Descripton:
   *   New mouse data is available for the window
   *
   * Input Parameters:
   *   hwnd    - Window handle
   *   pos     - The (x,y) position of the mouse
   *   buttons - See NX_MOUSE_* definitions
   *
   * Returned Value:
   *   None
   *
   **************************************************************************/

#ifdef CONFIG_NX_MOUSE
  void (*mousein)(NXWINDOW hwnd, FAR const struct nxgl_point_s *pos, ubyte buttons);
#endif

  /**************************************************************************
   * Name: kbdin
   *
   * Descripton:
   *   New keyboard/keypad data is available for the window
   *
   * Input Parameters:
   *   hwnd - Window handle
   *   nch  - The number of characters that are available in ch[]
   *   ch   - The array of characters
   *
   * Returned Value:
   *   None
   *
   **************************************************************************/

#ifdef CONFIG_NX_KBD
  void (*kbdin)(NXWINDOW hwnd, ubyte nch, const ubyte *ch);
#endif
};

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

/****************************************************************************
 * Name: nx_runinstance (and nx_run macro)
 *
 * Description:
 *   This is the server entry point.  It does not return; the calling thread
 *   is dedicated to supporting NX server.
 *
 *   NOTE that multiple instances of the NX server may run at the same time,
 *   with different callback and message queue names.  nx_run() is simply
 *   a macro that can be used when only one server instance is required.  In
 *   that case, a default server name is used.
 *
 *   Multiple user mode only!
 *
 * Input Parameters:
 *   mqname - The name for the server incoming message queue
 *   fb     - Vtable "object" of the framebuffer "driver" to use
 *
 * Return:
 *   This function usually does not return.  If it does return, it will
 *   return ERROR and errno will be set appropriately.
 *
 ****************************************************************************/

#ifdef CONFIG_NX_MULTIUSER
EXTERN int nx_runinstance(FAR const char *mqname, FAR struct fb_vtable_s *fb);
#  define nx_run(fb) nx_runinstance(NX_DEFAULT_SERVER_MQNAME, fb)
#endif

/****************************************************************************
 * Name:nx_connectinstance (and nx_connect macro)
 *
 * Description:
 *   Open a connection from a client to the NX server.  One one client
 *   connection is normally needed per thread as each connection can host
 *   multiple windows.
 *
 *   NOTE that multiple instances of the NX server may run at the same time,
 *   each with different message queue names.  nx_connect() is simply
 *   a macro that can be used when only one server instance is required.  In
 *   that case, a default server name is used.
 *
 *   Multiple user mode only!
 *
 * Input Parameters:
 *   svrmqname - The name for the server incoming message queue
 *
 * Return:
 *   Success: A non-NULL handle used with subsequent NX accesses
 *   Failure:  NULL is returned and errno is set appropriately
 *
 ****************************************************************************/

#ifdef CONFIG_NX_MULTIUSER
EXTERN NXHANDLE nx_connectionstance(FAR const char *svrmqname);
#  define nx_connect(cb) nx_connectionstance(NX_DEFAULT_SERVER_MQNAME)
#endif

/****************************************************************************
 * Name: nx_open
 *
 * Description:
 *   Create, initialize and return an NX handle for use in subsequent
 *   NX API calls.  nx_open is the single user equivalent of nx_connect
 *   plus nx_run.
 *
 *   Single user mode only!
 *
 * Input Parameters:
 *   fb - Vtable "object" of the framebuffer "driver" to use
 *   cb - Callbacks used to process received NX server messages
 *
 * Return:
 *   Success: A non-NULL handle used with subsequent NX accesses
 *   Failure:  NULL is returned and errno is set appropriately
 *
 ****************************************************************************/

#ifndef CONFIG_NX_MULTIUSER
EXTERN NXHANDLE nx_open(FAR struct fb_vtable_s *fb);
#endif

/****************************************************************************
 * Name: nx_disconnect
 *
 * Description:
 *   Disconnect a client from the NX server and/or free resources reserved
 *   by nx_connect/nx_connectinstance. nx_disconnect is muliti-user equivalent
 *   of nx_close.
 *
 *   Multiple user mode only!
 *
 * Input Parameters:
 *   handle - the handle returned by nx_connect
 *
 * Return:
 *   None
 *
 ****************************************************************************/

#ifdef CONFIG_NX_MULTIUSER
EXTERN void nx_disconnect(NXHANDLE handle);
#endif

/****************************************************************************
 * Name: nx_close
 *
 * Description:
 *   Close the single user NX interface.  nx_close is single-user equivalent
 *   of nx_disconnect.
 *
 *   Single user mode only!
 *
 * Input Parameters:
 *   handle - the handle returned by nx_open
 *
 * Return:
 *   None
 *
 ****************************************************************************/

#ifndef CONFIG_NX_MULTIUSER
EXTERN void nx_close(NXHANDLE handle);
#endif

/****************************************************************************
 * Name: nx_eventhandler
 *
 * Description:
 *   The client code must call this function periodically to process
 *   incoming messages from the server.  If CONFIG_NX_BLOCKING is defined,
 *   then this function will never return until the host is disconnected.
 *
 * Input Parameters:
 *   handle - the handle returned by nx_connect
 *
 * Return:
 *   >0: The length of the message received in msgbuffer
 *    0: No message was received
 *   <0: An error occurred and errno has been set appropriately
 *
 *   Of particular interest, it will return errno == EHOSTDOWN when the
 *   server is disconnected.  After that event, the handle can not longer
 *   be used.
 *
 ****************************************************************************/

#ifdef CONFIG_NX_MULTIUSER
EXTERN int nx_eventhandler(NXHANDLE handle);
#endif

/****************************************************************************
 * Name: nx_openwindow
 *
 * Description:
 *   Create a new window.
 *
 * Input Parameters:
 *   handle - The handle returned by nx_connect
 *   cb     - Callbacks used to process window events
 *
 * Return:
 *   Success: A non-NULL handle used with subsequent NX accesses
 *   Failure:  NULL is returned and errno is set appropriately
 *
 ****************************************************************************/

EXTERN NXWINDOW nx_openwindow(NXHANDLE handle,
                              FAR const struct nx_callback_s *cb);

/****************************************************************************
 * Name: nx_closewindow
 *
 * Description:
 *   Destroy a window created by nx_openwindow.
 *
 * Input Parameters:
 *   wnd - The window to be destroyed
 *
 * Return:
 *   OK on success; ERROR on failure with errno set appropriately
 *
 ****************************************************************************/

EXTERN int nx_closewindow(NXWINDOW hwnd);

/****************************************************************************
 * Name: nx_getposition
 *
 * Description:
 *  Request the position and size information for the selected window.  The
 *  values will be return asynchronously through the client callback function
 *  pointer.
 *
 * Input Parameters:
 *   hwnd   - The window handle
 *
 * Return:
 *   OK on success; ERROR on failure with errno set appropriately
 *
 ****************************************************************************/

EXTERN int nx_getposition(NXWINDOW hwnd);

/****************************************************************************
 * Name: nx_setposition
 *
 * Description:
 *  Set the position and size for the selected window
 *
 * Input Parameters:
 *   hwnd  - The window handle
 *   pos   - The new position of the window
 *
 * Return:
 *   OK on success; ERROR on failure with errno set appropriately
 *
 ****************************************************************************/

EXTERN int nx_setposition(NXWINDOW hwnd, FAR struct nxgl_point_s *pos);

/****************************************************************************
 * Name: nx_setsize
 *
 * Description:
 *  Set the size of the selected window
 *
 * Input Parameters:
 *   hwnd   - The window handle
 *   size   - The new size of the window.
 *
 * Return:
 *   OK on success; ERROR on failure with errno set appropriately
 *
 ****************************************************************************/

EXTERN int nx_setsize(NXWINDOW hwnd, FAR struct nxgl_rect_s *size);

/****************************************************************************
 * Name: nx_raise
 *
 * Description:
 *   Bring the specified window to the top of the display.
 *
 * Input parameters:
 *   hwnd - the window to be raised
 *
 * Returned value:
 *   OK on success; ERROR on failure with errno set appropriately
 *
 ****************************************************************************/

EXTERN int nx_raise(NXWINDOW hwnd);

/****************************************************************************
 * Name: nx_raise
 *
 * Description:
 *   Lower the specified window to the bottom of the display.
 *
 * Input parameters:
 *   hwnd - the window to be lowered
 *
 * Returned value:
 *   OK on success; ERROR on failure with errno set appropriately
 *
 ****************************************************************************/

EXTERN int nx_lower(NXWINDOW hwnd);

/****************************************************************************
 * Name: nx_fill
 *
 * Description:
 *  Fill the specified rectangle in the window with the specified color
 *
 * Input Parameters:
 *   hwnd  - The window handle
 *   rect  - The location to be filled
 *   color - The color to use in the fill
 *
 * Return:
 *   OK on success; ERROR on failure with errno set appropriately
 *
 ****************************************************************************/

EXTERN int nx_fill(NXWINDOW hwnd, FAR const struct nxgl_rect_s *rect,
                   nxgl_mxpixel_t color[CONFIG_NX_NPLANES]);

/****************************************************************************
 * Name: nx_setbgcolor
 *
 * Description:
 *  Set the color of the background
 *
 * Input Parameters:
 *   handle  - The connection handle
 *   color - The color to use in the background
 *
 * Return:
 *   OK on success; ERROR on failure with errno set appropriately
 *
 ****************************************************************************/

int nx_setbgcolor(NXHANDLE handle,
                  nxgl_mxpixel_t color[CONFIG_NX_NPLANES]);

/****************************************************************************
 * Name: nx_move
 *
 * Description:
 *   Move a rectangular region within the window
 *
 * Input Parameters:
 *   hwnd   - The window within which the move is to be done
 *   rect   - Describes the rectangular region to move
 *   offset - The offset to move the region
 *
 * Return:
 *   OK on success; ERROR on failure with errno set appropriately
 *
 ****************************************************************************/

EXTERN int nx_move(NXWINDOW hwnd, FAR const struct nxgl_rect_s *rect,
                   FAR const struct nxgl_point_s *offset);

/****************************************************************************
 * Name: nx_bitmap
 *
 * Description:
 *   Copy a rectangular region of a larger image into the rectangle in the
 *   specified window.
 *
 * Input Parameters:
 *   hwnd   - The window that will receive the bitmap image
 *   dest   - Describes the rectangular on the display that will receive the
 *            the bit map.
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

EXTERN int nx_bitmap(NXWINDOW hwnd, FAR const struct nxgl_rect_s *dest,
                     FAR const void *src[CONFIG_NX_NPLANES],
                     FAR const struct nxgl_point_s *origin,
                     unsigned int stride);

/****************************************************************************
 * Name: nx_kbdin
 *
 * Description:
 *   Used by a thread or interrupt handler that manages some kind of keypad
 *   hardware to report text information to the NX server.  That text
 *   data will be routed by the NX server to the appropriate window client.
 *
 ****************************************************************************/

#ifdef CONFIG_NX_KBD
EXTERN int nx_kbdchin(NXHANDLE handle, ubyte ch);
EXTERN int nx_kbdin(NXHANDLE handle, ubyte nch const char *ch);
#endif

/****************************************************************************
 * Name: nx_mousein
 *
 * Description:
 *   Used by a thread or interrupt handler that manages some kind of pointing
 *   hardware to report new positional data to the NX server.  That positional
 *   data will be routed by the NX server to the appropriate window client.
 *
 ****************************************************************************/

#ifdef CONFIG_NX_MOUSE
EXTERN int nx_mousein(NXHANDLE handle, nxgl_coord_t x, nxgl_coord_t y, ubyte buttons);
#endif

#undef EXTERN
#if defined(__cplusplus)
}
#endif

#endif /* _INCLUDE_NUTTX_NX_H */

