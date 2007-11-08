/****************************************************************************
 * net/uip/uip-poll.c
 *
 *   Copyright (C) 2007 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <spudmonkey@racsa.co.cr>
 *
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
#ifdef CONFIG_NET

#include <sys/types.h>
#include <debug.h>

#include <net/uip/uipopt.h>
#include <net/uip/uip.h>
#include <net/uip/uip-arch.h>

#include "uip-internal.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Function: uip_polludpconnections
 *
 * Description:
 *   Poll all UDP connections for available packets to send.
 *
 * Assumptions:
 *   This function is called from the CAN device driver and may be called from
 *   the timer interrupt/watchdog handle level.
 *
 ****************************************************************************/

#ifdef CONFIG_NET_UDP
static int uip_polludpconnections(struct uip_driver_s *dev,
                                  uip_poll_callback_t callback)
{
  struct uip_udp_conn *udp_conn = NULL;
  int                  bstop    = 0;

  /* Traverse all of the allocated UDP connections and perform the poll action */

  while (!bstop && (udp_conn = uip_nextudpconn(udp_conn)))
    {
      /* Perform the UDP TX poll */

      uip_udppoll(dev, udp_conn);

      /* Call back into the driver */

      bstop = callback(dev);
    }

  return bstop;
}
#else
# define uip_polludpconnections(dev,callback) (0)
#endif /* CONFIG_NET_UDP */

/****************************************************************************
 * Function: uip_polltcpconnections
 *
 * Description:
 *   Poll all UDP connections for available packets to send.
 *
 * Assumptions:
 *   This function is called from the CAN device driver and may be called from
 *   the timer interrupt/watchdog handle level.
 *
 ****************************************************************************/

static inline int uip_polltcpconnections(struct uip_driver_s *dev,
                                         uip_poll_callback_t callback)
{
  struct uip_conn *conn  = NULL;
  int              bstop = 0;

  /* Traverse all of the active TCP connections and perform the poll action */

  while (!bstop && (conn = uip_nexttcpconn(conn)))
    {
      /* Perform the TCP TX poll */

      uip_tcppoll(dev, conn);

      /* Call back into the driver */

      bstop = callback(dev);
    }

  return bstop;
}

/****************************************************************************
 * Function: uip_polltcptimer
 *
 * Description:
 *   The TCP timer has expired.  Update TCP timing state in each active,
 *   TCP connection.
 *
 * Assumptions:
 *   This function is called from the CAN device driver and may be called from
 *   the timer interrupt/watchdog handle level.
 *
 ****************************************************************************/

static inline int uip_polltcptimer(struct uip_driver_s *dev,
                                   uip_poll_callback_t callback, int hsec)
{
  struct uip_conn *conn  = NULL;
  int              bstop = 0;

  /* Traverse all of the active TCP connections and perform the poll action */

  while (!bstop && (conn = uip_nexttcpconn(conn)))
    {
      /* Perform the TCP timer poll */

      uip_tcptimer(dev, conn, hsec);

      /* Call back into the driver */

      bstop = callback(dev);
    }

  return bstop;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Function: uip_poll
 *
 * Description:
 *   This function will traverse each active uIP connection structure and
 *   will perform TCP and UDP polling operations. uip_poll() may be called
 *   asychronously with the network drvier can accept another outgoing packet.
 *
 *   This function will call the provided callback function for every active
 *   connection. Polling will continue until all connections have been polled
 *   or until the user-suplied function returns a non-zero value (which it
 *   should do only if it cannot accept further write data).
 *
 *   When the callback function is called, there may be an outbound packet
 *   waiting for service in the uIP packet buffer, and if so the d_len field
 *   is set to a value larger than zero. The device driver should then send
 *   out the packet.
 *
 * Assumptions:
 *   This function is called from the CAN device driver and may be called from
 *   the timer interrupt/watchdog handle level.
 *
 ****************************************************************************/

int uip_poll(struct uip_driver_s *dev, uip_poll_callback_t callback)
{
  int bstop;

  /* Traverse all of the active TCP connections and perform the poll action */

  bstop = uip_polltcpconnections(dev, callback);
  if (!bstop)
    {
      /* Traverse all of the allocated UDP connections and perform the poll action */

      bstop = uip_polludpconnections(dev, callback);
  }

  return bstop;
}

/****************************************************************************
 * Function: uip_timer
 *
 * Description:
 *   These function will traverse each active uIP connection structure and
 *   perform TCP timer operations (and UDP polling operations). The CAN
 *   driver MUST implement logic to periodically call uip_timer().
 *
 *   This function will call the provided callback function for every active
 *   connection. Polling will continue until all connections have been polled
 *   or until the user-suplied function returns a non-zero value (which it
 *   should do only if it cannot accept further write data).
 *
 *   When the callback function is called, there may be an outbound packet
 *   waiting for service in the uIP packet buffer, and if so the d_len field
 *   is set to a value larger than zero. The device driver should then send
 *   out the packet.
 *
 * Assumptions:
 *   This function is called from the CAN device driver and may be called from
 *   the timer interrupt/watchdog handle level.
 *
 ****************************************************************************/

int uip_timer(struct uip_driver_s *dev, uip_poll_callback_t callback, int hsec)
{
  int bstop;

  /* Increment the timer used by the IP reassembly logic */

#if UIP_REASSEMBLY
  if (uip_reasstmr != 0 && uip_reasstmr < UIP_REASS_MAXAGE)
    {
      uip_reasstmr += hsec;
    }
#endif /* UIP_REASSEMBLY */

  /* Traverse all of the active TCP connections and perform the timer action */

  bstop = uip_polltcptimer(dev, callback, hsec);
  if (!bstop)
    {
      /* Traverse all of the allocated UDP connections and perform the poll action */

      bstop = uip_polludpconnections(dev, callback);
    }

  return bstop;
}

#endif /* CONFIG_NET */
