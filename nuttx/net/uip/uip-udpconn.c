/****************************************************************************
 * uip-udpconn.c
 *
 *   Copyright (C) 2007 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <spudmonkey@racsa.co.cr>
 *
 * Large parts of this file were leveraged from uIP logic:
 *
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

/****************************************************************************
 * Compilation Switches
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#if defined(CONFIG_NET) && defined(CONFIG_NET_UDP)

#include <sys/types.h>
#include <string.h>
#include <semaphore.h>
#include <assert.h>
#include <errno.h>
#include <arch/irq.h>

#include <net/uip/uipopt.h>
#include <net/uip/uip.h>
#include <net/uip/uip-arch.h>

#include "uip-internal.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* The array containing all uIP UDP connections. */

struct uip_udp_conn g_udp_connections[UIP_UDP_CONNS];

/* A list of all free UDP connections */

static dq_queue_t g_free_udp_connections;
static sem_t g_free_sem;

/* A list of all allocated UDP connections */

static dq_queue_t g_active_udp_connections;

/* Last port used by a UDP connection connection. */

static uint16 g_last_udp_port;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: _uip_semtake() and _uip_semgive()
 *
 * Description:
 *   Take/give semaphore
 *
 ****************************************************************************/

static inline void _uip_semtake(sem_t *sem)
{
  /* Take the semaphore (perhaps waiting) */

  while (sem_wait(sem) != 0)
    {
      /* The only case that an error should occr here is if
       * the wait was awakened by a signal.
       */

      ASSERT(*get_errno_ptr() == EINTR);
    }
}

#define _uip_semgive(sem) sem_post(sem)

/****************************************************************************
 * Name: uip_find_conn()
 *
 * Description:
 *   Find the UDP connection that uses this local port number.  Called only
 *   from user user level code, but with interrupts disabled.
 *
 ****************************************************************************/

