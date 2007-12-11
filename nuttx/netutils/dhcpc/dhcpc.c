/****************************************************************************
 * netutils/dhcpc/dhcpc.c
 *
 *   Copyright (C) 2007 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <spudmonkey@racsa.co.cr>
 *
 * Based heavily on portions of uIP:
 *
 *   Copyright (c) 2005, Swedish Institute of Computer Science
 *   All rights reserved.
 *
 * Copyright (c) 2005, Swedish Institute of Computer Science
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <sys/types.h>
#include <sys/socket.h>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <debug.h>

#include <net/uip/uip.h>
#include <net/uip/dhcpc.h>
#include <net/uip/uip-lib.h>

/****************************************************************************
 * Definitions
 ****************************************************************************/

#define STATE_INITIAL           0
#define STATE_HAVE_OFFER        1
#define STATE_HAVE_LEASE        2

#define BOOTP_BROADCAST         0x8000

#define DHCP_REQUEST            1
#define DHCP_REPLY              2
#define DHCP_HTYPE_ETHERNET     1
#define DHCP_HLEN_ETHERNET      6
#define DHCP_MSG_LEN            236

#define DHCPC_SERVER_PORT       67
#define DHCPC_CLIENT_PORT       68

#define DHCPDISCOVER            1
#define DHCPOFFER               2
#define DHCPREQUEST             3
#define DHCPDECLINE             4
#define DHCPACK                 5
#define DHCPNAK                 6
#define DHCPRELEASE             7

#define DHCP_OPTION_SUBNET_MASK 1
#define DHCP_OPTION_ROUTER      3
#define DHCP_OPTION_DNS_SERVER  6
#define DHCP_OPTION_REQ_IPADDR  50
#define DHCP_OPTION_LEASE_TIME  51
#define DHCP_OPTION_MSG_TYPE    53
#define DHCP_OPTION_SERVER_ID   54
#define DHCP_OPTION_REQ_LIST    55
#define DHCP_OPTION_END         255

#define BUFFER_SIZE             256

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct dhcp_msg
{
  uint8  op;
  uint8  htype;
  uint8  hlen;
  uint8  hops;
  uint8  xid[4];
  uint16 secs;
  uint16 flags;
  uint8  ciaddr[4];
  uint8  yiaddr[4];
  uint8  siaddr[4];
  uint8  giaddr[4];
  uint8  chaddr[16];
#ifndef CONFIG_NET_DHCP_LIGHT
  uint8  sname[64];
  uint8  file[128];
#endif
  uint8  options[312];
};

struct dhcpc_state_s
{
  struct uip_udp_conn *ds_conn;
  const void          *ds_macaddr;
  int                  ds_maclen;
  int                  sockfd;
  struct in_addr       serverid;
  struct dhcp_msg      packet;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const uint8 xid[4]          = {0xad, 0xde, 0x12, 0x23};
static const uint8 magic_cookie[4] = {99, 130, 83, 99};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: dhcpc_add<option>
 ****************************************************************************/

static uint8 *dhcpc_addmsgtype(uint8 *optptr, uint8 type)
{
  *optptr++ = DHCP_OPTION_MSG_TYPE;
  *optptr++ = 1;
  *optptr++ = type;
  return optptr;
}

static uint8 *dhcpc_addserverid(struct in_addr *serverid, uint8 *optptr)
{
  *optptr++ = DHCP_OPTION_SERVER_ID;
  *optptr++ = 4;
  memcpy(optptr, &serverid->s_addr, 4);
  return optptr + 4;
}

static uint8 *dhcpc_addreqipaddr(struct dhcpc_state *presult, uint8 *optptr)
{
  *optptr++ = DHCP_OPTION_REQ_IPADDR;
  *optptr++ = 4;
  memcpy(optptr, &presult->ipaddr.s_addr, 4);
  return optptr + 4;
}

static uint8 *dhcpc_addreqoptions(uint8 *optptr)
{
  *optptr++ = DHCP_OPTION_REQ_LIST;
  *optptr++ = 3;
  *optptr++ = DHCP_OPTION_SUBNET_MASK;
  *optptr++ = DHCP_OPTION_ROUTER;
  *optptr++ = DHCP_OPTION_DNS_SERVER;
  return optptr;
}

static uint8 *dhcpc_addend(uint8 *optptr)
{
  *optptr++ = DHCP_OPTION_END;
  return optptr;
}

/****************************************************************************
 * Name: dhcpc_sendmsg
 ****************************************************************************/

static int dhcpc_sendmsg(struct dhcpc_state_s *pdhcpc,
                         struct dhcpc_state *presult, int msgtype)
{
  struct sockaddr_in addr;
  uint8 *pend;
  in_addr_t serverid = INADDR_BROADCAST;
  int len;

