/****************************************************************************
 * net/connect.c
 *
 *   Copyright (C) 2007 Gregory Nutt. All rights reserved.
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
#ifdef CONFIG_NET

#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <arch/irq.h>

#include "net-internal.h"

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct tcp_connect_s
{
  FAR struct uip_conn *tc_conn;       /* Reference to TCP connection structure */
  sem_t                tc_sem;        /* Semaphore signals recv completion */
  int                  tc_result;     /* OK on success, otherwise a negated errno. */
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static void connection_event(void *private);
static inline void tcp_setup_callbacks(struct uip_conn *conn, FAR struct socket *psock,
                                       FAR struct tcp_connect_s *pstate);
static inline void tcp_teardown_callbacks(struct uip_conn *conn, int status);
static void tcp_connect_interrupt(struct uip_driver_s *dev, void *private);
#ifdef CONFIG_NET_IPv6
static inline int tcp_connect(FAR struct socket *psock, const struct sockaddr_in6 *inaddr);
#else
static inline int tcp_connect(FAR struct socket *psock, const struct sockaddr_in *inaddr);
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/
/****************************************************************************
 * Function: connection_event
 *
 * Description:
 *   Some connection related event has occurred
 *
 * Parameters:
 *   dev      The sructure of the network driver that caused the interrupt
 *   private  An instance of struct recvfrom_s cast to void*
 *
 * Returned Value:
 *   None
 *
 * Assumptions:
 *   Running at the interrupt level
 *
 ****************************************************************************/

static void connection_event(void *private)
{
  FAR struct socket *psock = (FAR struct socket *)private;
  if (psock)
    {
      /* UIP_CLOSE: The remote host has closed the connection
       * UIP_ABORT: The remote host has aborted the connection
       * UIP_TIMEDOUT: Connection aborted due to too many retransmissions.
       */
      if ((uip_flags & (UIP_CLOSE|UIP_ABORT|UIP_TIMEDOUT)) != 0)
        {
          /* Indicate that the socket is no longer connected */

          psock->s_flags &= ~_SF_CONNECTED;
        }

      /* UIP_CONNECTED: The socket is successfully connected */

      else if ((uip_flags & UIP_CONNECTED) != 0)
        {
          /* Indicate that the socket is now connected */

          psock->s_flags |= _SF_CONNECTED;
        }
    }
}

/****************************************************************************
 * Function: tcp_setup_callbacks
 ****************************************************************************/

static inline void tcp_setup_callbacks(struct uip_conn *conn, FAR struct socket *psock,
                                       FAR struct tcp_connect_s *pstate)
{
  /* Set up the callbacks in the connection */

  conn->data_private = (void*)pstate;
  conn->data_event   = tcp_connect_interrupt;

  /* Set up to receive callbacks on connection-related events */

  conn->connection_private = (void*)psock;
  conn->connection_event   = connection_event;
}

/****************************************************************************
 * Function: tcp_teardown_callbacks
 ****************************************************************************/

static inline void tcp_teardown_callbacks(struct uip_conn *conn, int status)
{
  /* Make sure that no further interrupts are processed */

  conn->data_private = NULL;
  conn->data_event   = NULL;

  /* If we successfully connected, we will continue to monitor the connection state
   * via callbacks.
   */

  if (status < 0)
    {
      /* Failed to connect */

      conn->connection_private = NULL;
      conn->connection_event   = NULL;
    }
}

/****************************************************************************
 * Function: tcp_connect_interrupt
 *
 * Description:
 *   This function is called from the interrupt level to perform the actual
 *   connection operation via by the uIP layer.
 *
 * Parameters:
 *   dev      The sructure of the network driver that caused the interrupt
 *   private  An instance of struct recvfrom_s cast to void*
 *
 * Returned Value:
 *   None
 *
 * Assumptions:
 *   Running at the interrupt level
 *
 ****************************************************************************/

