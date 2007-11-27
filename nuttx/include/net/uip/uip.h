/****************************************************************************
 * net/uip/uip.h
 *
 * The uIP header file contains definitions for a number of C macros that
 * are used by uIP programs as well as internal uIP structures and function
 * declarations.
 *
 *   Copyright (C) 2007 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <spudmonkey@racsa.co.cr>
 *
 * This logic was leveraged from uIP which also has a BSD-style license:
 *
 *   Author Adam Dunkels <adam@dunkels.com>
 *   Copyright (c) 2001-2003, Adam Dunkels.
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

#ifndef __NET_UIP_UIP_H
#define __NET_UIP_UIP_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/types.h>
#include <queue.h>
#include <arpa/inet.h>

#include <net/uip/uipopt.h>
/****************************************************************************
 * Definitions
 ****************************************************************************/

/* The following flags may be set in the set of flags before calling the
 * application callback. The UIP_ACKDATA, UIP_NEWDATA, and UIP_CLOSE flags
 * may be set at the same time, whereas the others are mutualy exclusive.
 */

#define UIP_ACKDATA    (1 << 0) /* Signifies that the outstanding data was acked and the
                                 * application should send out new data instead of retransmitting
                                 * the last data */
#define UIP_NEWDATA    (1 << 1) /* Flags the fact that the peer has sent us new data */
#define UIP_REXMIT     (1 << 2) /* Tells the application to retransmit the data that was last
                                 * sent */
#define UIP_POLL       (1 << 3) /* Used for polling the application, to check if the application
                                 * has data that it wants to send */
#define UIP_CLOSE      (1 << 4) /* The remote host has closed the connection, thus the connection
                                 * has gone away. Or the application signals that it wants to
                                 * close the connection */
#define UIP_ABORT      (1 << 5) /* The remote host has aborted the connection, thus the connection
                                 * has gone away. Or the application signals that it wants to
                                 * abort the connection */
#define UIP_CONNECTED  (1 << 6) /* We have got a connection from a remote host and have set up a
                                 * new connection for it, or an active connection has been
                                 * successfully established */
#define UIP_TIMEDOUT   (1 << 7) /* The connection has been aborted due to too many retransmissions */

#define UIP_DATA_EVENTS (UIP_ACKDATA|UIP_NEWDATA|UIP_REXMIT|UIP_POLL)
#define UIP_CONN_EVENTS (UIP_CLOSE|UIP_ABORT|UIP_CONNECTED|UIP_TIMEDOUT)

/* The buffer size available for user data in the d_buf buffer.
 *
 * This macro holds the available size for user data in the
 * d_buf buffer. The macro is intended to be used for checking
 * bounds of available user data.
 *
 * Example:
 *
 *   snprintf(dev->d_appdata, UIP_APPDATA_SIZE, "%u\n", i);
 */

#define UIP_APPDATA_SIZE (CONFIG_NET_BUFSIZE - UIP_LLH_LEN - UIP_TCPIP_HLEN)

#define UIP_PROTO_ICMP  1
#define UIP_PROTO_TCP   6
#define UIP_PROTO_UDP   17
#define UIP_PROTO_ICMP6 58

/* Header sizes */

#ifdef CONFIG_NET_IPv6
# define UIP_IPH_LEN    40    /* Size of IP header */
#else
# define UIP_IPH_LEN    20    /* Size of IP header */
#endif

/****************************************************************************
 * Public Type Definitions
 ****************************************************************************/

/* Repressentation of an IP address */

typedef in_addr_t uip_ip4addr_t;
typedef uint16 uip_ip6addr_t[8];

#ifdef CONFIG_NET_IPv6
typedef uip_ip6addr_t uip_ipaddr_t;
#else
typedef uip_ip4addr_t uip_ipaddr_t;
#endif

/* The IP header */

struct uip_ip_hdr
{
#ifdef CONFIG_NET_IPv6

  /* IPv6 Ip header */

  uint8  vtc;               /* Bits 0-3: version, bits 4-7: traffic class (MS) */
  uint8  tcf;               /* Bits 0-3: traffic class (LS), 4-bits: flow label (MS) */
  uint16 flow;              /* 16-bit flow label (LS) */
  uint8  len[2];            /* 16-bit Payload length */
  uint8  proto;             /*  8-bit Next header (same as IPv4 protocol field) */
  uint8  ttl;               /*  8-bit Hop limit (like IPv4 TTL field) */
  uip_ip6addr_t srcipaddr;  /* 128-bit Source address */
  uip_ip6addr_t destipaddr; /* 128-bit Destination address */

#else /* CONFIG_NET_IPv6 */

  /* IPv4 IP header */

  uint8  vhl;              /*  8-bit Version (4) and header length (5 or 6) */
  uint8  tos;              /*  8-bit Type of service (e.g., 6=TCP) */
  uint8  len[2];           /* 16-bit Total length */
  uint8  ipid[2];          /* 16-bit Identification */
  uint8  ipoffset[2];      /* 16-bit IP flags + fragment offset */
  uint8  ttl;              /*  8-bit Time to Live */
  uint8  proto;            /*  8-bit Protocol */
  uint16 ipchksum;         /* 16-bit Header checksum */
  uint16 srcipaddr[2];     /* 32-bit Source IP address */
  uint16 destipaddr[2];    /* 32-bit Destination IP address */

#endif /* CONFIG_NET_IPv6 */
};

