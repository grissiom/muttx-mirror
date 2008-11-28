/****************************************************************************
 * graphics/nxmu/nxfe.h
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

#ifndef __GRAPHICS_NXMU_NXFE_H
#define __GRAPHICS_NXMU_NXFE_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/types.h>
#include <mqueue.h>
#include <semaphore.h>

#include <nuttx/fb.h>
#include <nuttx/nx.h>

#include "nxbe.h"

/****************************************************************************
 * Pre-Processor Definitions
 ****************************************************************************/

/* Configuration ************************************************************/

#ifndef CONFIG_NX_MXSERVERMSGS
#  define CONFIG_NX_MXSERVERMSGS 32 /* Number of pending messages in server MQ */
#endif

#ifndef CONFIG_NX_MXCLIENTMSGS
#  define CONFIG_NX_MXCLIENTMSGS 16 /* Number of pending messages in each client MQ */
#endif

/* Used to create unique client MQ name */

#define NX_CLIENT_MQNAMEFMT  "/dev/nxc%d"
#define NX_CLIENT_MXNAMELEN  (16)

#define NX_MXSVRMSGLEN       (64) /* Maximum size of a client->server command */
#define NX_MXEVENTLEN        (64) /* Maximum size of an event */
#define NX_MXCLIMSGLEN       (64) /* Maximum size of a server->client message */

/* Handy macros */

#define nxmu_semgive(sem)    sem_post(sem) /* To match nxmu_semtake() */

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* Client/Connection structures *********************************************/

/* Client state */

enum nx_clistate_e
{
  NX_CLISTATE_NOTCONNECTED = 0,   /* Waiting for server to acknowledge connection */
  NX_CLISTATE_CONNECTED,          /* Connection established (normal state) */
  NX_CLISTATE_DISCONNECT_PENDING, /* Waiting for server to acknowledge disconnect */
};

/* This structure represents a connection between the client and the server */

struct nxfe_conn_s
{
  /* This number uniquely identifies the client */

  int   cid;                          /* Client ID (CID) */
  ubyte state;                        /* See enum nx_clistate_e */

  /* These are only usable on the client side of the connection */

  mqd_t crdmq;                        /* MQ to read from the server (may be non-blocking) */
  mqd_t cwrmq;                        /* MQ to write to the server (blocking) */
  FAR const struct nx_callback_s *cb; /* Message handling callbacks */

  /* These are only usable on the server side of the connection */

  mqd_t swrmq;                        /* MQ to write to the client */
};

/* Server state structure ***************************************************/

/* This the the server 'front-end' state structure */

struct nxfe_state_s
{
  /* The 'back-end' window status.  Must be first so that instances of
   * struct nxbe_state_s can be simply cast to an instance of struct
   * nxfe_state_s
   */

  struct nxbe_state_s be;

  /* This is the server's connection to iself */

  struct nxfe_conn_s conn;
};

/* Message IDs **************************************************************/

enum nxmsg_e
{
  /* Server-to-Client Messages **********************************************/

  NX_CLIMSG_CONNECTED = 1,    /* The server has completed the connection and is ready */
  NX_CLIMSG_DISCONNECTED,     /* The server has disconnected */
  NX_CLIMSG_REDRAW,           /* Re-draw the specified window */
  NX_CLIMSG_NEWPOSITION,      /* New window size/position */
  NX_CLIMSG_MOUSEIN,          /* New mouse positional data available for window */
  NX_CLIMSG_KBDIN,            /* New keypad input available for window */

  /* Client-to-Server Messages **********************************************/

