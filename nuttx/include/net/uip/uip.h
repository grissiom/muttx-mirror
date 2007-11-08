/****************************************************************************
 * uip.h
 * Header file for the uIP TCP/IP stack.
 *
 * The uIP TCP/IP stack header file contains definitions for a number
 * of C macros that are used by uIP programs as well as internal uIP
 * structures, TCP/IP header structures and function declarations.
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
                                 * the last data. */
#define UIP_NEWDATA    (1 << 1) /* Flags the fact that the peer has sent us new data. */
#define UIP_REXMIT     (1 << 2) /* Tells the application to retransmit the data that was last
                                 * sent. */
#define UIP_POLL       (1 << 3) /* Used for polling the application, to check if the application
                                 * has data that it wants to send. */
#define UIP_CLOSE      (1 << 4) /* The remote host has closed the connection, thus the connection
                                 * has gone away. Or the application signals that it wants to
                                 * close the connection. */
#define UIP_ABORT      (1 << 5) /* The remote host has aborted the connection, thus the connection
                                 * has gone away. Or the application signals that it wants to
                                 * abort the connection. */
#define UIP_CONNECTED  (1 << 6) /* We have got a connection from a remote host and have set up a
                                 * new connection for it, or an active connection has been
                                 * successfully established. */
#define UIP_TIMEDOUT   (1 << 7) /* The connection has been aborted due to too many retransmissions. */

#define UIP_DATA_EVENTS (UIP_ACKDATA|UIP_NEWDATA|UIP_REXMIT|UIP_POLL)
#define UIP_CONN_EVENTS (UIP_CLOSE|UIP_ABORT|UIP_CONNECTED|UIP_TIMEDOUT)

/* The TCP states used in the struct uip_conn tcpstateflags field. */

#define UIP_CLOSED      0 /* The connection is not in use and available */
#define UIP_ALLOCATED   1 /* The connection is allocated, but not yet initialized */
#define UIP_SYN_RCVD    2
#define UIP_SYN_SENT    3
#define UIP_ESTABLISHED 4
#define UIP_FIN_WAIT_1  5
#define UIP_FIN_WAIT_2  6
#define UIP_CLOSING     7
#define UIP_TIME_WAIT   8
#define UIP_LAST_ACK    9

#define UIP_TS_MASK     15
#define UIP_STOPPED     16

/* The buffer size available for user data in the d_buf buffer.
 *
 * This macro holds the available size for user data in the \ref
 * d_buf buffer. The macro is intended to be used for checking
 * bounds of available user data.
 *
 * Example:
 *
 *   snprintf(dev->d_appdata, UIP_APPDATA_SIZE, "%u\n", i);
 */

#define UIP_APPDATA_SIZE (UIP_BUFSIZE - UIP_LLH_LEN - UIP_TCPIP_HLEN)

#define UIP_PROTO_ICMP  1
#define UIP_PROTO_TCP   6
#define UIP_PROTO_UDP   17
#define UIP_PROTO_ICMP6 58

/* Header sizes */

#ifdef CONFIG_NET_IPv6
# define UIP_IPH_LEN    40
#else /* CONFIG_NET_IPv6 */
# define UIP_IPH_LEN    20    /* Size of IP header */
#endif /* CONFIG_NET_IPv6 */

#define UIP_UDPH_LEN    8     /* Size of UDP header */
#define UIP_TCPH_LEN    20    /* Size of TCP header */
#define UIP_IPUDPH_LEN (UIP_UDPH_LEN + UIP_IPH_LEN)    /* Size of IP + UDP header */
#define UIP_IPTCPH_LEN (UIP_TCPH_LEN + UIP_IPH_LEN)    /* Size of IP + TCP header */
#define UIP_TCPIP_HLEN UIP_IPTCPH_LEN

/****************************************************************************
 * Public Type Definitions
 ****************************************************************************/

/* Repressentation of an IP address. */

typedef in_addr_t uip_ip4addr_t;
typedef uint16 uip_ip6addr_t[8];