  /* Create the common message header settings */

  memset(&pdhcpc->packet, 0, sizeof(struct dhcp_msg));
  pdhcpc->packet.op    = DHCP_REQUEST;
  pdhcpc->packet.htype = DHCP_HTYPE_ETHERNET;
  pdhcpc->packet.hlen  = pdhcpc->ds_maclen;
  memcpy(pdhcpc->packet.xid, xid, 4);
  memcpy(pdhcpc->packet.chaddr, pdhcpc->ds_macaddr, pdhcpc->ds_maclen);
  memset(&pdhcpc->packet.chaddr[pdhcpc->ds_maclen], 0, 16 - pdhcpc->ds_maclen);
  memcpy(pdhcpc->packet.options, magic_cookie, sizeof(magic_cookie));

  /* Add the common header options */

  pend = &pdhcpc->packet.options[4];
  pend = dhcpc_addmsgtype(pend, msgtype);

  /* Handle the message specific settings */

  switch (msgtype)
    {
      /* Broadcast DISCOVER message to all servers */

      case DHCPDISCOVER:
        pdhcpc->packet.flags = HTONS(BOOTP_BROADCAST); /*  Broadcast bit. */
        pend     = dhcpc_addreqoptions(pend);
        break;

      /* Send REQUEST message to the server that send the OFFER */

      case DHCPREQUEST:
        pdhcpc->packet.flags = HTONS(BOOTP_BROADCAST); /*  Broadcast bit. */
        memcpy(pdhcpc->packet.ciaddr, &presult->ipaddr.s_addr, 4);
        pend     = dhcpc_addserverid(&pdhcpc->serverid, pend);
        pend     = dhcpc_addreqipaddr(presult, pend);
        break;

      /* Send DECLINE message to the server that sent the OFFER */

      case DHCPDECLINE:
        memcpy(pdhcpc->packet.ciaddr, &presult->ipaddr.s_addr, 4);
        pend     = dhcpc_addserverid(&presult->serverid, pend);
        serverid = presult->serverid.s_addr;
        break;

      default:
        return ERROR;
    }

  pend = dhcpc_addend(pend);
  len  = pend - (uint8*)&pdhcpc->packet;

  /* Send the request */

  addr.sin_family      = AF_INET;
  addr.sin_port        = HTONS(DHCPC_SERVER_PORT);
  addr.sin_addr.s_addr = serverid;