  NX_SVRMSG_CONNECT,          /* Establish connection with new NX server client */
  NX_SVRMSG_DISCONNECT,       /* Tear down connection with terminating client */
  NX_SVRMSG_OPENWINDOW,       /* Create a new window */
  NX_SVRMSG_CLOSEWINDOW,      /* Close an existing window */
  NX_SVRMSG_SETPOSITION,      /* Window position has changed */
  NX_SVRMSG_SETSIZE,          /* Window size has changed */
  NX_SVRMSG_GETPOSITION,      /* Get the current window position and size */
  NX_SVRMSG_RAISE,            /* Move the window to the top */
  NX_SVRMSG_LOWER,            /* Move the window to the bottom */
  NX_SVRMSG_FILL,             /* Fill a rectangle in the window with a color */
  NX_SVRMSG_FILLTRAP,         /* Fill a trapezoidal region in the window with a color */
  NX_SVRMSG_MOVE,             /* Move a rectangular region within the window */
  NX_SVRMSG_BITMAP,           /* Copy a rectangular bitmap into the window */
  NX_SVRMSG_SETBGCOLOR,       /* Set the color of the background */
  NX_SVRMSG_MOUSEIN,          /* New mouse report from mouse client */
  NX_SVRMSG_KBDIN,            /* New keyboard report from keyboard client */
};

/* Message priorities -- they must all be at the same priority to assure
 * FIFO execution.
 */

#define NX_CLIMSG_PRIO 42
#define NX_SVRMSG_PRIO 42

/* Server-to-Client Message Structures **************************************/

/* The generic message structure.  All messages begin with this form.  Also, messages
 * that have no data other than the msgid event use this structure.  This includes:
 * NX_CLIMSG_CONNECTED and NX_CLIMSG_DISCONNECTED.
 */

struct nxclimsg_s
{
  uint32 msgid;                  /* Any of nxclimsg_e */
};

/* This message is received when a requested window has been opened.  If wnd is NULL
 * then errorcode is the errno value that provides the explanation of the error.
 */

struct nxclimsg_redraw_s
{
  uint32 msgid;                  /* NX_CLIMSG_REDRAW */
  FAR struct nxbe_window_s *wnd; /* The handle to the window to redraw in */
  FAR struct nxgl_rect_s rect;   /* The rectangle to be redrawn */
  boolean more;                  /* TRUE: more redraw messages follow */
};

/* This message informs the client of the new size or position of the window  */

struct nxclimsg_newposition_s
{
  uint32 msgid;                  /* NX_CLIMSG_NEWPOSITION */
  FAR struct nxbe_window_s *wnd; /* The window whose position/size has changed */
  FAR struct nxgl_rect_s size;   /* The current window size */
  FAR struct nxgl_point_s pos;   /* The current window position */
};

/* This message reports a new mouse event to a particular window */

#ifdef CONFIG_NX_MOUSE
struct nxclimsg_mousein_s
{
  uint32 msgid;                  /* NX_SVRMSG_MOUSEIN */
  FAR struct nxbe_window_s *wnd; /* The handle of window receiving mouse input */
  struct nxgl_point_s pos;       /* Mouse X/Y position */
  ubyte buttons;                 /* Mouse button set */
}
#endif

/* This message reports a new keypad event to a particular window */

#ifdef CONFIG_NX_KBD
struct nxclimsg_key
{
  uint32 msgid;                  /* NX_CLIMSG_KBDIN */
  FAR struct nxbe_window_s *wnd; /* The handle of window receiving keypad input */
  ubyte nch                      /* Number of characters received */
  ubyte ch[1];                   /* Array of received characters */
};
#endif

/* Client-to-Server Message Structures **************************************/

/* The generic message structure.  All server messages begin with this form.  Also
 * messages that have no additional data fields use this structure.  This includes:
 * NX_SVRMSG_CONNECT and NX_SVRMSG_DISCONNECT.
 */

struct nxsvrmsg_s                    /* Generic server message */
{
  uint32 msgid;                      /* One of enum nxsvrmsg_e */
  FAR struct nxfe_conn_s *conn;      /* The specific connection sending the message */
};

/* This message requests the server to create a new window */

struct nxsvrmsg_openwindow_s
{
  uint32 msgid;                      /* NX_SVRMSG_OPENWINDOW */
  FAR struct nxfe_conn_s *conn;      /* The specific connection sending the message */
  FAR struct nxbe_window_s *wnd;     /* The pre-allocated window structure */
};

