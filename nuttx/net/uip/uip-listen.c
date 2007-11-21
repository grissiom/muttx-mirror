/****************************************************************************
 * net/uip/uip-listen.c
 *
 *   Copyright (C) 2007 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <spudmonkey@racsa.co.cr>
 *
 * A direct leverage of logic from uIP which also has b BSD style license
 *
 *   Author: Adam Dunkels <adam@dunkels.com>
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
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#ifdef CONFIG_NET

#include <sys/types.h>
#include <debug.h>

#include <net/uip/uipopt.h>

#include "uip-internal.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* The uip_listenports list all currently listening ports. */

static struct uip_conn *uip_listenports[CONFIG_NET_MAX_LISTENPORTS];

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Function: uip_listeninit
 *
 * Description:
 *   Setup the listening data structures
 *
 * Assumptions:
 *   Called early in the inialization phase while the system is still
 *   single-threaded.
 *
 ****************************************************************************/

void uip_listeninit(void)
{
  int ndx;
  for (ndx = 0; ndx < CONFIG_NET_MAX_LISTENPORTS; ndx++)
    {
      uip_listenports[ndx] = NULL;
    }
}

/****************************************************************************
 * Function: uip_unlisten
 *
 * Description:
 *   Stop listening to the port bound to the specified TCP connection
 *
 * Assumptions:
 *   Called from normal user code.
 *
 ****************************************************************************/

int uip_unlisten(struct uip_conn *conn)
{
  irqstate_t flags;
  int ndx;
  int ret = -EINVAL;

  flags = irqsave();
  for (ndx = 0; ndx < CONFIG_NET_MAX_LISTENPORTS; ndx++)
    {
      if (uip_listenports[ndx] == conn)
        {
          uip_listenports[ndx] = NULL;
          ret = OK;
          break;
        }
    }

  irqrestore(flags);
  return ret;
}

/****************************************************************************
 * Function: uip_listen
 *
 * Description:
 *   Start listening to the port bound to the specified TCP connection
 *
 * Assumptions:
 *   Called from normal user code.
 *
 ****************************************************************************/

int uip_listen(struct uip_conn *conn)
{
  irqstate_t flags;
  int ndx;
  int ret;

  /* This must be done with interrupts disabled because the listener table
   * is accessed from interrupt level as well.
   */

  flags = irqsave();

  /* First, check if there is already a socket listening on this port */

  if (uip_islistener(conn->lport))
    {
      /* Yes, then we must refuse this request */

      ret = -EADDRINUSE;
    }
  else
    {
      /* Otherwise, save a reference to the connection structure in the
       * "listener" list.
       */

      ret = -ENOBUFS; /* Assume failure */

      /* Search all slots until an available slot is found */

      for (ndx = 0; ndx < CONFIG_NET_MAX_LISTENPORTS; ndx++)
        {
          /* Is the next slot available? */

          if (!uip_listenports[ndx])
            {
              /* Yes.. we found it */

              uip_listenports[ndx] = conn;
              ret = OK;
              break;
            }
        }
    }

  irqrestore(flags);
  return ret;
}

/****************************************************************************
 * Function: uip_islistener
 *
 * Description:
 *   Return TRUE is there is a listener for the specified port
 *
 * Assumptions:
 *   Called at interrupt level
 *
 ****************************************************************************/

boolean uip_islistener(uint16 portno)
{
  int ndx;

  /* Examine each connection structure in each slot of the listener list */

  for (ndx = 0; ndx < CONFIG_NET_MAX_LISTENPORTS; ndx++)
    {
      /* Is this slot assigned?  If so, does the connection have the same
       * local port number?
       */

      struct uip_conn *conn = uip_listenports[ndx];
      if (conn && conn->lport == portno)
        {
          /* Yes.. we found a listener on this port */

          return TRUE;
        }
    }

  /* No listener for this port */

  return FALSE;
}

/****************************************************************************
 * Function: uip_accept
 *
 * Description:
 *   Accept the new connection for the specified listening port.
 *
 * Assumptions:
 *   Called at interrupt level
 *
 ****************************************************************************/

int uip_accept(struct uip_conn *conn, uint16 portno)
{
  struct uip_conn *listener;
  int ret = ERROR;

  /* The interrupt logic has already allocated and initialized a TCP
   * connection -- now check if is an application in place to accept the
   * connection.
   */

  listener = uip_tcplistener(portno);
  if (listener && listener->accept)
    {
      /* Yes.. accept the connection */

      ret = listener->accept(listener, conn);
    }
   return ret;
}

#endif /* CONFIG_NET */
