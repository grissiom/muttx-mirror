/****************************************************************************
 * net/uip/uiplib.h
 * Various uIP library functions.
 *
 *   Copyright (C) 2007 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <spudmonkey@racsa.co.cr>
 *
 * Based on uIP which also has a BSD style license:
 *
 *   Author: Adam Dunkels <adam@sics.se>
 *   Copyright (c) 2002, Adam Dunkels.
 *   All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
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

#ifndef __UIPLIB_H__
#define __UIPLIB_H__

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <pthread.h>
#include <netinet/in.h>

/****************************************************************************
 * Definitions
 ****************************************************************************/

/* SOCK_DGRAM is the preferred socket type to use when we just want a
 * socket for performing drive ioctls.  However, we can't use SOCK_DRAM
 * if UDP is disabled.
 */

#ifdef CONFIG_NET_UDP
# define UIPLIB_SOCK_IOCTL SOCK_DGRAM
#else
# define UIPLIB_SOCK_IOCTL SOCK_STREAM
#endif

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/* Convert a textual representation of an IP address to a numerical representation.
 *
 * This function takes a textual representation of an IP address in
 * the form a.b.c.d and converts it into a 4-byte array that can be
 * used by other uIP functions.
 *
 * addrstr A pointer to a string containing the IP address in
 * textual form.
 *
 * addr A pointer to a 4-byte array that will be filled in with
 * the numerical representation of the address.
 *
 * Return: 0 If the IP address could not be parsed.
 * Return: Non-zero If the IP address was parsed.
 */

extern unsigned char uiplib_ipaddrconv(char *addrstr, unsigned char *addr);

/* Get and set IP/MAC addresses */

extern int uip_setmacaddr(const char *ifname, const uint8 *macaddr);
extern int uip_getmacaddr(const char *ifname, uint8 *macaddr);

#ifdef CONFIG_NET_IPv6
extern int uip_gethostaddr(const char *ifname, struct in6_addr *addr);
extern int uip_sethostaddr(const char *ifname, const struct in6_addr *addr);
extern int uip_setdraddr(const char *ifname, const struct in6_addr *addr);
extern int uip_setnetmask(const char *ifname, const struct in6_addr *addr);
#else
extern int uip_gethostaddr(const char *ifname, struct in_addr *addr);
extern int uip_sethostaddr(const char *ifname, const struct in_addr *addr);
extern int uip_setdraddr(const char *ifname, const struct in_addr *addr);
extern int uip_setnetmask(const char *ifname, const struct in_addr *addr);
#endif

/* Generic server logic */

extern void uip_server(uint16 portno, pthread_startroutine_t handler, int stacksize);

#endif /* __UIPLIB_H__ */