/* This message informs the server that client wishes to close a window */

struct nxsvrmsg_closewindow_s
{
  uint32 msgid;                      /* NX_SVRMSG_CLOSEWINDOW */
  FAR struct nxbe_window_s *wnd;     /* The window to be closed */
};

/* This message informs the server that the size or position of the window has changed */

struct nxsvrmsg_setposition_s
{
  uint32 msgid;                      /* NX_SVRMSG_SETPOSITION */
  FAR struct nxbe_window_s *wnd;     /* The window whose position/size has changed */
  FAR struct nxgl_point_s pos;       /* The new window position */
};

/* This message informs the server that the size or position of the window has changed */

struct nxsvrmsg_setsize_s
{
  uint32 msgid;                      /* NX_SVRMSG_SETSIZE */
  FAR struct nxbe_window_s *wnd;     /* The window whose position/size has changed */
  FAR struct nxgl_rect_s  size;      /* The new window size */
};

/* This message informs the server that the size or position of the window has changed */

struct nxsvrmsg_getposition_s
{
  uint32 msgid;                      /* NX_SVRMSG_FETPOSITION */
  FAR struct nxbe_window_s *wnd;     /* The window whose position/size has changed */
};

/* This message informs the server to raise this window to the top of the display */

struct nxsvrmsg_raise_s
{
  uint32 msgid;                      /* NX_SVRMSG_RAISE */
  FAR struct nxbe_window_s *wnd;     /* The window to be raised */
};

/* This message informs the server to lower this window to the bottom of the display */

struct nxsvrmsg_lower_s
{
  uint32 msgid;                      /* NX_SVRMSG_LOWER */
  FAR struct nxbe_window_s *wnd;     /* The window to be lowered  */
};

/* Fill a rectangle in the window with a color */

struct nxsvrmsg_fill_s
{
  uint32  msgid;                     /* NX_SVRMSG_FILL */
  FAR struct nxbe_window_s *wnd;     /* The window to fill  */
  struct nxgl_rect_s rect;           /* The rectangle in the window to fill */
  nxgl_mxpixel_t color[CONFIG_NX_NPLANES]; /* Color to use in the fill */
};

/* Fill a trapezoidal region in the window with a color */

struct nxsvrmsg_filltrapezoid_s
{
  uint32  msgid;                     /* NX_SVRMSG_FILLTRAP */
  FAR struct nxbe_window_s *wnd;     /* The window to fill  */
  struct nxgl_trapezoid_s trap;      /* The trapezoidal region in the window to fill */
  nxgl_mxpixel_t color[CONFIG_NX_NPLANES]; /* Color to use in the fill */
};

/* Move a rectangular region within the window */

struct nxsvrmsg_move_s
{
  uint32 msgid;                      /* NX_SVRMSG_MOVE */
  FAR struct nxbe_window_s *wnd;     /* The window within which the move is done  */
  struct nxgl_rect_s rect;           /* Describes the rectangular region to move */
  struct nxgl_point_s offset;        /* The offset to move the region */
};

/* Copy a rectangular bitmap into the window */

struct nxsvrmsg_bitmap_s
{
  uint32 msgid;                     /* NX_SVRMSG_BITMAP */
  FAR struct nxbe_window_s *wnd;    /* The window with will receive the bitmap image  */
  struct nxgl_rect_s dest;          /* Destination location of the bitmap in the window */
  FAR const void *src[CONFIG_NX_NPLANES]; /* The start of the source image. */
  struct nxgl_point_s origin;       /* Offset into the source image data */
  unsigned int stride;              /* The width of the full source image in pixels. */
};

/* Set the color of the background */

struct nxsvrmsg_setbgcolor_s
{
  uint32  msgid;                     /* NX_SVRMSG_SETBGCOLOR */
  nxgl_mxpixel_t color[CONFIG_NX_NPLANES]; /* Color to use in the background */
};

/* This message reports a new mouse event from a hardware controller attached to
 * the server as a regular client (this message may have even been sent from an
 * interrupt handler).
 */

