/****************************************************************************
 * uip-resolv.c
 * DNS host name to IP address resolver.
 *
 * The uIP DNS resolver functions are used to lookup a hostname and
 * map it to a numerical IP address. It maintains a list of resolved
 * hostnames that can be queried with the resolv_lookup()
 * function. New hostnames can be resolved using the resolv_query()
 * function.
 *
 * When a hostname has been resolved (or found to be non-existant),
 * the resolver code calls a callback function called resolv_found()
 * that must be implemented by the module that uses the resolver.
 *
 *   Copyright (C) 2007 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <spudmonkey@racsa.co.cr>
 *
 * Based heavily on portions of uIP:
 *
 *   Author: Adam Dunkels <adam@dunkels.com>
 *   Copyright (c) 2002-2003, Adam Dunkels.
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <debug.h>

#include <sys/socket.h>
#include <arpa/inet.h>

#include <net/uip/resolv.h>

/****************************************************************************
 * Definitions
 ****************************************************************************/

#ifndef CONFIG_NET_RESOLV_ENTRIES
#define RESOLV_ENTRIES 4
#else /* CONFIG_NET_RESOLV_ENTRIES */
#define RESOLV_ENTRIES CONFIG_NET_RESOLV_ENTRIES
#endif /* CONFIG_NET_RESOLV_ENTRIES */

#ifndef NULL
#define NULL (void *)0
#endif /* NULL */

/* The maximum number of retries when asking for a name */

#define MAX_RETRIES 8

#define DNS_FLAG1_RESPONSE        0x80
#define DNS_FLAG1_OPCODE_STATUS   0x10
#define DNS_FLAG1_OPCODE_INVERSE  0x08
#define DNS_FLAG1_OPCODE_STANDARD 0x00
#define DNS_FLAG1_AUTHORATIVE     0x04
#define DNS_FLAG1_TRUNC           0x02
#define DNS_FLAG1_RD              0x01
#define DNS_FLAG2_RA              0x80
#define DNS_FLAG2_ERR_MASK        0x0f
#define DNS_FLAG2_ERR_NONE        0x00
#define DNS_FLAG2_ERR_NAME        0x03

#define SEND_BUFFER_SIZE 64
#define RECV_BUFFER_SIZE 64

#ifdef CONFIG_NET_IPv6
#define ADDRLEN sizeof(struct sockaddr_in6)
#else
#define ADDRLEN sizeof(struct sockaddr_in)
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* The DNS message header */

struct dns_hdr
{
  uint16 id;
  uint8 flags1, flags2;
  uint16 numquestions;
  uint16 numanswers;
  uint16 numauthrr;
  uint16 numextrarr;
};

/* The DNS answer message structure */

struct dns_answer
{
  /* DNS answer record starts with either a domain name or a pointer
   * to a name already present somewhere in the packet.
   */

  uint16 type;
  uint16 class;
  uint16 ttl[2];
  uint16 len;
#ifdef CONFIG_NET_IPv6
  struct in6_addr ipaddr;
#else
  struct in_addr ipaddr;
#endif
};

