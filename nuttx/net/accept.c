/****************************************************************************
 * net/accept.c
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
#if defined(CONFIG_NET) && CONFIG_NSOCKET_DESCRIPTORS > 0

#include <sys/types.h>
#include <sys/socket.h>
#include <semaphore.h>
#include <string.h>
#include <errno.h>
#include <arch/irq.h>

#include "net-internal.h"

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct accept_s
{
  sem_t                          acpt_sem;        /* Wait for interrupt event */
#ifdef CONFIG_NET_IPv6
  FAR const struct sockaddr_in6 *acpt_addr;       /* Return connection adress */
#else
  FAR const struct sockaddr_in  *acpt_addr;       /* Return connection adress */
#endif
  FAR struct uip_conn           *acpt_listenconn; /* The listener connection */
  FAR struct uip_conn           *acpt_newconn;    /* The accepted connection */
  int                            acpt_result;     /* The result of the wait */
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Function: accept_interrupt
 *
 * Description:
 *   Receive interrupt level callbacks when connections occur
 *
 ****************************************************************************/

static int accept_interrupt(void *private, struct uip_conn *conn)
{
  struct accept_s *pstate = (struct accept_s *)private;
  int ret = -EINVAL;
  if (pstate)
    {
      /* Get the connection address */
#warning "need to return the address of the connection"

      /* Save the connection structure */
      
      pstate->acpt_newconn = conn;
      pstate->acpt_result  = OK;
      sem_post(&pstate->acpt_sem);
      
      /* Stop any further callbacks */

      pstate->acpt_listenconn->accept_private = NULL;
      pstate->acpt_listenconn->accept         = NULL;
      ret = OK;
  }
  return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Function: accept
 *
 * Description:
 *   The accept function is used with connection-based socket types
 *   (SOCK_STREAM, SOCK_SEQPACKET and SOCK_RDM). It extracts the first
 *   connection request on the queue of pending connections, creates a new
 *   connected socket with mostly the same properties as 'sockfd', and
 *   allocates a new socket descriptor for the socket, which is returned. The
 *   newly created socket is no longer in the listening state. The original
 *   socket 'sockfd' is unaffected by this call.  Per file descriptor flags
 *   are not inherited across an accept.
 *
 *   The 'sockfd' argument is a socket descriptor that has been created with
 *   socket(), bound to a local address with bind(), and is listening for
 *   connections after a call to listen().
 *
 *   On return, the 'addr' structure is filled in with the address of the
 *   connecting entity. The 'addrlen' argument initially contains the size
 *   of the structure pointed to by 'addr'; on return it will contain the
 *   actual length of the address returned.
 *
 *   If no pending connections are present on the queue, and the socket is
 *   not marked as non-blocking, accept blocks the caller until a connection
 *   is present. If the socket is marked non-blocking and no pending
 *   connections are present on the queue, accept returns EAGAIN.
 *
 * Parameters:
 *   sockfd   The listening socket descriptior
 *   addr     Receives the address of the connecting client
 *   addrlen  Input: allocated size of 'addr', Return: returned size of 'addr'
 *
 * Returned Value:
 *  Returns -1 on error. If it succeeds, it returns a non-negative integer
 *  that is a descriptor for the accepted socket.
 *
 * EAGAIN or EWOULDBLOCK
 *   The socket is marked non-blocking and no connections are present to
 *   be accepted.
 * EBADF
 *   The descriptor is invalid.
 * ENOTSOCK
 *  The descriptor references a file, not a socket.
 * EOPNOTSUPP
 *   The referenced socket is not of type SOCK_STREAM.
 * EINTR
 *   The system call was interrupted by a signal that was caught before
 *   a valid connection arrived.
 * ECONNABORTED
 *   A connection has been aborted.
 * EINVAL
 *   Socket is not listening for connections.
 * EMFILE
 *   The per-process limit of open file descriptors has been reached.
 * ENFILE
 *   The system maximum for file descriptors has been reached.
 * EFAULT
 *   The addr parameter is not in a writable part of the user address
 *   space.
 * ENOBUFS or ENOMEM
 *   Not enough free memory.
 * EPROTO
 *   Protocol error.
 * EPERM
 *   Firewall rules forbid connection.
 *
 * Assumptions:
 *
 ****************************************************************************/

int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
  FAR struct socket *psock = sockfd_socket(sockfd);
  FAR struct socket *pnewsock;
  FAR struct uip_conn *conn;
  struct accept_s state;
#ifdef CONFIG_NET_IPv6
  FAR const struct sockaddr_in6 *inaddr = (const struct sockaddr_in6 *)addr;
#else
  FAR const struct sockaddr_in *inaddr = (const struct sockaddr_in *)addr;
#endif
  irqstate_t save;
  int newfd;
  int err;
  int ret;

  /* Verify that the sockfd corresponds to valid, allocated socket */

  if (!psock || psock->s_crefs <= 0)
    {
      /* It is not a valid socket description.  Distinguish between the cases
       * where sockfd is a just valid and when it is a valid file descriptor used
       * in the wrong context.
       */

#if CONFIG_NFILE_DESCRIPTORS > 0
      if ((unsigned int)sockfd < CONFIG_NFILE_DESCRIPTORS)
        {
          err = ENOTSOCK;
        }
      else
#endif
        {
          err = EBADF;
        }
      goto errout;
    }

  /* We have a socket descriptor, but it is a stream? */
 
  if (psock->s_type != SOCK_STREAM)
    {
      err = EOPNOTSUPP;
      goto errout;
    }

  /* Is the socket listening for a connection? */

  if (!_SS_ISLISTENING(psock->s_flags))
    {
      err = EINVAL;
      goto errout;
    }

  /* Verify that a valid memory block has been provided to receive the address address */

#ifdef CONFIG_NET_IPv6
  if (addr->sa_family != AF_INET6 || *addrlen < sizeof(struct sockaddr_in6))
#else
  if (addr->sa_family != AF_INET || *addrlen < sizeof(struct sockaddr_in))
#endif
  {
      err = EBADF;
      goto errout;
  }

  /* Allocate a socket descriptor for the new connection now (so that it cannot fail later) */

  newfd = sockfd_allocate();
  if (newfd < 0)
    {
      err = ENFILE;
      goto errout;
    }

  pnewsock = sockfd_socket(newfd);
  if (newfd)
    {
      err = ENFILE;
      goto errout_with_socket;
    }
    
  /* Set the socket state to accepting */

  psock->s_flags = _SS_SETSTATE(psock->s_flags, _SF_ACCEPT);

  /* Perform the TCP accept operation */

  /* Initialize the state structure.  This is done with interrupts
   * disabled because we don't want anything to happen until we
   * are ready.
   */

  save                  = irqsave();
  state.acpt_addr       = inaddr;
  state.acpt_listenconn = psock->s_conn;
  state.acpt_newconn    = NULL;
  state.acpt_result     = OK;

  /* Set up the callback in the connection */

  conn                  = (struct uip_conn *)psock->s_conn;
  conn->accept_private  = (void*)&state;
  conn->accept          = accept_interrupt;

  /* Wait for the send to complete or an error to occur:  NOTES: (1)
   * sem_wait will also terminate if a signal is received, (2) interrupts
   * are disabled!  They will be re-enabled while the task sleeps and
   * automatically re-enabled when the task restarts.
   */

  ret = sem_wait(&state. acpt_sem);

  /* Make sure that no further interrupts are processed */

  conn->accept_private = NULL;
  conn->accept         = NULL;

  sem_destroy(&state. acpt_sem);
  irqrestore(save);

  /* Set the socket state to idle */

  psock->s_flags = _SS_SETSTATE(psock->s_flags, _SF_IDLE);

  /* Check for a errors.  Errors are signaled by negative errno values
   * for the send length
   */

  if (state.acpt_result != 0)
    {
      err = state.acpt_result;
      goto errout_with_socket;
    }

  /* If sem_wait failed, then we were probably reawakened by a signal. In
   * this case, sem_wait will have set errno appropriately.
   */

  if (ret < 0)
    {
      err = -ret;
      goto errout_with_socket;
    }

  /* Initialize the socket structure */

  pnewsock->s_type = SOCK_STREAM;
  pnewsock->s_conn = state.acpt_newconn;
  return newfd;

errout_with_socket:
  sockfd_release(newfd);

errout:
  *get_errno_ptr() = err;
  return ERROR;
}

#endif /* CONFIG_NET && CONFIG_NSOCKET_DESCRIPTORS */
