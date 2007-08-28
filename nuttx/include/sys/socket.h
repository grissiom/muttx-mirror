/****************************************************************************
 * socket.h
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

#ifndef __SYS_SOCKET_H
#define __SYS_SOCKET_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <sys/types.h>

/****************************************************************************
 * Definitions
 ****************************************************************************/

/* The socket()domain parameter specifies a communication domain; this selects
 * the protocol family which will be used for communication.
 */

#define PF_UNIX       0 /* Local communication */
#define PF_LOCAL      1 /* Local communication */
#define PF_INET       2 /* IPv4 Internet protocols */
#define PF_INET6      3 /* IPv6 Internet protocols */
#define PF_IPX        4 /* IPX - Novell protocols */
#define PF_NETLINK    5 /* Kernel user interface device */
#define PF_X25        6 /* ITU-T X.25 / ISO-8208 protocol */
#define PF_AX25       7 /* Amateur radio AX.25 protocol */
#define PF_ATMPVC     8 /* Access to raw ATM PVCs */
#define PF_APPLETALK  9 /* Appletalk */
#define PF_PACKET    10 /* Low level packet interface */

/*The socket created by socket() has the indicated type, which specifies
 * the communication semantics.
 */

#define SOCK_STREAM    0 /* Provides sequenced, reliable, two-way, connection-based byte streams.
                          * An  out-of-band data transmission mechanism may be supported. */
#define SOCK_DGRAM     1 /* Supports  datagrams (connectionless, unreliable messages of a fixed
                          * maximum length). */
#define SOCK_SEQPACKET 2 /* Provides a sequenced, reliable, two-way connection-based data
                          * transmission path for datagrams of fixed maximum length; a consumer
                          * is required to read an entire packet with each read system call. */
#define SOCK_RAW       3 /* Provides raw network protocol access. */
#define SOCK_RDM       4 /* Provides a reliable datagram layer that does not guarantee ordering. */
#define SOCK_PACKET    5 /* Obsolete and should not be used in new programs */

/****************************************************************************
 * Type Definitions
 ****************************************************************************/

struct sockaddr
{
  sa_family_t sa_family;
  char        sa_data[14];
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#undef EXTERN
#if defined(__cplusplus)
#define EXTERN extern "C"
extern "C" {
#else
#define EXTERN extern
#endif

EXTERN int socket(int domain, int type, int protocol);
EXTERN int bind(int sockfd, const struct sockaddr *my_addr, socklen_t addrlen);

#undef EXTERN
#if defined(__cplusplus)
}
#endif

#endif /* __SYS_SOCKET_H */
