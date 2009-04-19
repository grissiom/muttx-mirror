/****************************************************************************
 * graphics/nxsu/nxfe.h
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

#ifndef __GRAPHICS_NXSU_NXFE_H
#define __GRAPHICS_NXSU_NXFE_H

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

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* Server state structure ***************************************************/

/* This the the server 'front-end' state structure */

struct nxfe_state_s
{
  /* The 'back-end' window status.  Must be first so that instances of
   * struct nxbe_state_s can be simply cast to an instance of struct
   * nxfe_state_s
   */

  struct nxbe_state_s be;

  /* Event handling callbacks */

  FAR const struct nx_callback_s *cb; /* Message handling callbacks */
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
 * Name: nxfe_redrawreq
 *
 * Descripton:
 *   Request the client that has this window to redraw the rectangular region.
 *
 ****************************************************************************/

EXTERN void nxfe_redrawreq(FAR struct nxbe_window_s *wnd,
                           FAR const struct nxgl_rect_s *rect);

/****************************************************************************
 * Name: nxfe_reportposition
 *
 * Descripton:
 *   Report the new size/position of the window.
 *
 ****************************************************************************/

EXTERN void nxfe_reportposition(FAR struct nxbe_window_s *wnd);

/****************************************************************************
 * Name: nxmu_mouseinit
 *
 * Description:
 *   Initialize with the mouse in the center of the display
 *
 ****************************************************************************/

#ifdef CONFIG_NX_MOUSE
EXTERN void nxsu_mouseinit(int x, int y);
#endif

/****************************************************************************
 * Name: nxmu_mousereport
 *
 * Description:
 *   Report mouse position info to the specified window
 *
 ****************************************************************************/

#ifdef CONFIG_NX_MOUSE
EXTERN void nxsu_mousereport(struct nxbe_window_s *wnd);
#endif

#undef EXTERN
#if defined(__cplusplus)
}
#endif

#endif  /* __GRAPHICS_NSMU_NXFE_H */

