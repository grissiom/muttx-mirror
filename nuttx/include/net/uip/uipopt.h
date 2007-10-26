/****************************************************************************
 * uipopt.h
 * Configuration options for uIP.
 *
 * This file is used for tweaking various configuration options for
 * uIP. You should make a copy of this file into one of your project's
 * directories instead of editing this example "uipopt.h" file that
 * comes with the uIP distribution.
 *
 * uIP is configured using the per-project configuration file
 * uipopt.h. This file contains all compile-time options for uIP and
 * should be tweaked to match each specific project. The uIP
 * distribution contains a documented example "uipopt.h" that can be
 * copied and modified for each project.
 *
 * Note: Most of the configuration options in the uipopt.h should not
 * be changed, but rather the per-project defconfig file.
 *
 *   Copyright (C) 2007 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <spudmonkey@racsa.co.cr>
 *
 * This logic was leveraged from uIP which also has a BSD-style license:
 *
 *   Author: Adam Dunkels <adam@dunkels.com>
 *   Copyright (c) 2001-2003, Adam Dunkels.
 *   All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
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

#ifndef __UIPOPT_H__
#define __UIPOPT_H__

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <sys/types.h>
#include <nuttx/config.h>

/****************************************************************************
 * Public Definitions
 ****************************************************************************/

/****************************************************************************
 * Public Type Definitions
 ****************************************************************************/

/* Static configuration options */

/* Ping IP address asignment.
 *
 * uIP uses a "ping" packets for setting its own IP address if this
 * option is set. If so, uIP will start with an empty IP address and
 * the destination IP address of the first incoming "ping" (ICMP echo)
 * packet will be used for setting the hosts IP address.
 */

#ifdef CONFIG_NET_PINGADDRCONF
#define UIP_PINGADDRCONF CONFIG_NET_PINGADDRCONF
#else /* CONFIG_NET_PINGADDRCONF */
#define UIP_PINGADDRCONF 0
#endif /* CONFIG_NET_PINGADDRCONF */

/* IP configuration options */

/* The IP TTL (time to live) of IP packets sent by uIP.
 *
 * This should normally not be changed.
 */

#define UIP_TTL         64

/* Turn on support for IP packet reassembly.
 *
 * uIP supports reassembly of fragmented IP packets. This features
 * requires an additonal amount of RAM to hold the reassembly buffer
 * and the reassembly code size is approximately 700 bytes.  The
 * reassembly buffer is of the same size as the d_buf buffer
 * (configured by UIP_BUFSIZE).
 *
 * Note: IP packet reassembly is not heavily tested.
 */

#define UIP_REASSEMBLY 0

/* The maximum time an IP fragment should wait in the reassembly
 * buffer before it is dropped.
 */

#define UIP_REASS_MAXAGE 40

/* UDP configuration options */

/* Toggles if UDP checksums should be used or not.
 *
 * Note: Support for UDP checksums is currently not included in uIP,
 * so this option has no function.
 */

#ifdef CONFIG_NET_UDP_CHECKSUMS
# define UIP_UDP_CHECKSUMS CONFIG_NET_UDP_CHECKSUMS
#else
# define UIP_UDP_CHECKSUMS 0
#endif

/* The maximum amount of concurrent UDP connections. */

#ifdef CONFIG_NET_UDP_CONNS
# define UIP_UDP_CONNS CONFIG_NET_UDP_CONNS
#else /* CONFIG_NET_UDP_CONNS */
# define UIP_UDP_CONNS    10
#endif /* CONFIG_NET_UDP_CONNS */

/* TCP configuration options */

/* The maximum number of simultaneously open TCP connections.
 *
 * Since the TCP connections are statically allocated, turning this
 * configuration knob down results in less RAM used. Each TCP
 * connection requires approximatly 30 bytes of memory.
 */

#ifndef CONFIG_NET_MAX_CONNECTIONS
# define UIP_CONNS       10
#else /* CONFIG_NET_MAX_CONNECTIONS */
# define UIP_CONNS CONFIG_NET_MAX_CONNECTIONS
#endif /* CONFIG_NET_MAX_CONNECTIONS */

/* The maximum number of simultaneously listening TCP ports.
 *
 * Each listening TCP port requires 2 bytes of memory.
 */

#ifndef CONFIG_NET_MAX_LISTENPORTS
# define UIP_LISTENPORTS 20
#else /* CONFIG_NET_MAX_LISTENPORTS */
# define UIP_LISTENPORTS CONFIG_NET_MAX_LISTENPORTS
#endif /* CONFIG_NET_MAX_LISTENPORTS */

/* Determines if support for TCP urgent data notification should be
 * compiled in.
 *
 * Urgent data (out-of-band data) is a rarely used TCP feature that
 * very seldom would be required.
 */

#define UIP_URGDATA      0

/* The initial retransmission timeout counted in timer pulses.
 *
 * This should not be changed.
 */

#define UIP_RTO         3

