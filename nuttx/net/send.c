/****************************************************************************
 * net/send.c
 *
 *   Copyright (C) 2007, 2008 Gregory Nutt. All rights reserved.
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
#include <sys/socket.h>

#include <string.h>
#include <errno.h>
#include <debug.h>

#include <arch/irq.h>
#include <net/uip/uip-arch.h>

#include "net-internal.h"
#include "uip/uip-internal.h"

/****************************************************************************
 * Definitions
 ****************************************************************************/

#define TCPBUF ((struct uip_tcpip_hdr *)&dev->d_buf[UIP_LLH_LEN])

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* This structure holds the state of the send operation until it can be
 * operated upon from the interrupt level.
 */

struct send_s
{
  FAR struct socket         *snd_sock;    /* Points to the parent socket structure */
  FAR struct uip_callback_s *snd_cb;      /* Reference to callback instance */
  sem_t                      snd_sem;     /* Used to wake up the waiting thread */
  FAR const uint8           *snd_buffer;  /* Points to the buffer of data to send */
  size_t                     snd_buflen;  /* Number of bytes in the buffer to send */
  ssize_t                    snd_sent;    /* The number of bytes sent */
  uint32                     snd_isn;     /* Initial sequence number */
  uint32                     snd_acked;   /* The number of bytes acked */
#if defined(CONFIG_NET_SOCKOPTS) && !defined(CONFIG_DISABLE_CLOCK)
  uint32                     snd_time;    /* last send time for determining timeout */
#endif
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Function: send_getisn
 *
 * Description:
 *   Get the next initial sequence number from the connection stucture
 *
 * Parameters:
 *   conn     The connection structure associated with the socket
 *
 * Returned Value:
 *   None
 *
 * Assumptions:
 *   Running at the interrupt level
 *
 ****************************************************************************/

static uint32 send_getisn(struct uip_conn *conn)
{
   uint32 tmp;
   memcpy(&tmp, conn->snd_nxt, 4);
   return ntohl(tmp);
}

/****************************************************************************
 * Function: send_getackno
 *
 * Description:
 *   Extract the current acknowledgement sequence number from the incoming packet
 *
 * Parameters:
 *   dev - The sructure of the network driver that caused the interrupt
 *
 * Returned Value:
 *   None
 *
 * Assumptions:
 *   Running at the interrupt level
 *
 ****************************************************************************/

static uint32 send_getackno(struct uip_driver_s *dev)
{
   uint32 tmp;
   memcpy(&tmp, TCPBUF->ackno, 4);
   return ntohl(tmp);
}

/****************************************************************************
 * Function: send_timeout
 *
 * Description:
 *   Check for send timeout.
 *
 * Parameters:
 *   pstate   send state structure
 *
 * Returned Value:
 *   TRUE:timeout FALSE:no timeout
 *
 * Assumptions:
 *   Running at the interrupt level
 *
 ****************************************************************************/

#if defined(CONFIG_NET_SOCKOPTS) && !defined(CONFIG_DISABLE_CLOCK)
static inline int send_timeout(struct send_s *pstate)
{
  FAR struct socket *psock = 0;

  /* Check for a timeout configured via setsockopts(SO_SNDTIMEO).
   * If none... we well let the send wait forever.
   */

  psock = pstate->snd_sock;
  if (psock && psock->s_sndtimeo != 0)
    {
      /* Check if the configured timeout has elapsed */

      return net_timeo(pstate->snd_time, psock->s_sndtimeo);
    }

  /* No timeout */

  return FALSE;
}
#endif /* CONFIG_NET_SOCKOPTS && !CONFIG_DISABLE_CLOCK */

/****************************************************************************
 * Function: send_interrupt
 *
 * Description:
 *   This function is called from the interrupt level to perform the actual
 *   send operation when polled by the uIP layer.
 *
 * Parameters:
 *   dev      The sructure of the network driver that caused the interrupt
 *   conn     The connection structure associated with the socket
 *   flags    Set of events describing why the callback was invoked
 *
 * Returned Value:
 *   None
 *
 * Assumptions:
 *   Running at the interrupt level
 *
 ****************************************************************************/

static uint16 send_interrupt(struct uip_driver_s *dev, void *pvconn,
                             void *pvprivate, uint16 flags)
{
  struct uip_conn *conn = (struct uip_conn*)pvconn;
  struct send_s *pstate = (struct send_s *)pvprivate;

  nvdbg("flags: %04x acked: %d sent: %d\n", flags, pstate->snd_acked, pstate->snd_sent);

  /* If this packet contains an acknowledgement, then update the count of
   * acknowldged bytes.
   */

  if ((flags & UIP_ACKDATA) != 0)
    {
      /* The current acknowledgement number number is the (relative) offset
       * of the of the next byte needed by the receiver.  The snd_isn is the
       * offset of the first byte to send to the receiver.  The difference
       * is the number of bytes to be acknowledged.
       */

      pstate->snd_acked = send_getackno(dev) - pstate->snd_isn;
      nvdbg("ACK: acked=%d sent=%d buflen=%d\n",
            pstate->snd_acked, pstate->snd_sent, pstate->snd_buflen);

      /* Have all of the bytes in the buffer been sent and ACKed? */

      if ( pstate->snd_acked >= pstate->snd_buflen)
        {
          /* Yes.  Then pstate->snd_len should hold the number of bytes
           * actually sent.
           */

          goto end_wait;
        }

      /* No.. fall through to send more data if necessary */
    }

  /* Check if we are being asked to retransmit data */

  else if ((flags & UIP_REXMIT) != 0)
    {
      /* Yes.. in this case, reset the number of bytes that have been sent
       * to the number of bytes that have been ACKed.
       */

      pstate->snd_sent = pstate->snd_acked;

      /* Fall through to re-send data from the last that was ACKed */
    }

 /* Check for a loss of connection */

  else if ((flags & (UIP_CLOSE|UIP_ABORT|UIP_TIMEDOUT)) != 0)
    {
      /* Report not connected */

      nvdbg("Lost connection\n");
      pstate->snd_sent = -ENOTCONN;
      goto end_wait;
    }

   /* Check if the outgoing packet is available (it may have been claimed
    * by a sendto interrupt serving a different thread.
    */

#if 0 /* We can't really support multiple senders on the same TCP socket */
   else if (dev->d_sndlen > 0)
     {
       /* Another thread has beat us sending data, wait for the next poll */

         return flags;
      }
#endif

  /* We get here if (1) not all of the data has been ACKed, (2) we have been
   * asked to retransmit data, (3) the connection is still healthy, and (4)
   * the outgoing packet is available for our use.  In this case, we are
   * now free to send more data to receiver.
   */

  if (pstate->snd_sent < pstate->snd_buflen)
    {
      /* Get the amount of data that we can send in the next packet */

      uint32 sndlen = pstate->snd_buflen - pstate->snd_sent;
      if (sndlen > uip_mss(conn))
        {
          sndlen = uip_mss(conn);
        }

      /* Then send that amount of data */

      uip_send(dev, &pstate->snd_buffer[pstate->snd_sent], sndlen);

      /* And update the amount of data sent (but not necessarily ACKed) */

      pstate->snd_sent += sndlen;
      nvdbg("SEND: acked=%d sent=%d buflen=%d\n",
            pstate->snd_acked, pstate->snd_sent, pstate->snd_buflen);
    }

  /* All data has been send and we are just waiting for ACK or re-tranmist
   * indications to complete the send.  Check for a timeout.
   */

#if defined(CONFIG_NET_SOCKOPTS) && !defined(CONFIG_DISABLE_CLOCK)
  else if (send_timeout(pstate))
    {
      /* Yes.. report the timeout */

      nvdbg("TCP timeout\n");
      pstate->snd_sent = -EAGAIN;
      goto end_wait;
    }
#endif /* CONFIG_NET_SOCKOPTS && !CONFIG_DISABLE_CLOCK */

  /* Continue waiting */

  return flags;

end_wait:
  /* Do not allow any further callbacks */

  pstate->snd_cb->flags   = 0;
  pstate->snd_cb->private = NULL;
  pstate->snd_cb->event   = NULL;

  /* Wake up the waiting thread */

  sem_post(&pstate->snd_sem);
  return flags;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Function: send
 *
 * Description:
 *   The send() call may be used only when the socket is in a connected state
 *   (so that the intended recipient is known). The only difference between
 *   send() and write() is the presence of flags. With zero flags parameter,
 *   send() is equivalent to write(). Also, send(sockfd,buf,len,flags) is
 *   equivalent to sendto(sockfd,buf,len,flags,NULL,0).
 *
 * Parameters:
 *   sockfd   Socket descriptor of socket
 *   buf      Data to send
 *   len      Length of data to send
 *   flags    Send flags
 *
 * Returned Value:
 *   On success, returns the number of characters sent.  On  error,
 *   -1 is returned, and errno is set appropriately:
 *
 *   EAGAIN or EWOULDBLOCK
 *     The socket is marked non-blocking and the requested operation
 *     would block.
 *   EBADF
 *     An invalid descriptor was specified.
 *   ECONNRESET
 *     Connection reset by peer.
 *   EDESTADDRREQ
 *     The socket is not connection-mode, and no peer address is set.
 *   EFAULT
 *      An invalid user space address was specified for a parameter.
 *   EINTR
 *      A signal occurred before any data was transmitted.
 *   EINVAL
 *      Invalid argument passed.
 *   EISCONN
 *     The connection-mode socket was connected already but a recipient
 *     was specified. (Now either this error is returned, or the recipient
 *     specification is ignored.)
 *   EMSGSIZE
 *     The socket type requires that message be sent atomically, and the
 *     size of the message to be sent made this impossible.
 *   ENOBUFS
 *     The output queue for a network interface was full. This generally
 *     indicates that the interface has stopped sending, but may be
 *     caused by transient congestion.
 *   ENOMEM
 *     No memory available.
 *   ENOTCONN
 *     The socket is not connected, and no target has been given.
 *   ENOTSOCK
 *     The argument s is not a socket.
 *   EOPNOTSUPP
 *     Some bit in the flags argument is inappropriate for the socket
 *     type.
 *   EPIPE
 *     The local end has been shut down on a connection oriented socket.
 *     In this case the process will also receive a SIGPIPE unless
 *     MSG_NOSIGNAL is set.
 *
 * Assumptions:
 *
 ****************************************************************************/

ssize_t send(int sockfd, const void *buf, size_t len, int flags)
{
  FAR struct socket *psock = sockfd_socket(sockfd);
  struct send_s state;
  irqstate_t save;
  int err;
  int ret = OK;

  /* Verify that the sockfd corresponds to valid, allocated socket */

  if (!psock || psock->s_crefs <= 0)
    {
      err = EBADF;
      goto errout;
    }

  /* If this is a connected socket, then return ENOTCONN */

  if (psock->s_type != SOCK_STREAM || !_SS_ISCONNECTED(psock->s_flags))
    {
      err = ENOTCONN;
      goto errout;
    }

  /* Set the socket state to sending */

  psock->s_flags = _SS_SETSTATE(psock->s_flags, _SF_SEND);

  /* Perform the TCP send operation */

  /* Initialize the state structure.  This is done with interrupts
   * disabled because we don't want anything to happen until we
   * are ready.
   */

  save                = irqsave();
  memset(&state, 0, sizeof(struct send_s));
  (void)sem_init(&state. snd_sem, 0, 0); /* Doesn't really fail */
  state.snd_sock      = psock;             /* Socket descriptor to use */
  state.snd_buflen    = len;               /* Number of bytes to send */
  state.snd_buffer    = buf;               /* Buffer to send from */

  if (len > 0)
    {
      struct uip_conn *conn = (struct uip_conn*)psock->s_conn;

      /* Allocate resources to receive a callback */

      state.snd_cb = uip_tcpcallbackalloc(conn);
      if (state.snd_cb)
        {
          /* Get the initial sequence number that will be used */

          state.snd_isn      = send_getisn(conn); /* Initial sequence number */

          /* Set up the callback in the connection */

          state.snd_cb->flags   = UIP_ACKDATA|UIP_REXMIT|UIP_POLL|UIP_CLOSE|UIP_ABORT|UIP_TIMEDOUT;
          state.snd_cb->private = (void*)&state;
          state.snd_cb->event   = send_interrupt;

          /* Notify the device driver of the availaibilty of TX data */

          netdev_txnotify(&conn->ripaddr);

          /* Wait for the send to complete or an error to occur:  NOTES: (1)
           * sem_wait will also terminate if a signal is received, (2) interrupts
           * are disabled!  They will be re-enabled while the task sleeps and
           * automatically re-enabled when the task restarts.
           */

          ret = sem_wait(&state. snd_sem);

          /* Make sure that no further interrupts are processed */

          uip_tcpcallbackfree(conn, state.snd_cb);
        }
    }

  sem_destroy(&state. snd_sem);
  irqrestore(save);

  /* Set the socket state to idle */

  psock->s_flags = _SS_SETSTATE(psock->s_flags, _SF_IDLE);

  /* Check for a errors.  Errors are signaled by negative errno values
   * for the send length
   */

  if (state.snd_sent < 0)
    {
      err = state.snd_sent;
      goto errout;
    }

  /* If sem_wait failed, then we were probably reawakened by a signal. In
   * this case, sem_wait will have set errno appropriately.
   */

  if (ret < 0)
    {
      err = -ret;
      goto errout;
    }

  /* Return the number of bytes actually sent */

  return state.snd_sent;

errout:
  *get_errno_ptr() = err;
  return ERROR;
}

#endif /* CONFIG_NET && CONFIG_NET_TCP */