#ifdef CONFIG_NET_IPv6
typedef uip_ip6addr_t uip_ipaddr_t;
#else /* CONFIG_NET_IPv6 */
typedef uip_ip4addr_t uip_ipaddr_t;
#endif /* CONFIG_NET_IPv6 */

/* Representation of a uIP TCP connection.
 *
 * The uip_conn structure is used for identifying a connection. All
 * but one field in the structure are to be considered read-only by an
 * application. The only exception is the "private: field whos purpose
 * is to let the application store application-specific state (e.g.,
 * file pointers) for the connection.
 */

struct uip_driver_s;      /* Forward reference */
struct uip_conn
{
  dq_entry_t node;        /* Implements a doubly linked list */
  uip_ipaddr_t ripaddr;   /* The IP address of the remote host. */
  uint16 lport;           /* The local TCP port, in network byte order. */
  uint16 rport;           /* The local remote TCP port, in network byte order. */
  uint8  rcv_nxt[4];      /* The sequence number that we expect to
                           * receive next. */
  uint8  snd_nxt[4];      /* The sequence number that was last sent by us. */
  uint16 len;             /* Length of the data that was previously sent. */
  uint16 mss;             /* Current maximum segment size for the
                           * connection. */
  uint16 initialmss;      /* Initial maximum segment size for the
                           * connection. */
  uint8  sa;              /* Retransmission time-out calculation state
                           * variable. */
  uint8  sv;              /* Retransmission time-out calculation state
                           * variable. */
  uint8  rto;             /* Retransmission time-out. */
  uint8  tcpstateflags;   /* TCP state and flags. */
  uint8  timer;           /* The retransmission timer (units: half-seconds). */
  uint8  nrtx;            /* The number of retransmissions for the last
                           * segment sent. */

  /* Higher level logic can retain application specific information
   * in the following:
   *
   *   data_event() is called on all events.  May return one of the following:
   *      UIP_CLOSE - Gracefully close the current connection
   *      UIP_ABORT - Abort (reset) the current connection on an error that
   *                  prevents UIP_CLOSE from working.
   *   accept() is called when the TCP logic has created a connection
   *   connection_event() is called on any of the subset of connection-related events
   */

  void *data_private;
  uint8 (*data_event)(struct uip_driver_s *dev, struct uip_conn *conn, uint8 flags);

  void *accept_private;
  int (*accept)(struct uip_conn *listener, struct uip_conn *conn);

  void *connection_private;
  void (*connection_event)(struct uip_conn *conn, uint8 flags);
};

#ifdef CONFIG_NET_UDP
/* Representation of a uIP UDP connection. */

struct uip_udp_conn
{
  dq_entry_t node;        /* Implements a doubly linked list */
  uip_ipaddr_t ripaddr;   /* The IP address of the remote peer. */
  uint16 lport;           /* The local port number in network byte order. */
  uint16 rport;           /* The remote port number in network byte order. */
  uint8  ttl;             /* Default time-to-live. */

  /* Defines the UDP callback */

  void *private;
  void (*event)(struct uip_driver_s *dev, struct uip_udp_conn *conn, uint8 flags);
};
#endif  /* CONFIG_NET_UDP */

/* The structure holding the TCP/IP statistics that are gathered if
 * CONFIG_NET_STATISTICS is defined.
 */

struct uip_stats
{
  struct
  {
    uip_stats_t drop;       /* Number of dropped packets at the IP layer. */
    uip_stats_t recv;       /* Number of received packets at the IP layer. */
    uip_stats_t sent;       /* Number of sent packets at the IP layer. */
    uip_stats_t vhlerr;     /* Number of packets dropped due to wrong
                               IP version or header length. */
    uip_stats_t hblenerr;   /* Number of packets dropped due to wrong
                               IP length, high byte. */
    uip_stats_t lblenerr;   /* Number of packets dropped due to wrong
                               IP length, low byte. */
    uip_stats_t fragerr;    /* Number of packets dropped since they
                               were IP fragments. */
    uip_stats_t chkerr;     /* Number of packets dropped due to IP
                               checksum errors. */
    uip_stats_t protoerr;   /* Number of packets dropped since they
                               were neither ICMP, UDP nor TCP. */
  } ip;                     /* IP statistics. */