  return sendto(pdhcpc->sockfd, &pdhcpc->packet, len, 0,
                (struct sockaddr*)&addr, sizeof(struct sockaddr_in));
}

/****************************************************************************
 * Name: dhcpc_parseoptions
 ****************************************************************************/

static uint8 dhcpc_parseoptions(struct dhcpc_state *presult, uint8 *optptr, int len)
{
  uint8 *end = optptr + len;
  uint8 type = 0;

  while (optptr < end)
    {
      switch(*optptr)
        {
          case DHCP_OPTION_SUBNET_MASK:
            memcpy(&presult->netmask.s_addr, optptr + 2, 4);
            break;
          case DHCP_OPTION_ROUTER:
            memcpy(&presult->default_router.s_addr, optptr + 2, 4);
            break;
          case DHCP_OPTION_DNS_SERVER:
            memcpy(&presult->dnsaddr.s_addr, optptr + 2, 4);
            break;
          case DHCP_OPTION_MSG_TYPE:
            type = *(optptr + 2);
            break;
          case DHCP_OPTION_SERVER_ID:
            memcpy(&presult->serverid.s_addr, optptr + 2, 4);
            break;
          case DHCP_OPTION_LEASE_TIME:
            memcpy(presult->lease_time, optptr + 2, 4);
            break;
          case DHCP_OPTION_END:
            return type;
        }

      optptr += optptr[1] + 2;
    }
  return type;
}

/****************************************************************************
 * Name: dhcpc_parsemsg
 ****************************************************************************/

static uint8 dhcpc_parsemsg(struct dhcpc_state_s *pdhcpc, int buflen,
                            struct dhcpc_state *presult)
{
  if (pdhcpc->packet.op == DHCP_REPLY &&
      memcmp(pdhcpc->packet.xid, xid, sizeof(xid)) == 0 &&
      memcmp(pdhcpc->packet.chaddr, pdhcpc->ds_macaddr, pdhcpc->ds_maclen) == 0)
    {
      memcpy(&presult->ipaddr.s_addr, pdhcpc->packet.yiaddr, 4);
      return dhcpc_parseoptions(presult, &pdhcpc->packet.options[4], buflen);
    }
  return 0;
}

/****************************************************************************
 * Global Functions
 ****************************************************************************/

/****************************************************************************
 * Name: dhcpc_open
 ****************************************************************************/

void *dhcpc_open(const void *macaddr, int maclen)
{
  struct dhcpc_state_s *pdhcpc;
  struct sockaddr_in addr;
  struct timeval tv;

  dbg("MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
      ((uint8*)macaddr)[0], ((uint8*)macaddr)[1], ((uint8*)macaddr)[2],
      ((uint8*)macaddr)[3], ((uint8*)macaddr)[4], ((uint8*)macaddr)[5]);

  /* Allocate an internal DHCP structure */

  pdhcpc = (struct dhcpc_state_s *)malloc(sizeof(struct dhcpc_state_s));
  if (pdhcpc)
    {
      /* Initialize the allocated structure */

      memset(pdhcpc, 0, sizeof(struct dhcpc_state_s));
      pdhcpc->ds_macaddr = macaddr;
      pdhcpc->ds_maclen  = maclen;

      /* Create a UDP socket */

      pdhcpc->sockfd    = socket(PF_INET, SOCK_DGRAM, 0);
      if (pdhcpc->sockfd < 0)
        {
          free(pdhcpc);
          return NULL;
        }

      /* bind the socket */

      addr.sin_family      = AF_INET;
      addr.sin_port        = HTONS(DHCPC_CLIENT_PORT);
      addr.sin_addr.s_addr = INADDR_ANY;

      if (bind(pdhcpc->sockfd, (struct sockaddr*)&addr, sizeof(struct sockaddr_in)) < 0)
        {
          close(pdhcpc->sockfd);
          free(pdhcpc);
          return NULL;
        }

      /* Configure for read timeouts */

      tv.tv_sec  = 10;
      tv.tv_usec = 0;
      if (setsockopt(pdhcpc->sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(struct timeval)) < 0)
        {
          close(pdhcpc->sockfd);
          free(pdhcpc);
          return NULL;
        }
    }

  return (void*)pdhcpc;
}

/****************************************************************************
 * Name: dhcpc_close
 ****************************************************************************/

void dhcpc_close(void *handle)
{
  struct dchcpc_state_internal *pdhcpc = (struct dchcpc_state_internal *)handle;
  if (pdhcpc)
    {
      free(pdhcpc);
    }
}

/****************************************************************************
 * Name: dhcpc_request
 ****************************************************************************/

int dhcpc_request(void *handle, struct dhcpc_state *presult)
{
  struct dhcpc_state_s *pdhcpc = (struct dhcpc_state_s *)handle;
  struct in_addr oldaddr;
  struct in_addr newaddr;
  ssize_t result;
  uint8   msgtype;
  int     retries;
  int     state;

  /* Save the currently assigned IP address (should be INADDR_ANY) */

  oldaddr.s_addr = 0;
  uip_gethostaddr("eth0", &oldaddr);

  /* Loop until we receive the lease (or an error occurs) */

  do
    {
      /* Set the IP address to INADDR_ANY. */

      newaddr.s_addr = INADDR_ANY;
      (void)uip_sethostaddr("eth0", &newaddr);

      /* Loop sending DISCOVER until we receive an OFFER from a DHCP
       * server.  We will lock on to the first OFFER and decline any
       * subsequent offers (which will happen if there are more than one
       * DHCP servers on the network.
       */

      state = STATE_INITIAL;
      do
        {
          /* Send the DISCOVER command */

          dbg("Broadcast DISCOVER\n");
          if (dhcpc_sendmsg(pdhcpc, presult, DHCPDISCOVER) < 0)
            {
              return ERROR;
            }

          /* Get the DHCPOFFER response */

          result = recv(pdhcpc->sockfd, &pdhcpc->packet, sizeof(struct dhcp_msg), 0);
          if (result >= 0)
            {
              msgtype = dhcpc_parsemsg(pdhcpc, result, presult);
              if (msgtype == DHCPOFFER)
                {
                  /* Save the servid from the presult so that it is not clobbered
                   * by a new OFFER.
                   */

                  dbg("Received OFFER from %08x\n", ntohl(presult->serverid.s_addr));
		  pdhcpc->serverid.s_addr = presult->serverid.s_addr;

                  /* Temporarily use the address offered by the server and break
                   * out of the loop.
                   */

                  (void)uip_sethostaddr("eth0", &presult->ipaddr);
                  state = STATE_HAVE_OFFER;
                }
            }

          /* An error has occurred.  If this was a timeout error (meaning that
           * nothing was received on this socket for a long period of time).
           * Then loop and send the DISCOVER command again.
           */

          else if (*get_errno_ptr() != EAGAIN)
            {
              /* An error other than a timeout was received -- error out */

              return ERROR;
            }
        }
      while (state == STATE_INITIAL);


      /* Loop sending the REQUEST up to three times (if there is no response) */

      retries = 0;
      do
        {
          /* Send the REQUEST message to obtain the lease that was offered to us. */

          dbg("Send REQUEST\n");
          if (dhcpc_sendmsg(pdhcpc, presult, DHCPREQUEST) < 0)
            {
              return ERROR;
            }
          retries++;

          /* Get the ACK/NAK response to the REQUEST (or timeout) */

          result = recv(pdhcpc->sockfd, &pdhcpc->packet, sizeof(struct dhcp_msg), 0);
          if (result >= 0)
            {
              /* Parse the response */

              msgtype = dhcpc_parsemsg(pdhcpc, result, presult);

              /* The ACK response means that the server has accepted our request
               * and we have the lease.
               */

              if (msgtype == DHCPACK)
                {
                  dbg("Received ACK\n");
                  state = STATE_HAVE_LEASE;
                }

              /* NAK means the the server has refused our request.  Break out of
               * this loop with state == STATE_HAVE_OFFER and send DISCOVER again
               */

              else if (msgtype == DHCPNAK)
                {
                  dbg("Received NAK\n");
                  break;
                }

              /* If we get any OFFERs from other servers, then decline them now
               * and continue waiting for the ACK from the server that we
               * requested from.
               */

              else if (msgtype == DHCPOFFER)
                {
                  dbg("Received another OFFER, send DECLINE\n");
                  (void)dhcpc_sendmsg(pdhcpc, presult, DHCPDECLINE);
                }

              /* Otherwise, it is something that we do not recognize */

              else
                {
                  dbg("Ignoring msgtype=%d\n", msgtype);
                }
            }

          /* An error has occurred.  If this was a timeout error (meaning
           * that nothing was received on this socket for a long period of time).
           * Then break out and send the DISCOVER command again (at most
           * 3 times).
           */

          else if (*get_errno_ptr() != EAGAIN)
            {
              /* An error other than a timeout was received */

              (void)uip_sethostaddr("eth0", &oldaddr);
              return ERROR;
            }
        }
      while (state == STATE_HAVE_OFFER && retries < 3);
    }
  while (state != STATE_HAVE_LEASE);

  dbg("Got IP address %d.%d.%d.%d\n",
      (presult->ipaddr.s_addr >> 24 ) & 0xff,
      (presult->ipaddr.s_addr >> 16 ) & 0xff,
      (presult->ipaddr.s_addr >> 8  ) & 0xff,
      (presult->ipaddr.s_addr       ) & 0xff);
  dbg("Got netmask %d.%d.%d.%d\n",
      (presult->netmask.s_addr >> 24 ) & 0xff,
      (presult->netmask.s_addr >> 16 ) & 0xff,
      (presult->netmask.s_addr >> 8  ) & 0xff,
      (presult->netmask.s_addr       ) & 0xff);
  dbg("Got DNS server %d.%d.%d.%d\n",
      (presult->dnsaddr.s_addr >> 24 ) & 0xff,
      (presult->dnsaddr.s_addr >> 16 ) & 0xff,
      (presult->dnsaddr.s_addr >> 8  ) & 0xff,
      (presult->dnsaddr.s_addr       ) & 0xff);
  dbg("Got default router %d.%d.%d.%d\n",
      (presult->default_router.s_addr >> 24 ) & 0xff,
      (presult->default_router.s_addr >> 16 ) & 0xff,
      (presult->default_router.s_addr >> 8  ) & 0xff,
      (presult->default_router.s_addr       ) & 0xff);
  dbg("Lease expires in %ld seconds\n",
      ntohs(presult->lease_time[0])*65536ul + ntohs(presult->lease_time[1]));
  return OK;
}