/* Protocol-specific support */

#include <net/uip/uip-tcp.h>
#include <net/uip/uip-udp.h>
#include <net/uip/uip-icmp.h>

/* The structure holding the uIP statistics that are gathered if
 * CONFIG_NET_STATISTICS is defined.
 */

#ifdef CONFIG_NET_STATISTICS
struct uip_ip_stats_s
{
  uip_stats_t drop;       /* Number of dropped packets at the IP layer */
  uip_stats_t recv;       /* Number of received packets at the IP layer */
  uip_stats_t sent;       /* Number of sent packets at the IP layer */
  uip_stats_t vhlerr;     /* Number of packets dropped due to wrong
                             IP version or header length */
  uip_stats_t hblenerr;   /* Number of packets dropped due to wrong
                             IP length, high byte */
  uip_stats_t lblenerr;   /* Number of packets dropped due to wrong
                             IP length, low byte */
  uip_stats_t fragerr;    /* Number of packets dropped since they
                             were IP fragments */
  uip_stats_t chkerr;     /* Number of packets dropped due to IP
                             checksum errors */
  uip_stats_t protoerr;   /* Number of packets dropped since they
                             were neither ICMP, UDP nor TCP */
};

struct uip_stats
{
  struct uip_ip_stats_s   ip;   /* IP statistics */

#ifdef CONFIG_NET_ICMP
  struct uip_icmp_stats_s icmp; /* ICMP statistics */
#endif

#ifdef CONFIG_NET_TCP
  struct uip_tcp_stats_s  tcp;  /* TCP statistics */
#endif

#ifdef CONFIG_NET_UDP
  struct uip_udp_stats_s  udp;  /* UDP statistics */
#endif
};
#endif /* CONFIG_NET_STATISTICS */

/* Representation of a 48-bit Ethernet address */