  struct
  {
    uip_stats_t drop;       /* Number of dropped ICMP packets. */
    uip_stats_t recv;       /* Number of received ICMP packets. */
    uip_stats_t sent;       /* Number of sent ICMP packets. */
    uip_stats_t typeerr;    /* Number of ICMP packets with a wrong type. */
  } icmp;                   /* ICMP statistics. */

  struct

  {
    uip_stats_t drop;       /* Number of dropped TCP segments. */
    uip_stats_t recv;       /* Number of recived TCP segments. */
    uip_stats_t sent;       /* Number of sent TCP segments. */
    uip_stats_t chkerr;     /* Number of TCP segments with a bad checksum. */
    uip_stats_t ackerr;     /* Number of TCP segments with a bad ACK number. */
    uip_stats_t rst;        /* Number of recevied TCP RST (reset) segments. */
    uip_stats_t rexmit;     /* Number of retransmitted TCP segments. */
    uip_stats_t syndrop;    /* Number of dropped SYNs due to too few
                               connections was avaliable. */
    uip_stats_t synrst;     /* Number of SYNs for closed ports, triggering a RST. */
  } tcp;                    /* TCP statistics. */

#ifdef CONFIG_NET_UDP
  struct
  {
    uip_stats_t drop;       /* Number of dropped UDP segments. */
    uip_stats_t recv;       /* Number of recived UDP segments. */
    uip_stats_t sent;       /* Number of sent UDP segments. */
    uip_stats_t chkerr;     /* Number of UDP segments with a bad checksum. */
  } udp;                    /* UDP statistics. */
#endif  /* CONFIG_NET_UDP */
};

/* The TCP and IP headers. */

struct uip_tcpip_hdr
{
#ifdef CONFIG_NET_IPv6

  /* IPv6 Ip header. */

  uint8  vtc;               /* Bits 0-3: version, bits 4-7: traffic class (MS) */
  uint8  tcf;               /* Bits 0-3: traffic class (LS), 4-bits: flow label (MS) */
  uint16 flow;              /* 16-bit flow label (LS) */
  uint8  len[2];            /* 16-bit Payload length */
  uint8  proto;             /*  8-bit Next header (same as IPv4 protocol field) */
  uint8  ttl;               /*  8-bit Hop limit (like IPv4 TTL field) */
  uip_ip6addr_t srcipaddr;  /* 128-bit Source address */
  uip_ip6addr_t destipaddr; /* 128-bit Destination address */

#else /* CONFIG_NET_IPv6 */

  /* IPv4 IP header. */

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

  /* TCP header. */

  uint16 srcport;
  uint16 destport;
  uint8  seqno[4];
  uint8  ackno[4];
  uint8  tcpoffset;
  uint8  flags;
  uint8  wnd[2];
  uint16 tcpchksum;
  uint8  urgp[2];
  uint8  optdata[4];
};

/* The ICMP and IP headers. */

struct uip_icmpip_hdr
{
#ifdef CONFIG_NET_IPv6

  /* IPv6 Ip header. */

  uint8  vtc;               /* Bits 0-3: version, bits 4-7: traffic class (MS) */
  uint8  tcf;               /* Bits 0-3: traffic class (LS), bits 4-7: flow label (MS) */
  uint16 flow;              /* 16-bit flow label (LS) */
  uint8  len[2];            /* 16-bit Payload length */
  uint8  proto;             /*  8-bit Next header (same as IPv4 protocol field) */
  uint8  ttl;               /*  8-bit Hop limit (like IPv4 TTL field) */
  uip_ip6addr_t srcipaddr;  /* 128-bit Source address */
  uip_ip6addr_t destipaddr; /* 128-bit Destination address */

#else /* CONFIG_NET_IPv6 */

  /* IPv4 IP header. */

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

  /* ICMP (echo) header. */

  uint8  type;
  uint8  icode;
  uint16 icmpchksum;

#ifndef CONFIG_NET_IPv6

