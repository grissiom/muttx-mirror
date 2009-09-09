/****************************************************************************
 * net/uip/uip_tcpcallback.c
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#if defined(CONFIG_NET) && defined(CONFIG_NET_TCP)

#include <sys/types.h>
#include <string.h>
#include <debug.h>

#include <net/uip/uipopt.h>
#include <net/uip/uip.h>
#include <net/uip/uip-arch.h>

#include "uip_internal.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Function: uip_readahead
 *
 * Description:
 *   Copy as much received data as possible into the readahead buffer
 *
 * Assumptions:
 *   This function is called at the interrupt level with interrupts disabled.
 *
 ****************************************************************************/

#if CONFIG_NET_NTCP_READAHEAD_BUFFERS > 0
static int uip_readahead(struct uip_readahead_s *readahead, uint8 *buf, int len)
{
  int available = CONFIG_NET_TCP_READAHEAD_BUFSIZE - readahead->rh_nbytes;
  int recvlen   = 0;

  if (len > 0 && available > 0)
    {
      /* Get the length of the data to buffer. */

      if (len > available)
        {
          recvlen = available;
        }
      else
        {
          recvlen = len;
        }

      /* Copy the new appdata into the read-ahead buffer */

      memcpy(&readahead->rh_buffer[readahead->rh_nbytes], buf, recvlen);
      readahead->rh_nbytes += recvlen;
    }
  return recvlen;
}
#endif

/****************************************************************************
 * Function: uip_dataevent
 *
 * Description:
 *   This is the default data event handler that is called when there is no
 *   user data handler in place
 *
 * Assumptions:
 * - The called has checked that UIP_NEWDATA is set in flags and that is no
 *   other handler available to process the incoming data.
 * - This function is called at the interrupt level with interrupts disabled.
 *
 ****************************************************************************/

static inline uint16
uip_dataevent(struct uip_driver_s *dev, struct uip_conn *conn, uint16 flags)
{
  uint16 ret;

  /* Assume that we will ACK the data.  The data will be ACKed if it is
   * placed in the read-ahead buffer -OR- if it zero length
   */

  ret = (flags & ~UIP_NEWDATA) | UIP_SNDACK;

  /* Is there new data?  With non-zero length?  (Certain connection events
   * can have zero-length with UIP_NEWDATA set just to cause an ACK).
   */

  if (dev->d_len > 0)
    {
#if CONFIG_NET_NTCP_READAHEAD_BUFFERS > 0
      struct uip_readahead_s *readahead1;
      struct uip_readahead_s *readahead2 = NULL;
      uint16 recvlen = 0;
      uint8 *buf     = dev->d_appdata;
      int    buflen  = dev->d_len;
#endif

      nvdbg("No listener on connection\n");

#if CONFIG_NET_NTCP_READAHEAD_BUFFERS > 0
      /* First, we need to determine if we have space to buffer the data.  This
       * needs to be verified before we actually begin buffering the data. We
       * will use any remaining space in the last allocated readahead buffer
       * plus as much one additional buffer.  It is expected that the size of
       * readahead buffers are tuned so that one full packet will always fit
       * into one readahead buffer (for example if the buffer size is 420, then
       * a readahead buffer of 366 will hold a full packet of TCP data).
       */

      readahead1 = (struct uip_readahead_s*)conn->readahead.tail;
      if ((readahead1 &&
          (CONFIG_NET_TCP_READAHEAD_BUFSIZE - readahead1->rh_nbytes) > buflen) ||
          (readahead2 = uip_tcpreadaheadalloc()) != NULL)
        {
          /* We have buffer space.  Now try to append add as much data as possible
           * to the last readahead buffer attached to this connection.
           */

          if (readahead1)
            {
              recvlen = uip_readahead(readahead1, buf, buflen);
              if (recvlen > 0)
                {
                  buf    += recvlen;
                  buflen -= recvlen;
                }
            }

          /* Do we need to buffer into the newly allocated buffer as well? */

          if (readahead2)
            {
              readahead2->rh_nbytes = 0;
              (void)uip_readahead(readahead2, buf, buflen);

              /* Save the readahead buffer in the connection structure where
               * it can be found with recv() is called.
               */

              sq_addlast(&readahead2->rh_node, &conn->readahead);
            }

          nvdbg("Buffered %d bytes\n", dev->d_len);
        }
      else
#endif
        {
          /* There is no handler to receive new data and there are no free
           * read-ahead buffers to retain the data -- drop the packet.
           */

         nvdbg("Dropped %d bytes\n", dev->d_len);

 #ifdef CONFIG_NET_STATISTICS
          uip_stat.tcp.syndrop++;
          uip_stat.tcp.drop++;
#endif
          /* Clear the UIP_SNDACK bit so that no ACK will be sent */

          ret &= ~UIP_SNDACK;
        }
    }

  /* In any event, the new data has now been handled */

  dev->d_len = 0;
  return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Function: uip_tcpcallback
 *
 * Description:
 *   Inform the application holding the TCP socket of a change in state.
 *
 * Assumptions:
 *   This function is called at the interrupt level with interrupts disabled.
 *
 ****************************************************************************/

uint16 uip_tcpcallback(struct uip_driver_s *dev, struct uip_conn *conn, uint16 flags)
{
  /* Preserve the UIP_ACKDATA, UIP_CLOSE, and UIP_ABORT in the response.
   * These is needed by uIP to handle responses and buffer state.  The
   * UIP_NEWDATA indication will trigger the ACK response, but must be
   * explicitly set in the callback.
   */

  uint16 ret = flags;

  nvdbg("flags: %04x\n", flags);

  /* Perform the data callback.  When a data callback is executed from 'list',
   * the input flags are normally returned, however, the implementation
   * may set one of the following:
   *
   *   UIP_CLOSE   - Gracefully close the current connection
   *   UIP_ABORT   - Abort (reset) the current connection on an error that
   *                 prevents UIP_CLOSE from working.
   *
   * And/Or set/clear the following:
   *
   *   UIP_NEWDATA - May be cleared to indicate that the data was consumed
   *                 and that no further process of the new data should be
   *                 attempted.
   *   UIP_SNDACK  - If UIP_NEWDATA is cleared, then UIP_SNDACK may be set
   *                 to indicate that an ACK should be included in the response.
   *                 (In UIP_NEWDATA is cleared bu UIP_SNDACK is not set, then
   *                 dev->d_len should also be cleared).
   */

  ret = uip_callbackexecute(dev, conn, flags, conn->list);

  /* There may be no new data handler in place at them moment that the new
   * incoming data is received.  If the new incoming data was not handled, then
   * either (1) put the unhandled incoming data in the read-ahead buffer (if
   * enabled) or (2) suppress the ACK to the data in the hope that it will
   * be re-transmitted at a better time.
   */

  if ((ret & UIP_NEWDATA) != 0)
    {
      /* Data was not handled.. dispose of it appropriately */

      ret = uip_dataevent(dev, conn, ret);
    }

  /* Check if there is a connection-related event and a connection
   * callback.
   */

  if (((flags & UIP_CONN_EVENTS) != 0) && conn->connection_event)
    {
      /* Perform the callback */

      conn->connection_event(conn, flags);
    }

  return ret;
}

#endif /* CONFIG_NET && CONFIG_NET_TCP */