/* The maximum number of times a segment should be retransmitted
 * before the connection should be aborted.
 *
 * This should not be changed.
 */

#define UIP_MAXRTX      8

/* The maximum number of times a SYN segment should be retransmitted
 * before a connection request should be deemed to have been
 * unsuccessful.
 *
 * This should not need to be changed.
 */

#define UIP_MAXSYNRTX      5

/* The TCP maximum segment size.
 *
 * This is should not be to set to more than
 * UIP_BUFSIZE - UIP_LLH_LEN - UIP_TCPIP_HLEN.
 */

#define UIP_TCP_MSS     (UIP_BUFSIZE - UIP_LLH_LEN - UIP_TCPIP_HLEN)

/* The size of the advertised receiver's window.
 *
 * Should be set low (i.e., to the size of the d_buf buffer) is the
 * application is slow to process incoming data, or high (32768 bytes)
 * if the application processes data quickly.
 */

#ifndef CONFIG_NET_RECEIVE_WINDOW
# define UIP_RECEIVE_WINDOW UIP_TCP_MSS
#else
# define UIP_RECEIVE_WINDOW CONFIG_NET_RECEIVE_WINDOW
#endif

/* How long a connection should stay in the TIME_WAIT state.
 *
 * This configiration option has no real implication, and it should be
 * left untouched.
 */

#define UIP_TIME_WAIT_TIMEOUT 120

/* ARP configuration options */

/* The size of the ARP table.
 *
 * This option should be set to a larger value if this uIP node will
 * have many connections from the local network.
 */

#ifdef CONFIG_NET_ARPTAB_SIZE
# define UIP_ARPTAB_SIZE CONFIG_NET_ARPTAB_SIZE
#else
# define UIP_ARPTAB_SIZE 8
#endif

/* The maxium age of ARP table entries measured in 10ths of seconds.
 *
 * An UIP_ARP_MAXAGE of 120 corresponds to 20 minutes (BSD
 * default).
 */

#define UIP_ARP_MAXAGE 120

/* General configuration options */

/* The size of the uIP packet buffer.
 *
 * The uIP packet buffer should not be smaller than 60 bytes, and does
 * not need to be larger than 1500 bytes. Lower size results in lower
 * TCP throughput, larger size results in higher TCP throughput.
 */

#ifndef CONFIG_NET_BUFFER_SIZE
# define UIP_BUFSIZE     400
#else /* CONFIG_NET_BUFFER_SIZE */
# define UIP_BUFSIZE CONFIG_NET_BUFFER_SIZE
#endif /* CONFIG_NET_BUFFER_SIZE */

/* Determines if statistics support should be compiled in.
 *
 * The statistics is useful for debugging and to show the user.
 */

#ifndef CONFIG_NET_STATISTICS
# define UIP_STATISTICS  0
#else /* CONFIG_NET_STATISTICS */
# define UIP_STATISTICS CONFIG_NET_STATISTICS
#endif /* CONFIG_NET_STATISTICS */

/* Broadcast support.
 *
 * This flag configures IP broadcast support. This is useful only
 * together with UDP.
 */

#ifndef CONFIG_NET_BROADCAST
# define UIP_BROADCAST 0
#else /* CONFIG_NET_BROADCAST */
# define UIP_BROADCAST CONFIG_NET_BROADCAST
#endif /* CONFIG_NET_BROADCAST */

/* The link level header length.
 *
 * This is the offset into the d_buf where the IP header can be
 * found. For Ethernet, this should be set to 14. For SLIP, this
 * should be set to 0.
 */

#ifdef CONFIG_NET_LLH_LEN
# define UIP_LLH_LEN CONFIG_NET_LLH_LEN
#else /* CONFIG_NET_LLH_LEN */
# define UIP_LLH_LEN     14
#endif /* CONFIG_NET_LLH_LEN */

/* CPU architecture configuration
 *
 * The CPU architecture configuration is where the endianess of the
 * CPU on which uIP is to be run is specified. Most CPUs today are
 * little endian, and the most notable exception are the Motorolas
 * which are big endian. The CONFIG_ENDIAN_BIG macro should be changed
 * if uIP is to be run on a big endian architecture.
 */

/* The byte order of the CPU architecture on which uIP is to be run.
 *
 * This option can be either CONFIG_ENDIAN_BIG (Motorola byte order) or
 * default little endian byte order (Intel byte order).
 */

#define UIP_BIG_ENDIAN     1234
#define UIP_LITTLE_ENDIAN  3412

#ifdef CONFIG_ENDIAN_BIG
# define UIP_BYTE_ORDER     UIP_BIG_ENDIAN
#else
# define UIP_BYTE_ORDER     UIP_LITTLE_ENDIAN
#endif

/****************************************************************************
 * Public Type Definitions
 ****************************************************************************/

/* Statistics datatype
 *
 * This typedef defines the dataype used for keeping statistics in
 * uIP.
 */

typedef uint16 uip_stats_t;

#endif /* __UIPOPT_H__ */