static inline struct uip_udp_conn *uip_find_conn( uint16 portno )
{
  uint16 nlastport = htons(g_last_udp_port);
  int i;

  /* Now search each connection structure.*/

  for (i = 0; i < UIP_UDP_CONNS; i++)
    {
      if (g_udp_connections[ i ].lport == nlastport)
        {
          return &g_udp_connections[ i ];
        }
    }

  return NULL;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: uip_udpinit()
 *
 * Description:
 *   Initialize the UDP connection structures.  Called once and only from
 *   the UIP layer.
 *
 ****************************************************************************/

void uip_udpinit(void)
{
  int i;

  /* Initialize the queues */

  dq_init(&g_free_udp_connections);
  dq_init(&g_active_udp_connections);
  sem_init(&g_free_sem, 0, 1);

  for (i = 0; i < UIP_UDP_CONNS; i++)
    {
      /* Mark the connection closed and move it to the free list */

      g_udp_connections[i].lport = 0;
      dq_addlast(&g_udp_connections[i].node, &g_free_udp_connections);
    }

  g_last_udp_port = 1024;
}

/****************************************************************************
 * Name: uip_udpalloc()
 *
 * Description:
 *   Alloc a new, uninitialized UDP connection structure.
 *
 ****************************************************************************/

struct uip_udp_conn *uip_udpalloc(void)
{
  struct uip_udp_conn *conn;

  /* The free list is only accessed from user, non-interrupt level and
   * is protected by a semaphore (that behaves like a mutex).
   */

  _uip_semtake(&g_free_sem);
  conn = (struct uip_udp_conn *)dq_remfirst(&g_free_udp_connections);
  if (conn)
    {
      /* Make sure that the connection is marked as uninitialized */

      conn->lport = 0;
    }
  _uip_semgive(&g_free_sem);
  return conn;
}

/****************************************************************************
 * Name: uip_udpfree()
 *
 * Description:
 *   Free a UDP connection structure that is no longer in use. This should be
 *   done by the implementation of close().  uip_udpdisable must have been
 *   previously called.
 *
 ****************************************************************************/

void uip_udpfree(struct uip_udp_conn *conn)
{
  /* The free list is only accessed from user, non-interrupt level and
   * is protected by a semaphore (that behaves like a mutex).
   */

  _uip_semtake(&g_free_sem);
  conn->lport = 0;
  dq_addlast(&conn->node, &g_free_udp_connections);
  _uip_semgive(&g_free_sem);
}

/****************************************************************************
 * Name: uip_udpactive()
 *
 * Description:
 *   Find a connection structure that is the appropriate
 *   connection to be used withi the provided TCP/IP header
 *
 * Assumptions:
 *   This function is called from UIP logic at interrupt level
 *
 ****************************************************************************/

struct uip_udp_conn *uip_udpactive(struct uip_udpip_hdr *buf)
{
  struct uip_udp_conn *conn = (struct uip_udp_conn *)g_active_udp_connections.head;
  while (conn)
    {
      /* If the local UDP port is non-zero, the connection is considered
       * to be used. If so, the local port number is checked against the
       * destination port number in the received packet. If the two port
       * numbers match, the remote port number is checked if the
       * connection is bound to a remote port. Finally, if the
       * connection is bound to a remote IP address, the source IP
       * address of the packet is checked.
       */

      if (conn->lport != 0 && buf->destport == conn->lport &&
          (conn->rport == 0 || buf->srcport == conn->rport) &&
            (uip_ipaddr_cmp(conn->ripaddr, all_zeroes_addr) ||
             uip_ipaddr_cmp(conn->ripaddr, all_ones_addr) ||
             uip_ipaddr_cmp(buf->srcipaddr, conn->ripaddr)))
        {
          /* Matching connection found.. return a reference to it */

          break;
        }

      /* Look at the next active connection */

      conn = (struct uip_udp_conn *)conn->node.flink;
    }

  return NULL;
}

/****************************************************************************
 * Name: uip_udppoll()
 *
 * Description:
 *   Periodic processing for a UDP connection identified by its number.
 *   This function does the necessary periodic processing (timers,
 *   polling) for a uIP TCP conneciton, and should be called by the UIP
 *   device driver when the periodic uIP timer goes off. It should be
 *   called for every connection, regardless of whether they are open of
 *   closed.
 *
 * Assumptions:
 *   This function is called from the CAN device driver may be called from
 *   the timer interrupt/watchdog handle level.
 *
 ****************************************************************************/

void uip_udppoll(struct uip_driver_s *dev, unsigned int conn)
{
  uip_udp_conn = &g_udp_connections[conn];
  uip_interrupt(dev, UIP_UDP_TIMER);
}

/****************************************************************************
 * Name: uip_udpconnect()
 *
 * Description:
 * This function sets up a new UDP connection. The function will
 * automatically allocate an unused local port for the new
 * connection. However, another port can be chosen by using the
 * uip_udpbind() call, after the uip_udpconnect() function has been
 * called.
 *
 * uip_udpenable() must be called before the connection is made active (i.e.,
 * is eligible for callbacks.
 *
 * addr The address of the remote host.
 *
 * Assumptions:
 *   This function is called user code.  Interrupts may be enabled.
 *
 ****************************************************************************/

#ifdef CONFIG_NET_IPv6
int uip_udpconnect(struct uip_udp_conn *conn, const struct sockaddr_in6 *addr)
#else
int uip_udpconnect(struct uip_udp_conn *conn, const struct sockaddr_in *addr)
#endif
{
  irqstate_t flags;

  /* Find an unused local port number.  Loop until we find a valid listen port
   * number that is not being used by any other connection.
   */

  flags = irqsave();
  do
    {
      /* Guess that the next available port number will be the one after
       * the last port number assigned.
       */

      ++g_last_udp_port;

      /* Make sure that the port number is within range */
      if (g_last_udp_port >= 32000)
        {
          g_last_udp_port = 4096;
        }
    }
  while (uip_find_conn(g_last_udp_port));

  /* Initialize and return the connection structure, bind it to the port number */

  conn->lport = HTONS(g_last_udp_port);
  irqrestore(flags);

  if (addr)
    {
      conn->rport = addr->sin_port;
      uip_ipaddr_copy(&conn->ripaddr, &addr->sin_addr.s_addr);
    }
  else
    {
      conn->rport = 0;
      uip_ipaddr_copy(&conn->ripaddr, &all_zeroes_addr);
    }
  conn->ttl   = UIP_TTL;
  return OK;
}

/****************************************************************************
 * Name: uip_udpenable() uip_udpdisable.
 *
 * Description:
 *   Enable/disable callbacks for the specified connection
 *
 * Assumptions:
 *   This function is called user code.  Interrupts may be enabled.
 *
 ****************************************************************************/

void uip_udpenable(struct uip_udp_conn *conn)
{
  /* Add the connection structure to the active connectionlist. This list
   * is modifiable from interrupt level, we we must diable interrupts to
   * access it safely.
   */

  irqstate_t flags = irqsave();
  dq_addlast(&conn->node, &g_active_udp_connections);
  irqrestore(flags);
}

void uip_udpdisable(struct uip_udp_conn *conn)
{
  /* Remove the connection structure to the active connectionlist. This list
   * is modifiable from interrupt level, we we must diable interrupts to
   * access it safely.
   */

  irqstate_t flags = irqsave();
  dq_rem(&conn->node, &g_active_udp_connections);
  irqrestore(flags);
}

#endif /* CONFIG_NET && CONFIG_NET_UDP */
