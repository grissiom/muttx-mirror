/****************************************************************************
 * net/uip/uip-icmpping.c
 *
 *   Copyright (C) 2008 Gregory Nutt. All rights reserved.
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

#include <nuttx/config.h>
#if defined(CONFIG_NET) && defined(CONFIG_NET_ICMP) && \
    defined(CONFIG_NET_ICMP_PING) && !defined(CONFIG_DISABLE_CLOCK)

#include <sys/types.h>
#include <semaphore.h>
#include <debug.h>

#include <net/if.h>
#include <nuttx/clock.h>
#include <net/uip/uipopt.h>
#include <net/uip/uip.h>
#include <net/uip/uip-arch.h>

#include "uip-internal.h"

/****************************************************************************
 * Definitions
 ****************************************************************************/

#define ICMPBUF ((struct uip_icmpip_hdr *)&dev->d_buf[UIP_LLH_LEN])

/* Allocate a new ICMP data callback */

#define uip_icmpcallbackalloc()   uip_callbackalloc(&g_echocallback)
#define uip_icmpcallbackfree(cb)  uip_callbackfree(cb, &g_echocallback)

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct icmp_ping_s
{
  FAR struct uip_callback_s *png_cb; /* Reference to callback instance */

  sem_t        png_sem;     /* Use to manage the wait for the response */
  uint32       png_time;    /* Start time for determining timeouts */
  uint32       png_ticks;   /* System clock ticks to wait */
  int          png_result;  /* 0: success; <0:negated errno on fail */
  uip_ipaddr_t png_addr;    /* The peer to be ping'ed */
  uint16       png_id;      /* Used to match requests with replies */
  uint16       png_seqno;   /* IN: seqno to send; OUT: seqno recieved */
  boolean      png_sent;    /* TRUE... the PING request has been sent */
};

/****************************************************************************
 * Public Variables
 ****************************************************************************/

/****************************************************************************
 * Private Variables
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Function: ping_timeout
 *
 * Description:
 *   Check for send timeout.
 *
 * Parameters:
 *   pstate - Ping state structure
 *
 * Returned Value:
 *   TRUE:timeout FALSE:no timeout
 *
 * Assumptions:
 *   Running at the interrupt level
 *
 ****************************************************************************/

static inline int ping_timeout(struct icmp_ping_s *pstate)
{
  uint32 elapsed =  g_system_timer - pstate->png_time;
  if (elapsed >= pstate->png_ticks)
    {
      return TRUE;
    }
  return FALSE;
}

/****************************************************************************
 * Function: ping_interrupt
 *
 * Description:
 *   This function is called from the interrupt level to perform the actual
 *   ECHO request and/or ECHO reply actions when polled by the uIP layer.
 *
 * Parameters:
 *   dev        The structure of the network driver that caused the interrupt
 *   conn       The received packet, cast to void *
 *   pvprivate  An instance of struct icmp_ping_s cast to void*
 *   flags      Set of events describing why the callback was invoked
 *
 * Returned Value:
 *   Modified value of the input flags
 *
 * Assumptions:
 *   Running at the interrupt level
 *
 ****************************************************************************/