#ifdef CONFIG_NX_MOUSE
struct nxsvrmsg_mousein_s
{
  uint32 msgid;                      /* NX_SVRMSG_MOUSEIN */
  struct nx_point_x pt;              /* Mouse X/Y position */
  ubyte buttons;                     /* Mouse button set */
}
#endif

/* This message reports a new keyboard event from a hardware controller attached to
 * some kind of keypad (this message may have even been sent from an
 * interrupt handler).
 */

#ifdef CONFIG_NX_KBD
struct nxsvrmsg_kbdin_s
{
  uint32     msgid;                  /* NX_SVRMSG_KBDIN */
  ubyte      nch                     /* Number of characters received */
  ubyte      ch[1];                  /* Array of received characters */
}
#endif

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
 * Name: nxmu_semtake
 *
 * Description:
 *   Take the semaphore, handling EINTR wakeups.  See the nxmu_semgive macro.
 *
 * Input Parameters:
 *   sem - the semaphore to be taken.
 *
 * Return:
 *   None
 *
 ****************************************************************************/

EXTERN void nxmu_semtake(sem_t *sem);

/****************************************************************************
 * Name: nxmu_openwindow
 *
 * Description:
 *   Create a new window.
 *
 * Input Parameters:
 *   conn - The client containing connection information [IN]
 *   svr  - The server state structure [IN]
 *   wnd  - The pre-allocated window structure to be ininitilized [IN/OUT]
 *
 * Return:
 *   None
 *
 ****************************************************************************/

EXTERN void nxmu_openwindow(FAR struct nxfe_conn_s *conn,
                            FAR struct nxbe_state_s *be,
                            FAR struct nxbe_window_s *wnd);

/****************************************************************************
 * Name: nxfe_reportposition
 *
 * Descripton:
 *   Report the new size/position of the window.
 *
 ****************************************************************************/

EXTERN void nxfe_reportposition(FAR struct nxbe_window_s *wnd);

/****************************************************************************
 * Name: nxfe_redrawreq
 *
 * Descripton:
 *   Request the client that has this window to redraw the rectangular region.
 *
 ****************************************************************************/

EXTERN void nxfe_redrawreq(FAR struct nxbe_window_s *wnd,
                           FAR const struct nxgl_rect_s *rect);

/****************************************************************************
 * Name: nxmu_mouseinit
 *
 * Description:
 *   Initialize with the mouse in the center of the display
 *
 ****************************************************************************/

#ifdef CONFIG_NX_MOUSE
EXTERN void nxmu_mouseinit(int x, int y);
#endif

/****************************************************************************
 * Name: nxmu_mousereport
 *
 * Description:
 *   Report mouse position info to the specified window
 *
 ****************************************************************************/

#ifdef CONFIG_NX_MOUSE
EXTERN void nxmu_mousereport(struct nxbe_window_s *wnd);
#endif

/****************************************************************************
 * Name: nxmu_mousein
 *
 * Description:
 *   New positional data has been received from the thread or interrupt
 *   handler that manages some kind of pointing hardware.  Route that
 *   positional data to the appropriate window client.
 *
 ****************************************************************************/

#ifdef CONFIG_NX_MOUSE
EXTERN nxmu_mousein(FAR struct nxfe_state_s *fe,
                    FAR const struct nxgl_point_s *pos, int button);
#endif

/****************************************************************************
 * Name: nxmu_kbdin
 *
 * Descripton:
 *   New keyboard data has been received from the thread or interrupt
 *   handler that manages some kind of keyboard/keypad hardware.  Route that
 *   positional data to the appropriate window client.
 *
 ****************************************************************************/

#ifdef CONFIG_NX_KBD
EXTERN void nxmu_kbdin(FAR struct nxs_server_s *svr, ubyte nch, ubyte *ch);
#endif

#undef EXTERN
#if defined(__cplusplus)
}
#endif

#endif  /* __GRAPHICS_NXMU_NXFE_H */

