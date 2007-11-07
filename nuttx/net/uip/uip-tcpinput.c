/****************************************************************************
 * net/uip/uip-tcpinput.c
 * Handling incoming TCP input
 *
 *   Copyright (C) 2007 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <spudmonkey@racsa.co.cr>
 *
 * Adapted for NuttX from logic in uIP which also has a BSD-like license:
 *
 *   Original author Adam Dunkels <adam@dunkels.com>
 *   Copyright () 2001-2003, Adam Dunkels.
 *   All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
 * Definitions
 ****************************************************************************/

#define BUF ((struct uip_tcpip_hdr *)&dev->d_buf[UIP_LLH_LEN])

/****************************************************************************
 * Public Variables
 ****************************************************************************/

/****************************************************************************
 * Private Variables
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: uip_tcpinput
 *
 * Description:
 *   Handle incoming TCP input
 *
 * Parameters:
 *   dev - The device driver structure containing the received TCP packet.
 *
 * Return:
 *   None
 *
 * Assumptions:
 *   Called from the interrupt level or with interrupts disabled.
 *
 ****************************************************************************/

void uip_tcpinput(struct uip_driver_s *dev)
{
  register struct uip_conn *uip_connr = uip_conn;
  uint16 tmp16;
  uint8  opt;
  int    len;
  int    i;

#ifdef CONFIG_NET_STATISTICS
  uip_stat.tcp.recv++;
#endif

  /* Start of TCP input header processing code. */

  if (uip_tcpchksum(dev) != 0xffff)
    {
      /* Compute and check the TCP checksum. */

#ifdef CONFIG_NET_STATISTICS
      uip_stat.tcp.drop++;
      uip_stat.tcp.chkerr++;
#endif
      dbg("Bad TCP checksum\n");
      goto drop;
    }

  /* Demultiplex this segment. First check any active connections. */

  uip_connr = uip_tcpactive(BUF);
  if (uip_connr)
    {
      goto found;
    }

  /* If we didn't find and active connection that expected the packet,
   * either (1) this packet is an old duplicate, or (2) this is a SYN packet
   * destined for a connection in LISTEN. If the SYN flag isn't set,
   * it is an old packet and we send a RST.
   */

  if ((BUF->flags & TCP_CTL) == TCP_SYN)
    {
        /* This is a SYN packet for a connection.  Find the connection
         * listening on this port.
         */

        tmp16 = BUF->destport;
        if (uip_islistener(tmp16))
          {
            /* We matched the incoming packet with a connection in LISTEN.
             * We now need to create a new connection and send a SYNACK in
             * response.
             */

          /* First allocate a new connection structure and see if there is any
           * user application to accept it.
           */

          uip_connr = uip_tcpaccept(BUF);
          if (uip_connr)
            {
              /* The connection structure was successfully allocated.  Now see
               * there is an application waiting to accept the connection (or at
               * least queue it it for acceptance).
               */

              if (uip_accept(uip_connr, tmp16) != OK)
                {
                  /* No, then we have to give the connection back */

                  uip_tcpfree(uip_connr);
                  uip_connr = NULL;
                }
            }

          if (!uip_connr)
            {
              /* Either (1) all available connections are in use, or (2) there is no
               * application in place to accept the connection.  We drop packet and hope that
               * the remote end will retransmit the packet at a time when we
               * have more spare connections or someone waiting to accept the connection.
               */

#ifdef CONFIG_NET_STATISTICS
              uip_stat.tcp.syndrop++;
#endif
              dbg("No free TCP connections\n");
              goto drop;
            }

          uip_incr32(uip_conn->rcv_nxt, 1);
          uip_conn = uip_connr;

          /* Parse the TCP MSS option, if present. */

          if ((BUF->tcpoffset & 0xf0) > 0x50)
            {
              for (i = 0; i < ((BUF->tcpoffset >> 4) - 5) << 2 ;)
                {
                  opt = dev->d_buf[UIP_TCPIP_HLEN + UIP_LLH_LEN + i];
                  if (opt == TCP_OPT_END)
                    {
                      /* End of options. */

                      break;
                    }
                  else if (opt == TCP_OPT_NOOP)
                    {
                      /* NOP option. */

                      ++i;
                    }
                  else if (opt == TCP_OPT_MSS &&
                          dev->d_buf[UIP_TCPIP_HLEN + UIP_LLH_LEN + 1 + i] == TCP_OPT_MSS_LEN)
                    {
                      /* An MSS option with the right option length. */

                      tmp16 = ((uint16)dev->d_buf[UIP_TCPIP_HLEN + UIP_LLH_LEN + 2 + i] << 8) |
                               (uint16)dev->d_buf[UIP_IPTCPH_LEN + UIP_LLH_LEN + 3 + i];
                      uip_connr->initialmss = uip_connr->mss =
                              tmp16 > UIP_TCP_MSS? UIP_TCP_MSS: tmp16;

                      /* And we are done processing options. */

                      break;
                    }
                  else
                    {
                      /* All other options have a length field, so that we easily
                       * can skip past them.
                       */

                      if (dev->d_buf[UIP_TCPIP_HLEN + UIP_LLH_LEN + 1 + i] == 0)
                        {
                          /* If the length field is zero, the options are malformed
                           * and we don't process them further.
                           */

                          break;
                        }
                      i += dev->d_buf[UIP_TCPIP_HLEN + UIP_LLH_LEN + 1 + i];
                    }
                }
            }

          /* Our response will be a SYNACK. */

          uip_tcpack(dev, uip_connr, TCP_ACK | TCP_SYN);
          goto done;
        }
    }

  /* This is (1) an old duplicate packet or (2) a SYN packet but with
   * no matching listener found.  Send RST packet in either case.
   */

  /* We do not send resets in response to resets. */

  if (BUF->flags & TCP_RST)
    {
      goto drop;
    }

#ifdef CONFIG_NET_STATISTICS
  uip_stat.tcp.synrst++;
#endif
  uip_tcpreset(dev);
  goto done;

found:

  uip_conn = uip_connr;
  uip_flags = 0;

  /* We do a very naive form of TCP reset processing; we just accept
   * any RST and kill our connection. We should in fact check if the
   * sequence number of this reset is wihtin our advertised window
   * before we accept the reset.
   */

  if (BUF->flags & TCP_RST)
    {
      uip_connr->tcpstateflags = UIP_CLOSED;
      dbg("Recvd reset - TCP state: UIP_CLOSED\n");

      uip_flags = UIP_ABORT;
      uip_tcpcallback(dev);
      goto drop;
    }

  /* Calculated the length of the data, if the application has sent
   * any data to us.
   */

  len = (BUF->tcpoffset >> 4) << 2;

  /* d_len will contain the length of the actual TCP data. This is
   * calculated by subtracing the length of the TCP header (in
   * len) and the length of the IP header (20 bytes).
   */

  dev->d_len -= (len + UIP_IPH_LEN);

  /* First, check if the sequence number of the incoming packet is
   * what we're expecting next. If not, we send out an ACK with the
   * correct numbers in.
   */

  if (!(((uip_connr->tcpstateflags & UIP_TS_MASK) == UIP_SYN_SENT) &&
      ((BUF->flags & TCP_CTL) == (TCP_SYN | TCP_ACK))))
    {
      if ((dev->d_len > 0 || ((BUF->flags & (TCP_SYN | TCP_FIN)) != 0)) &&
          (BUF->seqno[0] != uip_connr->rcv_nxt[0] ||
           BUF->seqno[1] != uip_connr->rcv_nxt[1] ||
           BUF->seqno[2] != uip_connr->rcv_nxt[2] ||
           BUF->seqno[3] != uip_connr->rcv_nxt[3]))
        {
            uip_tcpsend(dev, uip_connr, TCP_ACK, UIP_IPTCPH_LEN);
            goto done;
        }
    }

  /* Next, check if the incoming segment acknowledges any outstanding
   * data. If so, we update the sequence number, reset the length of
   * the outstanding data, calculate RTT estimations, and reset the
   * retransmission timer.
   */

  if ((BUF->flags & TCP_ACK) && uip_outstanding(uip_connr))
    {
      /* Temporary variables. */

      uint8 acc32[4];
      uip_add32(uip_connr->snd_nxt, uip_connr->len, acc32);

      if (BUF->ackno[0] == acc32[0] && BUF->ackno[1] == acc32[1] &&
          BUF->ackno[2] == acc32[2] && BUF->ackno[3] == acc32[3])
        {
          /* Update sequence number. */

          uip_connr->snd_nxt[0] = acc32[0];
          uip_connr->snd_nxt[1] = acc32[1];
          uip_connr->snd_nxt[2] = acc32[2];
          uip_connr->snd_nxt[3] = acc32[3];

          /* Do RTT estimation, unless we have done retransmissions. */

          if (uip_connr->nrtx == 0)
            {
              signed char m;
              m = uip_connr->rto - uip_connr->timer;

              /* This is taken directly from VJs original code in his paper */

              m = m - (uip_connr->sa >> 3);
              uip_connr->sa += m;
              if (m < 0)
                {
                  m = -m;
                }

              m = m - (uip_connr->sv >> 2);
              uip_connr->sv += m;
              uip_connr->rto = (uip_connr->sa >> 3) + uip_connr->sv;
            }

          /* Set the acknowledged flag. */

          uip_flags = UIP_ACKDATA;

          /* Reset the retransmission timer. */

          uip_connr->timer = uip_connr->rto;

          /* Reset length of outstanding data. */

          uip_connr->len = 0;
        }
    }

  /* Do different things depending on in what state the connection is. */

  switch(uip_connr->tcpstateflags & UIP_TS_MASK)
    {
      /* CLOSED and LISTEN are not handled here. CLOSE_WAIT is not
       * implemented, since we force the application to close when the
       * peer sends a FIN (hence the application goes directly from
       * ESTABLISHED to LAST_ACK).
       */

      case UIP_SYN_RCVD:
        /* In SYN_RCVD we have sent out a SYNACK in response to a SYN, and
         * we are waiting for an ACK that acknowledges the data we sent
         * out the last time. Therefore, we want to have the UIP_ACKDATA
         * flag set. If so, we enter the ESTABLISHED state.
         */

        if (uip_flags & UIP_ACKDATA)
          {
            uip_connr->tcpstateflags = UIP_ESTABLISHED;
            uip_connr->len           = 0;
            vdbg("TCP state: UIP_ESTABLISHED\n");

            uip_flags                = UIP_CONNECTED;

            if (dev->d_len > 0)
              {
                uip_flags           |= UIP_NEWDATA;
                uip_incr32(uip_conn->rcv_nxt, dev->d_len);
              }

            dev->d_sndlen            = 0;
            uip_tcpcallback(dev);
            uip_tcpappsend(dev, uip_connr, uip_flags);
            goto done;
          }
        goto drop;

      case UIP_SYN_SENT:
        /* In SYN_SENT, we wait for a SYNACK that is sent in response to
         * our SYN. The rcv_nxt is set to sequence number in the SYNACK
         * plus one, and we send an ACK. We move into the ESTABLISHED
         * state.
         */

        if ((uip_flags & UIP_ACKDATA) &&
            (BUF->flags & TCP_CTL) == (TCP_SYN | TCP_ACK))
          {
            /* Parse the TCP MSS option, if present. */

            if ((BUF->tcpoffset & 0xf0) > 0x50)
              {
                for (i = 0; i < ((BUF->tcpoffset >> 4) - 5) << 2 ;)
                  {
                    opt = dev->d_buf[UIP_IPTCPH_LEN + UIP_LLH_LEN + i];
                    if (opt == TCP_OPT_END)
                      {
                        /* End of options. */

                        break;
                      }
                    else if (opt == TCP_OPT_NOOP)
                      {
                        /* NOP option. */

                        ++i;
                      }
                    else if (opt == TCP_OPT_MSS &&
                              dev->d_buf[UIP_TCPIP_HLEN + UIP_LLH_LEN + 1 + i] == TCP_OPT_MSS_LEN)
                      {
                        /* An MSS option with the right option length. */

                        tmp16 =
                          (dev->d_buf[UIP_TCPIP_HLEN + UIP_LLH_LEN + 2 + i] << 8) |
                          dev->d_buf[UIP_TCPIP_HLEN + UIP_LLH_LEN + 3 + i];
                        uip_connr->initialmss =
                          uip_connr->mss =
                          tmp16 > UIP_TCP_MSS? UIP_TCP_MSS: tmp16;

                        /* And we are done processing options. */

                        break;
                      }
                    else
                      {
                        /* All other options have a length field, so that we
                         * easily can skip past them.
                         */

                        if (dev->d_buf[UIP_TCPIP_HLEN + UIP_LLH_LEN + 1 + i] == 0)
                          {
                            /* If the length field is zero, the options are
                             * malformed and we don't process them further.
                             */

                            break;
                          }
                        i += dev->d_buf[UIP_TCPIP_HLEN + UIP_LLH_LEN + 1 + i];
                      }
                  }
              }

            uip_connr->tcpstateflags = UIP_ESTABLISHED;
            uip_connr->rcv_nxt[0]    = BUF->seqno[0];
            uip_connr->rcv_nxt[1]    = BUF->seqno[1];
            uip_connr->rcv_nxt[2]    = BUF->seqno[2];
            uip_connr->rcv_nxt[3]    = BUF->seqno[3];
            vdbg("TCP state: UIP_ESTABLISHED\n");

            uip_incr32(uip_conn->rcv_nxt, 1);
            uip_flags      = UIP_CONNECTED | UIP_NEWDATA;
            uip_connr->len = 0;
            dev->d_len     = 0;
            dev->d_sndlen  = 0;
            uip_tcpcallback(dev);
            uip_tcpappsend(dev, uip_connr, uip_flags);
            goto done;
          }

        /* Inform the application that the connection failed */

        uip_flags = UIP_ABORT;
        uip_tcpcallback(dev);

        /* The connection is closed after we send the RST */

        uip_conn->tcpstateflags = UIP_CLOSED;
        vdbg("TCP state: UIP_CLOSED\n");

        /* We do not send resets in response to resets. */

        if (BUF->flags & TCP_RST)
          {
            goto drop;
          }
        uip_tcpreset(dev);
        goto done;

      case UIP_ESTABLISHED:
        /* In the ESTABLISHED state, we call upon the application to feed
         * data into the d_buf. If the UIP_ACKDATA flag is set, the
         * application should put new data into the buffer, otherwise we are
         * retransmitting an old segment, and the application should put that
         * data into the buffer.
         *
         * If the incoming packet is a FIN, we should close the connection on
         * this side as well, and we send out a FIN and enter the LAST_ACK
         * state. We require that there is no outstanding data; otherwise the
         * sequence numbers will be screwed up.
         */

        if (BUF->flags & TCP_FIN && !(uip_connr->tcpstateflags & UIP_STOPPED))
          {
            if (uip_outstanding(uip_connr))
              {
                goto drop;
              }

            uip_incr32(uip_conn->rcv_nxt, dev->d_len + 1);
            uip_flags |= UIP_CLOSE;

            if (dev->d_len > 0)
              {
                uip_flags |= UIP_NEWDATA;
              }

            uip_tcpcallback(dev);

            uip_connr->tcpstateflags = UIP_LAST_ACK;
            uip_connr->len = 1;
            uip_connr->nrtx = 0;
            vdbg("TCP state: UIP_LAST_ACK\n");

            uip_tcpsend(dev, uip_connr, TCP_FIN | TCP_ACK, UIP_IPTCPH_LEN);
            goto done;
          }

        /* Check the URG flag. If this is set, the segment carries urgent
           data that we must pass to the application. */
        if ((BUF->flags & TCP_URG) != 0)
          {
#if UIP_URGDATA > 0
            uip_urglen = (BUF->urgp[0] << 8) | BUF->urgp[1];
            if (uip_urglen > dev->d_len)
              {
                /* There is more urgent data in the next segment to come. */

                uip_urglen = dev->d_len;
              }

            uip_incr32(uip_conn->rcv_nxt, uip_urglen);
            dev->d_len     -= uip_urglen;
            uip_urgdata     = dev->d_appdata;
            dev->d_appdata += uip_urglen;
          }
        else
          {
            uip_urglen     = 0;
#else /* UIP_URGDATA > 0 */
            dev->d_appdata =
              ((uint8*)dev->d_appdata) + ((BUF->urgp[0] << 8) | BUF->urgp[1]);
            dev->d_len    -=
              (BUF->urgp[0] << 8) | BUF->urgp[1];
#endif /* UIP_URGDATA > 0 */
          }

        /* If d_len > 0 we have TCP data in the packet, and we flag this
         * by setting the UIP_NEWDATA flag and update the sequence number
         * we acknowledge. If the application has stopped the dataflow
         * using uip_stop(), we must not accept any data packets from the
         * remote host.
         */

        if (dev->d_len > 0 && !(uip_connr->tcpstateflags & UIP_STOPPED))
          {
            uip_flags |= UIP_NEWDATA;
            uip_incr32(uip_conn->rcv_nxt, dev->d_len);
          }

        /* Check if the available buffer space advertised by the other end
         * is smaller than the initial MSS for this connection. If so, we
         * set the current MSS to the window size to ensure that the
         * application does not send more data than the other end can
         * handle.
         *
         * If the remote host advertises a zero window, we set the MSS to
         * the initial MSS so that the application will send an entire MSS
         * of data. This data will not be acknowledged by the receiver,
         * and the application will retransmit it. This is called the
         * "persistent timer" and uses the retransmission mechanim.
         */

        tmp16 = ((uint16)BUF->wnd[0] << 8) + (uint16)BUF->wnd[1];
        if (tmp16 > uip_connr->initialmss || tmp16 == 0)
          {
            tmp16 = uip_connr->initialmss;
          }
        uip_connr->mss = tmp16;

        /* If this packet constitutes an ACK for outstanding data (flagged
         * by the UIP_ACKDATA flag, we should call the application since it
         * might want to send more data. If the incoming packet had data
         * from the peer (as flagged by the UIP_NEWDATA flag), the
         * application must also be notified.
         *
         * When the application is called, the d_len field
         * contains the length of the incoming data. The application can
         * access the incoming data through the global pointer
         * d_appdata, which usually points UIP_IPTCPH_LEN + UIP_LLH_LEN
         *  bytes into the d_buf array.
         *
         * If the application wishes to send any data, this data should be
         * put into the d_appdata and the length of the data should be
         * put into d_len. If the application don't have any data to
         * send, d_len must be set to 0.
         */

        if (uip_flags & (UIP_NEWDATA | UIP_ACKDATA))
          {
            dev->d_sndlen = 0;
            uip_tcpcallback(dev);
            uip_tcpappsend(dev, uip_connr, uip_flags);
            goto done;
          }
        goto drop;

      case UIP_LAST_ACK:
        /* We can close this connection if the peer has acknowledged our
         * FIN. This is indicated by the UIP_ACKDATA flag.
         */

        if (uip_flags & UIP_ACKDATA)
          {
            uip_connr->tcpstateflags = UIP_CLOSED;
            vdbg("TCP state: UIP_CLOSED\n");

            uip_flags = UIP_CLOSE;
            uip_tcpcallback(dev);
          }
        break;

      case UIP_FIN_WAIT_1:
        /* The application has closed the connection, but the remote host
         * hasn't closed its end yet. Thus we do nothing but wait for a
         * FIN from the other side.
         */

        if (dev->d_len > 0)
          {
            uip_incr32(uip_conn->rcv_nxt, dev->d_len);
          }
        if (BUF->flags & TCP_FIN)
          {
            if (uip_flags & UIP_ACKDATA)
              {
                uip_connr->tcpstateflags = UIP_TIME_WAIT;
                uip_connr->timer = 0;
                uip_connr->len = 0;
                vdbg("TCP state: UIP_TIME_WAIT\n");
              }
            else
              {
                uip_connr->tcpstateflags = UIP_CLOSING;
                vdbg("TCP state: UIP_CLOSING\n");
              }

            uip_incr32(uip_conn->rcv_nxt, 1);
            uip_flags = UIP_CLOSE;
            uip_tcpcallback(dev);
            uip_tcpsend(dev, uip_connr, TCP_ACK, UIP_IPTCPH_LEN);
            goto done;
          }
        else if (uip_flags & UIP_ACKDATA)
          {
            uip_connr->tcpstateflags = UIP_FIN_WAIT_2;
            uip_connr->len = 0;
            vdbg("TCP state: UIP_FIN_WAIT_2\n");
            goto drop;
          }

        if (dev->d_len > 0)
          {
            uip_tcpsend(dev, uip_connr, TCP_ACK, UIP_IPTCPH_LEN);
            goto done;
          }
        goto drop;

      case UIP_FIN_WAIT_2:
        if (dev->d_len > 0)
          {
            uip_incr32(uip_conn->rcv_nxt, dev->d_len);
          }

        if (BUF->flags & TCP_FIN)
          {
            uip_connr->tcpstateflags = UIP_TIME_WAIT;
            uip_connr->timer = 0;
            vdbg("TCP state: UIP_TIME_WAIT\n");

            uip_incr32(uip_conn->rcv_nxt, 1);
            uip_flags = UIP_CLOSE;
            uip_tcpcallback(dev);
            uip_tcpsend(dev, uip_connr, TCP_ACK, UIP_IPTCPH_LEN);
            goto done;
          }

        if (dev->d_len > 0)
          {
            uip_tcpsend(dev, uip_connr, TCP_ACK, UIP_IPTCPH_LEN);
            goto done;
          }
        goto drop;

      case UIP_TIME_WAIT:
        uip_tcpsend(dev, uip_connr, TCP_ACK, UIP_IPTCPH_LEN);
        goto done;

      case UIP_CLOSING:
        if (uip_flags & UIP_ACKDATA)
          {
            uip_connr->tcpstateflags = UIP_TIME_WAIT;
            uip_connr->timer = 0;
            vdbg("TCP state: UIP_TIME_WAIT\n");
          }
    }
  goto drop;

done:
  uip_flags = 0;
  return;

drop:
  uip_flags  = 0;
  dev->d_len = 0;
}

#endif /* CONFIG_NET */
