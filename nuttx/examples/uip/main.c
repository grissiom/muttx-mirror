/****************************************************************************
 * examples/uip/main.c
 *
 *   Copyright (C) 2007 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <spudmonkey@racsa.co.cr>
 *
 * Based on uIP which also has a BSD style license:
 *
 *   Copyright (c) 2001, Adam Dunkels.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Adam Dunkels.
 * 4. The name of the author may not be used to endorse or promote
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

#include <stdio.h>
#include <unistd.h>
#include <time.h>

#include <net/uip/uip.h>
#include <net/uip/uip-arp.h>

/* Here we include the header file for the application(s) we use in
 * our project as defined in the config/<board-name>/defconfig file
 */

#define CONFIG_EXAMPLE_UIP_WEBSERVER 1 /* For now */

#if defined(CONFIG_EXAMPLE_UIP_SMTP)
# include <net/uip/smtp.h>
#elif defined(CONFIG_EXAMPLE_UIP_TELNETD)
# include <net/uip/telnetd.h>
#elif defined(CONFIG_EXAMPLE_UIP_WEBSERVER)
# include <net/uip/httpd.h>
#elif defined(CONFIG_EXAMPLE_UIP_DHCPC)
# include <net/uip/dhcpc.h>
#elif defined(CONFIG_EXAMPLE_UIP_RESOLV)
# include <net/uip/resolv.h>
#elif defined(CONFIG_EXAMPLE_UIP_WEBCLIENT)
# include <net/uip/webclient.h>
#endif

int user_start(int argc, char *argv[])
{
  uip_ipaddr_t ipaddr;
#if defined(CONFIG_EXAMPLE_UIP_DHCPC)
  uint16 mac[6] = {1, 2, 3, 4, 5, 6};
#endif
#if defined(CONFIG_EXAMPLE_UIP_DHCPC) || defined(CONFIG_EXAMPLE_UIP_SMTP)
  void *handle;
#endif

  uip_ipaddr(ipaddr, 192, 168, 0, 2);
  uip_sethostaddr(ipaddr);
  uip_ipaddr(ipaddr, 192, 168, 0, 1);
  uip_setdraddr(ipaddr);
  uip_ipaddr(ipaddr, 255, 255, 255, 0);
  uip_setnetmask(ipaddr);

#if defined(CONFIG_EXAMPLE_UIP_WEBSERVER)
  httpd_init();
#elif defined(CONFIG_EXAMPLE_UIP_TELNETD)
  telnetd_init();
#elif defined(CONFIG_EXAMPLE_UIP_DHCPC)
  resolv_init();
  handle = dhcpc_open(&mac, 6);
  if (handle)
    {
        struct dhcpc_state ds;
        (void)dhcpc_request(handle, &ds);
        uip_sethostaddr(ds.ipaddr);
        uip_setnetmask(ds.netmask);
        uip_setdraddr(ds.default_router);
        resolv_conf(ds.dnsaddr);
        dhcpc_close(handle);
    }
#elif defined(CONFIG_EXAMPLE_UIP_SMTP)
  uip_ipaddr(ipaddr, 127, 0, 0, 1);
  handle = smtp_open();
  if (handle)
    {
      smtp_configure("localhost", ipaddr);
      smtp_send("adam@sics.se", NULL, "uip-testing@example.com",
                "Testing SMTP from uIP", "Test message sent by uIP\r\n");
      smtp_close(handle);
    }
#elif defined(CONFIG_EXAMPLE_UIP_WEBCLIENT)
  webclient_init();
  resolv_init();
  uip_ipaddr(ipaddr, 195, 54, 122, 204);
  resolv_conf(ipaddr);
  resolv_query("www.sics.se");
#endif

  while(1)
    {
      sleep(3);
      printf("main: Still running\n");
    }
  return 0;
}

void uip_log(char *m)
{
  printf("uIP log message: %s\n", m);
}

void webclient_closed(void)
{
  printf("Webclient: connection closed\n");
}

void webclient_aborted(void)
{
  printf("Webclient: connection aborted\n");
}

void webclient_timedout(void)
{
  printf("Webclient: connection timed out\n");
}

void webclient_connected(void)
{
  printf("Webclient: connected, waiting for data...\n");
}

void webclient_datahandler(char *data, uint16 len)
{
  printf("Webclient: got %d bytes of data.\n", len);
}