  uint16 id;
  uint16 seqno;

#else /* !CONFIG_NET_IPv6 */

  uint8 flags;
  uint8 reserved1;
  uint8 reserved2;
  uint8 reserved3;
  uint8 icmp6data[16];
  uint8 options[1];

#endif /* !CONFIG_NET_IPv6 */
};

/* The UDP and IP headers. */

struct uip_udpip_hdr
{
#ifdef CONFIG_NET_IPv6

  /* IPv6 Ip header. */

  uint8  vtc;               /* Bits 0-3: version, bits 4-7: traffic class (MS) */
  uint8  tcf;               /* Bits 0-3: traffic class (LS), 4-bits: flow label (MS) */
  uint16 flow;              /* 16-bit flow label (LS) */
  uint8  len[2];            /* 16-bit Payload length */
  uint8  proto;             /*  8-bit Next header (same as IPv4 protocol field) */
  uint8  ttl;               /*  8-bit Hop limit (like IPv4 TTL field) */
  uip_ip6addr_t srcipaddr;  /* 128-bit Source address */
  uip_ip6addr_t destipaddr; /* 128-bit Destination address */

#else /* CONFIG_NET_IPv6 */

  /* IPv4 header. */

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

  /* UDP header. */

  uint16 srcport;
  uint16 destport;
  uint16 udplen;
  uint16 udpchksum;
};

/* Representation of a 48-bit Ethernet address. */