static uint16 ping_interrupt(struct uip_driver_s *dev, void *conn,
                             void *pvprivate, uint16 flags)
{
  struct icmp_ping_s *pstate = (struct icmp_ping_s *)pvprivate;
  int failcode = -ETIMEDOUT;

  nvdbg("flags: %04x\n", flags);
  if (pstate)
    {
      /* Check if this device is on the same network as the destination device. */

      if (!uip_ipaddr_maskcmp(pstate->png_addr, dev->d_ipaddr, dev->d_netmask))
        {
          /* Destination address was not on the local network served by this
           * device.  If a timeout occurs, then the most likely reason is
           * that the destination address is not reachable.
           */

          failcode = -ENETUNREACH;
        }
      else
        {
          /* Check if this is a ICMP ECHO reply.  If so, return the sequence
           * number to the the caller.  NOTE: We may not even have sent the
           * requested ECHO request; this could have been the delayed ECHO
           * response from a previous ping.
           */

          if ((flags & UIP_ECHOREPLY) != 0 && conn != NULL)
            {
              struct uip_icmpip_hdr *icmp = (struct uip_icmpip_hdr *)conn;
              if (ntohs(icmp->id) == pstate->png_id)
                {
                  pstate->png_result = OK;
                  pstate->png_seqno  = ntohs(icmp->seqno);
                  goto end_wait;
                }
            }

          /* Check:
           *   If the outgoing packet is available (it may have been claimed
           *   by a sendto interrupt serving a different thread
           * -OR-
           *   If the output buffer currently contains unprocessed incoming
           *   data.
           * -OR-
           *   If we have alread sent the ECHO request
           *
           * In the first two cases, we will just have to wait for the next
           * polling cycle.
           */

          if (dev->d_sndlen <= 0 &&           /* Packet available */
              (flags & UIP_NEWDATA) == 0 &&   /* No incoming data */
              !pstate->png_sent)              /* Request not sent */
             {
              /* We can send the ECHO request now.
               *
               * Format the ICMP ECHO request packet
               */

              ICMPBUF->type  = ICMP_ECHO_REQUEST;
              ICMPBUF->icode = 0;
#ifndef CONFIG_NET_IPv6
              ICMPBUF->id     = htons(pstate->png_id);
              ICMPBUF->seqno  = htons(pstate->png_seqno);
#else
# error "IPv6 ECHO Request not implemented"
#endif

              /* Send the ICMP echo request.  Note that d_sndlen is set to
               * the size of the ICMP payload and does not include the size
               * of the ICMP header.
               */

              dev->d_sndlen= 4;
              uip_icmpsend(dev, &pstate->png_addr);
              pstate->png_sent = TRUE;
              return flags;
            }
        }

      /* Check if the selected timeout has elapsed */

      if (ping_timeout(pstate))
        {
          /* Yes.. report the timeout */

          nvdbg("Ping timeout\n");
          pstate->png_result = failcode;
          goto end_wait;
        }

      /* Continue waiting */
    }
  return flags;

end_wait:
  /* Do not allow any further callbacks */

  pstate->png_cb->flags   = 0;
  pstate->png_cb->private = NULL;
  pstate->png_cb->event   = NULL;

  /* Wake up the waiting thread */

  sem_post(&pstate->png_sem);
  return flags;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: uip_ping
 *
 * Description:
 *   Send a ECHO request and wait for the ECHO response
 *
 * Parameters:
 *   addr  - The IP address of the peer to send the ICMP ECHO request to
 *           in network order.
 *   id    - The ID to use in the ICMP ECHO request.  This number should be
 *           unique; only ECHO responses with this matching ID will be
 *           processed (host order)
 *   seqno - The sequence number used in the ICMP ECHO request.  NOT used
 *           to match responses (host order)
 *   dsecs - Wait up to this many deci-seconds for the ECHO response to be
 *           returned (host order).
 *
 * Return:
 *   seqno of received ICMP ECHO with matching ID (may be different
 *   from the seqno argument (may be a delayed response from an earlier
 *   ping with the same ID). Or a negated errno on any failure.
 *
 * Assumptions:
 *   Called from the user level with interrupts enabled.
 *
 ****************************************************************************/

int uip_ping(uip_ipaddr_t addr, uint16 id, uint16 seqno, int dsecs)
{
  struct icmp_ping_s state;
  irqstate_t save;

  /* Initialize the state structure */

  sem_init(&state.png_sem, 0, 0);
  state.png_ticks  = DSEC2TICK(dsecs); /* System ticks to wait */
  state.png_result = -ENOMEM;          /* Assume allocation failure */
  state.png_addr   = addr;             /* Address of the peer to be ping'ed */
  state.png_id     = id;               /* The ID to use in the ECHO request */
  state.png_seqno  = seqno;            /* The seqno to use int the ECHO request */
  state.png_sent   = FALSE;            /* ECHO request not yet sent */

  save             = irqsave();
  state.png_time   = g_system_timer;

  /* Set up the callback */

 state.png_cb = uip_icmpcallbackalloc();
  if (state.png_cb)
    {
      state.png_cb->flags   = UIP_POLL|UIP_ECHOREPLY;
      state.png_cb->private = (void*)&state;
      state.png_cb->event   = ping_interrupt;

      /* Wait for either the full round trip transfer to complete or
       * for timeout to occur. (1) sem_wait will also terminate if a
       * signal is received, (2) interrupts are disabled!  They will
       * be re-enabled while the task sleeps and automatically
       * re-enabled when the task restarts.
       */

      state.png_result = -EINTR; /* Assume sem-waited interrupt by signal */
      sem_wait(&state.png_sem);

      uip_icmpcallbackfree(state.png_cb);
    }
  irqrestore(save);

  /* Return the negated error number in the event of a failure, or the
   * sequence number of the ECHO reply on success.
   */

  if (!state.png_result)
    {
      return (int)state.png_seqno;
    }
  else
    {
      return state.png_result;
    }
}

#endif /* CONFIG_NET_ICMP && CONFIG_NET_ICMP_PING  ... */
