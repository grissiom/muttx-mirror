/****************************************************************************
 * apps/netutils/ftpc/ftpc_socket.c
 *
 *   Copyright (C) 2011 Gregory Nutt. All rights reserved.
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

#include <sys/socket.h>

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <debug.h>
#include <errno.h>

#include "ftpc_internal.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ftpc_sockinit
 *
 * Description:
 *   Initialize a socket.  Create the socket and "wrap" it as C standard
 *   incoming and outgoing streams.
 *
 ****************************************************************************/

int ftpc_sockinit(FAR struct ftpc_socket_s *sock)
{
  int dupsd;

  /* Initialize the socket structure */

  memset(sock, 0, sizeof(struct ftpc_socket_s));
  sock->raddr.sin_family = AF_INET;

  /* Create a socket descriptor */

  sock->sd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (sock->sd < 0)
    {
      ndbg("socket() failed: %d\n", errno);
      goto errout;
    }

  /* 'dup' the socket descriptor to create an independent input stream */

  dupsd = dup(sock->sd);
  if (dupsd < 0)
    {
      ndbg("socket() failed: %d\n", errno);
      goto errout_with_sd;
    }

  /* Call fdopen to "wrap" the input stream with C buffered I/O */

  sock->instream = fdopen(dupsd, "r");
  if (!sock->instream)
    {
      ndbg("fdopen() failed: %d\n", errno);
      close(dupsd);
      goto errout_with_sd;
    }

  /* 'dup' the socket descriptor to create an independent output stream */

  dupsd = dup(sock->sd);
  if (dupsd < 0)
    {
      ndbg("socket() failed: %d\n", errno);
      goto errout_with_instream;
    }

  /* Call fdopen to "wrap" the output stream with C buffered I/O */

  sock->outstream = fdopen(dupsd, "w");
  if (!sock->outstream)
    {
      ndbg("fdopen() failed: %d\n", errno);
      close(dupsd);
      goto errout_with_instream;
    }

  return OK;

errout_with_instream:
  fclose(sock->instream);
errout_with_sd:
  close(sock->sd);
errout:
  return ERROR;
}

/****************************************************************************
 * Name: ftpc_sockclose
 *
 * Description:
 *   Close a socket
 *
 ****************************************************************************/

void ftpc_sockclose(struct ftpc_socket_s *sock)
{
  fclose(sock->instream);
  fclose(sock->outstream);
  close(sock->sd);
  memset(sock, 0, sizeof(struct ftpc_socket_s));
}

/****************************************************************************
 * Name: ftpc_sockconnect
 *
 * Description:
 *   Connect the socket to the host
 *
 ****************************************************************************/

int ftpc_sockconnect(struct ftpc_socket_s *sock, struct sockaddr_in *addr)
{
  int ret;

  /* Connect to the socket */

  ret = connect(sock->sd, (struct sockaddr *)addr, sizeof(struct sockaddr));
  if (ret < 0)
    {
      ndbg("connect() failed: %d\n", errno);
      close(sock->sd);
      return ERROR;
    }

  /* Get the local address of the socket */

  ret = ftpc_sockgetsockname(sock, &sock->laddr);
  if (ret < 0)
    {
      ndbg("ftpc_sockgetsockname() failed: %d\n", errno);
      close(sock->sd);
      return ERROR;
    }
  sock->connected = true;

  return OK;
}

/****************************************************************************
 * Name: ftpc_sockcopy
 *
 * Description:
 *   Copy the socket state from one location to another.
 *
 ****************************************************************************/

void ftpc_sockcopy(FAR struct ftpc_socket_s *dest,
                   FAR const struct ftpc_socket_s *src)
{
  memcpy(&dest->raddr, &src->raddr, sizeof(struct sockaddr_in));
  memcpy(&dest->laddr, &src->laddr, sizeof(struct sockaddr_in));
  dest->connected = ftpc_sockconnected(src);
}

/****************************************************************************
 * Name: ftpc_sockaccept
 *
 * Description:
 *   Accept a connection from the server.
 *
 ****************************************************************************/