struct uip_eth_addr
{
  uint8 addr[6];
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* This is the structure in which the statistics are gathered. */

#ifdef CONFIG_NET_STATISTICS
extern struct uip_stats uip_stat;
#endif

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/* uIP initialization functions
 *
 * The uIP initialization functions are used for booting uIP.
 *
 * This function should be called at boot up to initilize the uIP
 * TCP/IP stack.
 */

extern void uip_initialize(void);

/* This function may be used at boot time to set the initial ip_id.*/

extern void uip_setipid(uint16 id);

/* uIP application functions
 *
 * Functions used by an application running of top of uIP. This includes
 * functions for opening and closing connections, sending and receiving
 * data, etc.
 */

/* Send data on the current connection.
 *
 * This function is used to send out a single segment of TCP
 * data. Only applications that have been invoked by uIP for event
 * processing can send data.
 *
 * The amount of data that actually is sent out after a call to this
 * funcion is determined by the maximum amount of data TCP allows. uIP
 * will automatically crop the data so that only the appropriate
 * amount of data is sent. The function uip_mss() can be used to query
 * uIP for the amount of data that actually will be sent.
 *
 * Note: This function does not guarantee that the sent data will
 * arrive at the destination. If the data is lost in the network, the
 * application will be invoked with the UIP_REXMIT flag set.  The
 * application will then have to resend the data using this function.
 *
 * data A pointer to the data which is to be sent.
 *
 * len The maximum amount of data bytes to be sent.
 */

extern void uip_send(struct uip_driver_s *dev, const void *buf, int len);

/* uIP convenience and converting functions.
 *
 * These functions can be used for converting between different data
 * formats used by uIP.
 *
 * Construct an IP address from four bytes.
 *
 * This function constructs an IPv4 address in network byte order.
 *
 *   addr  A pointer to a uip_ipaddr_t variable that will be
 *         filled in with the IPv4 address.
 *   addr0 The first octet of the IPv4 address.
 *   addr1 The second octet of the IPv4 address.
 *   addr2 The third octet of the IPv4 address.
 *   addr3 The forth octet of the IPv4 address.
 */

#define uip_ipaddr(addr, addr0, addr1, addr2, addr3) \
  do { \
    addr = HTONL((addr0) << 24 | (addr1) << 16 | (addr2) << 8 | (addr3)); \
  } while(0)

/* Convert an IPv4 address of the form uint16[2] to an in_addr_t */

#ifdef CONFIG_ENDIAN_BIG
#  define uip_ip4addr_conv(addr) (((in_addr_t)((uint16*)addr)[0] << 16) | (in_addr_t)((uint16*)addr)[1])
#else
#  define uip_ip4addr_conv(addr) (((in_addr_t)((uint16*)addr)[1] << 16) | (in_addr_t)((uint16*)addr)[0])
#endif

/* Construct an IPv6 address from eight 16-bit words.
 *
 * This function constructs an IPv6 address.
 */

#define uip_ip6addr(addr, addr0,addr1,addr2,addr3,addr4,addr5,addr6,addr7) \
  do { \
    ((uint16*)(addr))[0] = HTONS((addr0)); \
    ((uint16*)(addr))[1] = HTONS((addr1)); \
    ((uint16*)(addr))[2] = HTONS((addr2)); \
    ((uint16*)(addr))[3] = HTONS((addr3)); \
    ((uint16*)(addr))[4] = HTONS((addr4)); \
    ((uint16*)(addr))[5] = HTONS((addr5)); \
    ((uint16*)(addr))[6] = HTONS((addr6)); \
    ((uint16*)(addr))[7] = HTONS((addr7)); \
  } while(0)

/* Copy an IP address to another IP address.
 *
 * Copies an IP address from one place to another.
 *
 * Example:
 *
 *   uip_ipaddr_t ipaddr1, ipaddr2;
 *
 *   uip_ipaddr(&ipaddr1, 192,16,1,2);
 *   uip_ipaddr_copy(&ipaddr2, &ipaddr1);
 *
 * dest The destination for the copy.
 * src The source from where to copy.
 */

#ifndef CONFIG_NET_IPv6
#  define uip_ipaddr_copy(dest, src) \
   do { \
     (dest) = (in_addr_t)(src); \
   } while(0)
#  define uiphdr_ipaddr_copy(dest, src) \
   do { \
     ((uint16*)(dest))[0] = ((uint16*)(src))[0]; \
     ((uint16*)(dest))[1] = ((uint16*)(src))[1]; \
   } while(0)
#else /* !CONFIG_NET_IPv6 */
#  define uip_ipaddr_copy(dest, src)    memcpy(&dest, &src, sizeof(uip_ip6addr_t))
#  define uiphdr_ipaddr_copy(dest, src) uip_ipaddr_copy(dest, src)
#endif /* !CONFIG_NET_IPv6 */

/* Compare two IP addresses
 *
 * Example:
 *
 *   uip_ipaddr_t ipaddr1, ipaddr2;
 *
 *   uip_ipaddr(&ipaddr1, 192,16,1,2);
 *   if(uip_ipaddr_cmp(&ipaddr2, &ipaddr1)) {
 *      printf("They are the same");
 *   }
 *
 * addr1 The first IP address.
 * addr2 The second IP address.
 */

#ifndef CONFIG_NET_IPv6
#  define uip_ipaddr_cmp(addr1, addr2)    (addr1 == addr2)
#  define uiphdr_ipaddr_cmp(addr1, addr2) uip_ipaddr_cmp(uip_ip4addr_conv(addr1), uip_ip4addr_conv(addr2))
#else /* !CONFIG_NET_IPv6 */
#  define uip_ipaddr_cmp(addr1, addr2)    (memcmp(&addr1, &addr2, sizeof(uip_ip6addr_t)) == 0)
#  define uiphdr_ipaddr_cmp(addr1, addr2) uip_ipaddr_cmp(addr, addr2)
#endif /* !CONFIG_NET_IPv6 */

/* Compare two IP addresses with netmasks
 *
 * Compares two IP addresses with netmasks. The masks are used to mask
 * out the bits that are to be compared.
 *
 * Example:
 *
 *   uip_ipaddr_t ipaddr1, ipaddr2, mask;
 *
 *   uip_ipaddr(&mask, 255,255,255,0);
 *   uip_ipaddr(&ipaddr1, 192,16,1,2);
 *   uip_ipaddr(&ipaddr2, 192,16,1,3);
 *   if(uip_ipaddr_maskcmp(ipaddr1, ipaddr2, &mask))
 *     {
 *       printf("They are the same");
 *     }
 *
 * addr1 The first IP address.
 * addr2 The second IP address.
 * mask The netmask.
 */

#ifndef CONFIG_NET_IPv6
#  define uip_ipaddr_maskcmp(addr1, addr2, mask) \
  (((in_addr_t)(addr1) & (in_addr_t)(mask)) == \
   ((in_addr_t)(addr2) & (in_addr_t)(mask)))
#else
extern boolean uip_ipaddr_maskcmp(uip_addr_t addr1, uip_addr_t addr2,
                                  uip_addr_t mask);
#endif

/* Mask out the network part of an IP address.
 *
 * Masks out the network part of an IP address, given the address and
 * the netmask.
 *
 * Example:
 *
 *   uip_ipaddr_t ipaddr1, ipaddr2, netmask;
 *
 *   uip_ipaddr(&ipaddr1, 192,16,1,2);
 *   uip_ipaddr(&netmask, 255,255,255,0);
 *   uip_ipaddr_mask(&ipaddr2, &ipaddr1, &netmask);
 *
 * In the example above, the variable "ipaddr2" will contain the IP
 * address 192.168.1.0.
 *
 * dest Where the result is to be placed.
 * src The IP address.
 * mask The netmask.
 */

#define uip_ipaddr_mask(dest, src, mask) \
  do { \
    (in_addr_t)(dest) = (in_addr_t)(src) & (in_addr_t)(mask); \
  } while(0)

#endif /* __NET_UIP_UIP_H */