struct namemap
{
  uint8 state;
  uint8 tmr;
  uint8 retries;
  uint8 seqno;
  uint8 err;
  char name[32];
#ifdef CONFIG_NET_IPv6
  struct in6_addr ipaddr;
#else
  struct in_addr ipaddr;
#endif
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static uint8 g_seqno;
static int g_sockfd = -1;
#ifdef CONFIG_NET_IPv6
static struct sockaddr_in6 g_dnsserver;
#else
static struct sockaddr_in g_dnsserver;
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/* Walk through a compact encoded DNS name and return the end of it. */

static unsigned char *parse_name(unsigned char *query)
{
  unsigned char n;

  do
    {
      n = *query++;

      while(n > 0)
        {
          ++query;
          --n;
        }
    }
  while(*query != 0);
  return query + 1;
}

/* Runs through the list of names to see if there are any that have
 * not yet been queried and, if so, sends out a query.
 */

#ifdef CONFIG_NET_IPv6
static int send_query(const char *name, struct sockaddr_in6 *addr)
#else
static int send_query(const char *name, struct sockaddr_in *addr)
#endif
{
  register struct dns_hdr *hdr;
  char *query;
  char *nptr;
  const char *nameptr;
  uint8 seqno = g_seqno++;
  static unsigned char endquery[] = {0, 0, 1, 0, 1};
  char buffer[SEND_BUFFER_SIZE];
  int n;

  hdr               = (struct dns_hdr*)buffer;
  memset(hdr, 0, sizeof(struct dns_hdr));
  hdr->id           = htons(seqno);
  hdr->flags1       = DNS_FLAG1_RD;
  hdr->numquestions = HTONS(1);
  query             = buffer + 12;

  /* Convert hostname into suitable query format. */

  nameptr = name - 1;
  do
   {
     nameptr++;
     nptr = query++;
     for (n = 0; *nameptr != '.' && *nameptr != 0; nameptr++)
       {
         *query++ = *nameptr;
         n++;
       }
     *nptr = n;
   }
  while(*nameptr != 0);

  memcpy(query, endquery, 5);
  return sendto(g_sockfd, buffer, query + 5 - buffer, 0, (struct sockaddr*)addr, ADDRLEN);
}

/* Called when new UDP data arrives */

#ifdef CONFIG_NET_IPv6
#error "Not implemented"
#else
int recv_response(struct sockaddr_in *addr)
#endif
{
  unsigned char *nameptr;
  char buffer[RECV_BUFFER_SIZE];
  struct dns_answer *ans;
  struct dns_hdr *hdr;
  uint8 nquestions;
  uint8 nanswers;
  int ret;

  /* Receive the response */

  ret = recv(g_sockfd, buffer, RECV_BUFFER_SIZE, 0);
  if (ret < 0)
    {
      return ret;
    }

  hdr = (struct dns_hdr *)buffer;

  dbg( "ID %d\n", htons(hdr->id));
  dbg( "Query %d\n", hdr->flags1 & DNS_FLAG1_RESPONSE);
  dbg( "Error %d\n", hdr->flags2 & DNS_FLAG2_ERR_MASK);
  dbg( "Num questions %d, answers %d, authrr %d, extrarr %d\n",
       htons(hdr->numquestions), htons(hdr->numanswers),
       htons(hdr->numauthrr), htons(hdr->numextrarr));

  /* Check for error. If so, call callback to inform */

  if ((hdr->flags2 & DNS_FLAG2_ERR_MASK) != 0)
    {
      return ERROR;
    }

  /* We only care about the question(s) and the answers. The authrr
   * and the extrarr are simply discarded.
   */

  nquestions = htons(hdr->numquestions);
  nanswers   = htons(hdr->numanswers);

  /* Skip the name in the question. XXX: This should really be
   * checked agains the name in the question, to be sure that they
   * match.
    */

  nameptr = parse_name((unsigned char *)buffer + 12) + 4;

  for (; nanswers > 0; nanswers--)
    {
      /* The first byte in the answer resource record determines if it
       * is a compressed record or a normal one.
       */

      if (*nameptr & 0xc0)
        {
          /* Compressed name. */

          nameptr +=2;
          dbg("Compressed anwser\n");
        }
      else
        {
          /* Not compressed name. */
          nameptr = parse_name(nameptr);
        }

      ans = (struct dns_answer *)nameptr;
      dbg("Answer: type %x, class %x, ttl %x, length %x\n",
          htons(ans->type), htons(ans->class), (htons(ans->ttl[0]) << 16) | htons(ans->ttl[1]),
          htons(ans->len));

      /* Check for IP address type and Internet class. Others are discarded. */

      if (ans->type == HTONS(1) && ans->class == HTONS(1) && ans->len == HTONS(4))
        {
          dbg("IP address %d.%d.%d.%d\n",
              (ans->ipaddr.s_addr >> 24 ) & 0xff,
              (ans->ipaddr.s_addr >> 16 ) & 0xff,
              (ans->ipaddr.s_addr >> 8  ) & 0xff,
              (ans->ipaddr.s_addr       ) & 0xff);

           /* XXX: we should really check that this IP address is the one
           * we want.
           */

          addr->sin_addr.s_addr = ans->ipaddr.s_addr;
          return OK;
        }
      else
        {
          nameptr = nameptr + 10 + htons(ans->len);
        }
    }
  return ERROR;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/* Get the binding for name. */

#ifdef CONFIG_NET_IPv6
int resolv_query(FAR const char *name, FAR struct sockaddr_in6 *addr)
#else
int resolv_query(FAR const char *name, FAR struct sockaddr_in *addr)
#endif
{
  int retries;
  int ret;

  /* Loop while receive timeout errors occur and there are remaining retries */

  for (retries = 0; retries < 3; retries++)
    {
      if (send_query(name, addr) < 0)
        {
          return ERROR;
        }

      ret = recv_response(addr);
      if (ret >= 0)
        {
          /* Response received successfully */

          return OK;
        }

      else if (errno != EAGAIN)
        {
          /* Some failure other than receive timeout occurred */

          return ERROR;
        }
    }

  return ERROR;
}

/* Obtain the currently configured DNS server. */

#ifdef CONFIG_NET_IPv6
void resolv_getserver(struct in6_addr *dnsserver)
#else
void resolv_getserver(struct in_addr *dnsserver)
#endif
{
#ifdef CONFIG_NET_IPv6
  memcpy(dnsserver, &g_dnsserver.sin6_addr, ADDRLEN);
#else
  dnsserver->s_addr = g_dnsserver.sin_addr.s_addr;
#endif
}

/* Configure which DNS server to use for queries */

#ifdef CONFIG_NET_IPv6
void resolv_conf(const struct in6_addr *dnsserver)
#else
void resolv_conf(const struct in_addr *dnsserver)
#endif
{
  g_dnsserver.sin_family = AF_INET;
  g_dnsserver.sin_port   = HTONS(53);
#ifdef CONFIG_NET_IPv6
  memcpy(&g_dnsserver.sin6_addr, dnsserver, ADDRLEN);
#else
  g_dnsserver.sin_addr.s_addr = dnsserver->s_addr;
#endif
}

/* Initalize the resolver. */

int resolv_init(void)
{
  struct timeval tv;
  g_sockfd = socket(PF_INET, SOCK_DGRAM, 0);
  if (g_sockfd < 0)
    {
      return ERROR;
    }

  /* Set up a receive timeout */

  tv.tv_sec  = 30;
  tv.tv_usec = 0;
  if (setsockopt(g_sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(struct timeval)) < 0)
    {
      close(g_sockfd);
      g_sockfd = -1;
      return ERROR;
    }

  return OK;
}