static void tcp_connect_interrupt(struct uip_driver_s *dev, void *private)
{
  struct tcp_connect_s *pstate = (struct tcp_connect_s *)private;

  /* 'private' might be null in some race conditions (?) */

  if (pstate)
    {
      /* The following errors should be detected here (someday)
       *
       *     ECONNREFUSED
       *       No one listening on the remote address.
       *     ENETUNREACH
       *       Network is unreachable.
       *     ETIMEDOUT
       *       Timeout while attempting connection. The server may be too busy
       *       to accept new connections.
       */

      /* UIP_CLOSE: The remote host has closed the connection
       * UIP_ABORT: The remote host has aborted the connection
       */

      if ((uip_flags & (UIP_CLOSE|UIP_ABORT)) != 0)
        {
          /* Indicate that remote host refused the connection */

          pstate->tc_result = -ECONNREFUSED;
        }

      /* UIP_TIMEDOUT: Connection aborted due to too many retransmissions. */

      else if ((uip_flags & UIP_TIMEDOUT) != 0)
        {
          /* Indicate that the remote host is unreachable (or should this be timedout?) */

          pstate->tc_result = -ECONNREFUSED;
        }

      /* UIP_CONNECTED: The socket is successfully connected */

      else if ((uip_flags & UIP_CONNECTED) != 0)
        {
          /* Indicate that the socket is no longer connected */

          pstate->tc_result = OK;
        }

      /* Otherwise, it is not an event of importance to us at the moment */

      else
        {
          return;
        }

      /* Stop further callbacks */

      tcp_teardown_callbacks(pstate->tc_conn, pstate->tc_result);

      /* Wake up the waiting thread */

      sem_post(&pstate->tc_sem);
    }
}

/****************************************************************************
 * Function: tcp_connect
 *
 * Description:
 *   Perform a TCP connection
 *
 * Parameters:
 *   psock    A reference to the socket structure of the socket to be connected
 *   inaddr   The address of the remote server to connect to
 *
 * Returned Value:
 *   None
 *
 * Assumptions:
 *   Running at the interrupt level
 *
 ****************************************************************************/