int ftpc_sockaccept(struct ftpc_socket_s *sock, const char *mode, bool passive)
{
  struct sockaddr addr;
  socklen_t addrlen;
  int dupsd;
  int sd;

  /* In active mode FTP the client connects from a random port (N>1023) to the
   * FTP server's command port, port 21. Then, the client starts listening to
   * port N+1 and sends the FTP command PORT N+1 to the FTP server. The server
   * will then connect back to the client's specified data port from its local
   * data port, which is port 20. In passive mode FTP the client initiates
   * both connections to the server, solving the problem of firewalls filtering 
   * the incoming data port connection to the client from the server. When
   * opening an FTP connection, the client opens two random ports locally
   * (N>1023 and N+1). The first port contacts the server on port 21, but
   * instead of then issuing a PORT command and allowing the server to connect
   * back to its data port, the client will issue the PASV command. The result
   * of this is that the server then opens a random unprivileged port (P >
   * 1023) and sends the PORT P command back to the client. The client then
   * initiates the connection from port N+1 to port P on the server to transfer 
   * data.
   */

  if (!passive)
    {
      addrlen = sizeof(addr);
      sd      = accept(sock->sd, &addr, &addrlen);
      close(sock->sd);
      if (sd == -1)
        {
          ndbg("accept() failed: %d\n", errno);
          sock->sd = -1;
          return ERROR;
        }

      sock->sd = sd;
      memcpy(&sock->laddr, &addr, sizeof(sock->laddr));
    }

  /* Create in/out C buffer I/O streams on the cmd channel */

  fclose(sock->instream);
  fclose(sock->outstream);

  /* Dup the socket descriptor and create the incoming stream */

  dupsd = dup(sock->sd);
  if (dupsd < 0)
    {
      ndbg("dup() failed: %d\n", errno);
      goto errout_with_sd;
    }

  sock->instream = fdopen(dupsd, mode);
  if (!sock->instream)
    {
      ndbg("fdopen() failed: %d\n", errno);
      close(dupsd);
      goto errout_with_sd;
    }

  /* Dup the socket descriptor and create the outgoing stream */

  dupsd = dup(sock->sd);
  if (dupsd < 0)
    {
      ndbg("dup() failed: %d\n", errno);
      goto errout_with_instream;
    }

  sock->outstream = fdopen(dupsd, mode);
  if (!sock->outstream)
    {
      ndbg("fdopen() failed: %d\n", errno);
      close(dupsd);
      goto errout_with_instream;
    }

  return OK;

errout_with_instream:
  fclose(sock->instream);
errout_with_sd:
  close(sock->sd);
  sock->sd = -1;
  return ERROR;
}

/****************************************************************************
 * Name: ftpc_socklisten
 *
 * Description:
 *   Bind the socket to local address and wait for connection from the server.
 *
 ****************************************************************************/

int ftpc_socklisten(struct ftpc_socket_s *sock)
{
  unsigned int addrlen = sizeof(sock->laddr);
  int ret;

  /* Bind the local socket to the local address */

  sock->laddr.sin_port = 0;
  ret = bind(sock->sd, (struct sockaddr *)&sock->laddr, addrlen);
  if (ret < 0)
    {
      ndbg("bind() failed: %d\n", errno);
      return ERROR;
    }

  /* Wait for the connection to the server */

  if (listen(sock->sd, 1) == -1)
    {
      return ERROR;
    }

  /* Then get the local address selected by NuttX */

  ret = ftpc_sockgetsockname(sock, &sock->laddr);
  return ret;
}

/****************************************************************************
 * Name: ftpc_sockprintf
 *
 * Description:
 *   printf to a socket stream
 *
 ****************************************************************************/

int ftpc_sockprintf(struct ftpc_socket_s *sock, const char *str, ...)
{
  va_list ap;
  int r;

  va_start(ap, str);
  r = vfprintf(sock->outstream, str, ap);
  va_end(ap);
  return r;
}

/****************************************************************************
 * Name: ftpc_sockgetsockname
 *
 * Description:
 *   Get the address of the local socket
 *
 ****************************************************************************/

int ftpc_sockgetsockname(FAR struct ftpc_socket_s *sock,
                         FAR struct sockaddr_in *addr)
{
  unsigned int len = sizeof(struct sockaddr_in);
  int ret;

  ret = getsockname(sock->sd, (struct sockaddr *)addr, &len);
  if (ret < 0)
    {
      ndbg("getsockname failed: %d\n", errno);
      return ERROR;
    }
  return OK;
}
