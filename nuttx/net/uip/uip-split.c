/* uip-split.c
 * Author: Adam Dunkels <adam@sics.se>
 *
 * Copyright (c) 2004, Swedish Institute of Computer Science.
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
 */

#include <nuttx/config.h>
#include <string.h>

#include <net/uip/uip.h>
#include <net/uip/uip-arch.h>

#include "uip-split.h"
#include "uip-fw.h"

#define BUF ((struct uip_tcpip_hdr *)&dev->d_buf[UIP_LLH_LEN])

void uip_split_output(struct uip_driver_s *dev)
{
  uint16 tcplen, len1, len2;

  /* We only try to split maximum sized TCP segments. */
  if(BUF->proto == UIP_PROTO_TCP &&
     dev->d_len == UIP_BUFSIZE - UIP_LLH_LEN) {

    tcplen = dev->d_len - UIP_TCPIP_HLEN;
    /* Split the segment in two. If the original packet length was
       odd, we make the second packet one byte larger. */
    len1 = len2 = tcplen / 2;
    if(len1 + len2 < tcplen) {
      ++len2;
    }

    /* Create the first packet. This is done by altering the length
       field of the IP header and updating the checksums. */
    dev->d_len = len1 + UIP_TCPIP_HLEN;
#ifdef CONFIG_NET_IPv6
    /* For IPv6, the IP length field does not include the IPv6 IP header
       length. */
    BUF->len[0] = ((dev->d_len - UIP_IPH_LEN) >> 8);
    BUF->len[1] = ((dev->d_len - UIP_IPH_LEN) & 0xff);
#else /* CONFIG_NET_IPv6 */
    BUF->len[0] = dev->d_len >> 8;
    BUF->len[1] = dev->d_len & 0xff;
#endif /* CONFIG_NET_IPv6 */

    /* Recalculate the TCP checksum. */
    BUF->tcpchksum = 0;
    BUF->tcpchksum = ~(uip_tcpchksum(dev));

#ifndef CONFIG_NET_IPv6
    /* Recalculate the IP checksum. */
    BUF->ipchksum = 0;
    BUF->ipchksum = ~(uip_ipchksum(dev));
#endif /* CONFIG_NET_IPv6 */

    /* Transmit the first packet. */
    /*    uip_fw_output();*/
    tcpip_output();

    /* Now, create the second packet. To do this, it is not enough to
       just alter the length field, but we must also update the TCP
       sequence number and point the d_appdata to a new place in
       memory. This place is detemined by the length of the first
       packet (len1). */
    dev->d_len = len2 + UIP_TCPIP_HLEN;
#ifdef CONFIG_NET_IPv6
    /* For IPv6, the IP length field does not include the IPv6 IP header
       length. */
    BUF->len[0] = ((dev->d_len - UIP_IPH_LEN) >> 8);
    BUF->len[1] = ((dev->d_len - UIP_IPH_LEN) & 0xff);
#else /* CONFIG_NET_IPv6 */
    BUF->len[0] = dev->d_len >> 8;
    BUF->len[1] = dev->d_len & 0xff;
#endif /* CONFIG_NET_IPv6 */

    /*    dev->d_appdata += len1;*/
    memcpy(dev->d_appdata, dev->d_appdata + len1, len2);

    uip_incr32(BUF->seqno, len1);

    /* Recalculate the TCP checksum. */

    BUF->tcpchksum = 0;
    BUF->tcpchksum = ~(uip_tcpchksum(dev));

#ifndef CONFIG_NET_IPv6
    /* Recalculate the IP checksum. */

    BUF->ipchksum = 0;
    BUF->ipchksum = ~(uip_ipchksum(dev));
#endif /* CONFIG_NET_IPv6 */

    /* Transmit the second packet. */
    /*    uip_fw_output();*/
    tcpip_output();
  } else {
    /*    uip_fw_output();*/
    tcpip_output();
  }
}