struct uip_eth_addr
{
  uint8 addr[6];
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

#if UIP_URGDATA > 0
/* uint8 *uip_urgdata:
 *
 * This pointer points to any urgent data that has been received. Only
 * present if compiled with support for urgent data (UIP_URGDATA).
 */
extern void *uip_urgdata;
#endif /* UIP_URGDATA > 0 */


/* Variables used in uIP device drivers
 *
 * uIP has a few global variables that are used in device drivers for
 * uIP.
 */

#if UIP_URGDATA > 0
extern uint16 uip_urglen; /* Length of (received) urgent data */
#endif /* UIP_URGDATA > 0 */

/* The current UDP connection. */

#ifdef CONFIG_NET_UDP
extern struct uip_udp_conn *uip_udp_conn;
#endif  /* CONFIG_NET_UDP */

/* The uIP TCP/IP statistics.
 *
 * This is the variable in which the uIP TCP/IP statistics are gathered.
 */

extern struct uip_stats uip_stat;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/* uIP configuration functions */

/* uIP initialization functions
 *
 * The uIP initialization functions are used for booting uIP.
 *
 * This function should be called at boot up to initilize the uIP
 * TCP/IP stack.
 */

void uip_initialize(void);

/* This function may be used at boot time to set the initial ip_id.*/

void uip_setipid(uint16 id);

/* uIP application functions
 *
 * Functions used by an application running of top of uIP. This includes
 * functions for opening and closing connections, sending and receiving
 * data, etc.
 *
 * Find a free connection structure and allocate it for use. This is
 * normally something done by the implementation of the socket() API
 */

extern struct uip_conn *uip_tcpalloc(void);
#ifdef CONFIG_NET_UDP
extern struct uip_udp_conn *uip_udpalloc(void);
#endif

/* Free a connection structure that is no longer in use. This should
 * be done by the implementation of close()
 */

extern void uip_tcpfree(struct uip_conn *conn);
#ifdef CONFIG_NET_UDP
extern void uip_udpfree(struct uip_udp_conn *conn);
#endif

/* Bind a TCP connection to a local address */

#ifdef CONFIG_NET_IPv6
extern int uip_tcpbind(struct uip_conn *conn, const struct sockaddr_in6 *addr);
#else
extern int uip_tcpbind(struct uip_conn *conn, const struct sockaddr_in *addr);
#endif

/* This function implements the UIP specific parts of the standard
 * TCP connect() operation:  It connects to a remote host using TCP.
 *
 * This function is used to start a new connection to the specified
 * port on the specied host. It uses the connection structure that was
 * allocated by a preceding socket() call.  It sets the connection to
 * the SYN_SENT state and sets the retransmission timer to 0. This will
 * cause a TCP SYN segment to be sent out the next time this connection
 * is periodically processed, which usually is done within 0.5 seconds
 * after the call to uip_tcpconnect().
 *
 * This function is called from normal user level code.
 */

#ifdef CONFIG_NET_IPv6
extern int uip_tcpconnect(struct uip_conn *conn, const struct sockaddr_in6 *addr);
#else
extern int uip_tcpconnect(struct uip_conn *conn, const struct sockaddr_in *addr);
#endif

/* Start listening to the specified port.
 *
 * Note: Since this function expects the port number in network byte
 * order, a conversion using HTONS() or htons() is necessary.
 *
 * port A 16-bit port number in network byte order.
 */

int uip_listen(uint16 port);

/* Stop listening to the specified port.
 *
 * Note: Since this function expects the port number in network byte
 * order, a conversion using HTONS() or htons() is necessary.
 *
 * port A 16-bit port number in network byte order.
 */

int uip_unlisten(uint16 port);

/* Check if a connection has outstanding (i.e., unacknowledged) data. */

#define uip_outstanding(conn) ((conn)->len)

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
 * application will be invoked with the uip_rexmit_event() event being
 * set. The application will then have to resend the data using this
 * function.
 *
 * data A pointer to the data which is to be sent.
 *
 * len The maximum amount of data bytes to be sent.
 */

void uip_send(struct uip_driver_s *dev, const void *buf, int len);

/* The length of any incoming data that is currently avaliable (if avaliable)
 * in the d_appdata buffer.
 *
 * The test function uip_data() must first be used to check if there
 * is any data available at all.
 */

#define uip_datalen(dev)    ((dev)->d_len)

/* The length of any out-of-band data (urgent data) that has arrived
 * on the connection.
 *
 * Note: The configuration parameter UIP_URGDATA must be set for this
 * function to be enabled.
 */

#define uip_urgdatalen()    uip_urglen

/* Tell the sending host to stop sending data.
 *
 * This function will close our receiver's window so that we stop
 * receiving data for the current connection.
 */

#define uip_stop(conn)          ((conn)->tcpstateflags |= UIP_STOPPED)

/* Find out if the current connection has been previously stopped with
 * uip_stop().
 */

#define uip_stopped(conn)   ((conn)->tcpstateflags & UIP_STOPPED)

/* Restart the current connection, if is has previously been stopped
 * with uip_stop().
 *
 * This function will open the receiver's window again so that we
 * start receiving data for the current connection.
 */

#define uip_restart(conn,f) \
  do { \
    (f) |= UIP_NEWDATA; \
    (conn)->tcpstateflags &= ~UIP_STOPPED; \
  } while(0)


/* uIP tests that can be made to determine in what state the current
 * connection is, and what the application function should do.
 *
 * Is new incoming data available?
 *
 * Will reduce to non-zero if there is new data for the application
 * present at the d_appdata pointer. The size of the data is
 * avaliable through the d_len element.
 */

#define uip_newdata_event(f) ((f) & UIP_NEWDATA)

/* Has previously sent data been acknowledged?
 *
 * Will reduce to non-zero if the previously sent data has been
 * acknowledged by the remote host. This means that the application
 * can send new data.
 */

#define uip_ack_event(f) ((f) & UIP_ACKDATA)

/* Has the connection just been connected?
 *
 * Reduces to non-zero if the current connenetutils/telnetd/telnetd.cction has been connected to
 * a remote host. This will happen both if the connection has been
 * actively opened (with uip_connect()) or passively opened (with
 * uip_listen()).
 */

#define uip_connected_event(f) ((f) & UIP_CONNECTED)

/* Has the connection been closed by the other end?
 *
 * Is non-zero if the connection has been closed by the remote
 * host. The application may then do the necessary clean-ups.
 */

#define uip_close_event(f) ((f) & UIP_CLOSE)

/* Has the connection been aborted by the other end?
 *
 * Non-zero if the current connection has been aborted (reset) by the
 * remote host.
 */

#define uip_abort_event(f) ((f) & UIP_ABORT)

/* Has the connection timed out?
 *
 * Non-zero if the current connection has been aborted due to too many
 * retransmissions.
 */

#define uip_timeout_event(f) ((f) & UIP_TIMEDOUT)

/* Do we need to retransmit previously data?
 *
 * Reduces to non-zero if the previously sent data has been lost in
 * the network, and the application should retransmit it. The
 * application should send the exact same data as it did the last
 * time, using the uip_send() function.
 */

#define uip_rexmit_event(f) ((f) & UIP_REXMIT)

/* Is the connection being polled by uIP?
 *
 * Is non-zero if the reason the application is invoked is that the
 * current connection has been idle for a while and should be
 * polled.
 *
 * The polling event can be used for sending data without having to
 * wait for the remote host to send data.
 */

#define uip_poll_event(f) ((f) & UIP_POLL)

/* Get the initial maxium segment size (MSS) of the current
 * connection.
 */

#define uip_initialmss(conn) ((conn)->initialmss)

/* Get the current maxium segment size that can be sent on the current
 * connection.
 *
 * The current maxiumum segment size that can be sent on the
 * connection is computed from the receiver's window and the MSS of
 * the connection (which also is available by calling
 * uip_initialmss()).
 */

#define uip_mss(conn) ((conn)->mss)

/* Bind a UDP connection to a local address */

#ifdef CONFIG_NET_IPv6
extern int uip_udpbind(struct uip_udp_conn *conn, const struct sockaddr_in6 *addr);
#else
extern int uip_udpbind(struct uip_udp_conn *conn, const struct sockaddr_in *addr);
#endif

/* This function sets up a new UDP connection. The function will
 * automatically allocate an unused local port for the new
 * connection. However, another port can be chosen by using the
 * uip_udpbind() call, after the uip_udpconnect() function has been
 * called.
 *
 * This function is called as part of the implementation of sendto
 * and recvfrom.
 *
 * addr The address of the remote host.
 */

#ifdef CONFIG_NET_IPv6
extern int uip_udpconnect(struct uip_udp_conn *conn, const struct sockaddr_in6 *addr);
#else
extern int uip_udpconnect(struct uip_udp_conn *conn, const struct sockaddr_in *addr);
#endif

/* Enable/disable UDP callbacks on a connection */

extern void uip_udpenable(struct uip_udp_conn *conn);
extern void uip_udpdisable(struct uip_udp_conn *conn);

/* uIP convenience and converting functions.
 *
 * These functions can be used for converting between different data
 * formats used by uIP.
 *
 * Construct an IP address from four bytes.
 *
 * This function constructs an IP address of the type that uIP handles
 * internally from four bytes. The function is handy for specifying IP
 * addresses to use with e.g. the uip_connect() function.
 *
 * Example:
 *
 *   uip_ipaddr_t ipaddr;
 *   struct uip_conn *c;
 *
 *   uip_ipaddr(&ipaddr, 192,168,1,2);
 *   c = uip_connect(&ipaddr, HTONS(80));
 *
 * addr A pointer to a uip_ipaddr_t variable that will be
 * filled in with the IP address.
 *
 * addr0 The first octet of the IP address.
 * addr1 The second octet of the IP address.
 * addr2 The third octet of the IP address.
 * addr3 The forth octet of the IP address.
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

#define uip_ip6addr(addr, addr0,addr1,addr2,addr3,addr4,addr5,addr6,addr7) do { \
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
 *   if(uip_ipaddr_maskcmp(ipaddr1, ipaddr2, &mask)) {
 *      printf("They are the same");
 *   }
 *
 * addr1 The first IP address.
 * addr2 The second IP address.
 * mask The netmask.
 */

#ifndef CONFIG_NET_IPv6
#  define uip_ipaddr_maskcmp(addr1, addr2, mask) \
  (((in_addr_t)(addr1) & (in_addr_t)(mask)) == ((in_addr_t)(addr2) & (in_addr_t)(mask)))
#else /* !CONFIG_NET_IPv6 */
#endif /* !CONFIG_NET_IPv6 */

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