#ifdef CONFIG_NET_IPv6
static inline int tcp_connect(FAR struct socket *psock, const struct sockaddr_in6 *inaddr)
#else
static inline int tcp_connect(FAR struct socket *psock, const struct sockaddr_in *inaddr)
#endif
{
  FAR struct uip_conn *conn;
  struct tcp_connect_s state;
  irqstate_t           flags;
  int                  ret = OK;

  /* Interrupts must be disabled through all of the following because
   * we cannot allow the network callback to occur until we are completely
   * setup.
   */

  flags = irqsave();

  /* Get the connection reference from the socket */

  conn = psock->s_conn;
  if (!conn) /* Should always be non-NULL */
    {
      ret = -EINVAL;
    }
  else
    {
      /* Perform the uIP connection operation */

      ret = uip_tcpconnect(conn, inaddr);
    }

  if (ret >= 0)
    {
      /* Initialize the TCP state structure */

      (void)sem_init(&state.tc_sem, 0, 0); /* Doesn't really fail */
      state.tc_conn   = conn;
      state.tc_result = -EAGAIN;

      /* Set up the callbacks in the connection */

      tcp_setup_callbacks(conn, psock, &state);

      /* Wait for either the connect to complete or for an error/timeout to occur.
       * NOTES:  (1) sem_wait will also terminate if a signal is received, (2)
       * interrupts are disabled!  They will be re-enabled while the task sleeps
       * and automatically re-enabled when the task restarts.
       */

      ret = sem_wait(&state.tc_sem);

      /* Uninitialize the state structure */

      (void)sem_destroy(&state.tc_sem);

      /* If sem_wait failed, recover the negated error (probably -EINTR) */

      if (ret < 0)
        {
          int err = *get_errno_ptr();
          if (err >= 0)
            {
              err = ENOSYS;
            }
          ret = -err;
        }
      else
        {
          /* If the wait succeeded, then get the new error value from the state structure */

          ret = state.tc_result;
        }

      /* Make sure that no further interrupts are processed */
      tcp_teardown_callbacks(conn, ret);

      /* Mark the connection bound and connected */
      if (ret >= 0)
        {
          psock->s_flags |= (_SF_BOUND|_SF_CONNECTED);
        }
    }
    irqrestore(flags);
    return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/
/****************************************************************************
 * Function: connect
 *
 * Description:
 *   connect() connects the socket referred to by the file descriptor sockfd
 *   to the address specified by 'addr'. The addrlen argument specifies
 *   the size of 'addr'.  The format of the address in 'addr' is
 *   determined by the address space of the socket sockfd.
 *
 *   If the socket sockfd is of type SOCK_DGRAM then 'addr' is the address
 *   to which datagrams are sent by default, and the only address from which
 *   datagrams are received. If the socket is of type SOCK_STREAM or
 *   SOCK_SEQPACKET, this call attempts to make a connection to the socket
 *   that is bound to the address specified by 'addr'.
 *
 *   Generally, connection-based protocol sockets may successfully connect()
 *   only once; connectionless protocol sockets may use connect() multiple
 *   times to change their association.  Connectionless sockets may dissolve
 *   the association by connecting to an address with the sa_family member of
 *   sockaddr set to AF_UNSPEC.
 *
 * Parameters:
 *   sockfd    Socket descriptor returned by socket()
 *   addr      Server address (form depends on type of socket)
 *   addrlen   Length of actual 'addr'
 *
 * Returned Value:
 *   0 on success; -1 on error with errno set appropriately
 *
 *     EACCES, EPERM
 *       The user tried to connect to a broadcast address without having the
 *       socket broadcast flag enabled or the connection request failed
 *       because of a local firewall rule.
 *     EADDRINUSE
 *       Local address is already in use.
 *     EAFNOSUPPORT
 *       The passed address didn't have the correct address family in its
 *       sa_family field.
 *     EAGAIN
 *       No more free local ports or insufficient entries in the routing
 *       cache.
 *     EALREADY
 *       The socket is non-blocking and a previous connection attempt has
 *       not yet been completed.
 *     EBADF
 *       The file descriptor is not a valid index in the descriptor table.
 *     ECONNREFUSED
 *       No one listening on the remote address.
 *     EFAULT
 *       The socket structure address is outside the user's address space.
 *     EINPROGRESS
 *       The socket is non-blocking and the connection cannot be completed
 *       immediately.
 *     EINTR
 *       The system call was interrupted by a signal that was caught.
 *     EISCONN
 *       The socket is already connected.
 *     ENETUNREACH
 *       Network is unreachable.
 *     ENOTSOCK
 *       The file descriptor is not associated with a socket.
 *     ETIMEDOUT
 *       Timeout while attempting connection. The server may be too busy
 *       to accept new connections.
 *
 * Assumptions:
 *
 ****************************************************************************/

int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
  FAR struct socket *psock = sockfd_socket(sockfd);
#ifdef CONFIG_NET_IPv6
  FAR const struct sockaddr_in6 *inaddr = (const struct sockaddr_in6 *)addr;
#else
  FAR const struct sockaddr_in *inaddr = (const struct sockaddr_in *)addr;
#endif
  int err;
  int ret;

  /* Verify that the sockfd corresponds to valid, allocated socket */

  if (!psock || psock->s_crefs <= 0)
    {
      err = EBADF;
      goto errout;
    }

  /* Verify that a valid address has been provided */

#ifdef CONFIG_NET_IPv6
  if (addr->sa_family != AF_INET6 || addrlen < sizeof(struct sockaddr_in6))
#else
  if (addr->sa_family != AF_INET || addrlen < sizeof(struct sockaddr_in))
#endif
  {
      err = EBADF;
      goto errout;
  }

  /* Perform the connection depending on the protocol type */

  switch (psock->s_type)
    {
      case SOCK_STREAM:
        {
          /* Verify that the socket is not already connected */

          if (_SS_ISCONNECTED(psock->s_flags))
            {
              err = EISCONN;
              goto errout;
            }

          /* Its not ... connect it */

          ret = tcp_connect(psock, inaddr);
          if (ret < 0)
            {
              err = -ret;
              goto errout;
            }
        }
        break;

#ifdef CONFIG_NET_UDP
      case SOCK_DGRAM:
        {
          ret = uip_udpconnect(psock->s_conn, inaddr);
          if (ret < 0)
            {
              err = -ret;
              goto errout;
            }
        }
        break;
#endif

      default:
        err = EBADF;
        goto errout;
    }

  return OK;

errout:
  *get_errno_ptr() = err;
  return ERROR;
}

#endif /* CONFIG_NET */
