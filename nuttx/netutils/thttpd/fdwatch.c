/****************************************************************************
 * netutils/thttpd/timers.c
 * FD watcher routines for poll()
 *
 *   Copyright (C) 2009 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <spudmonkey@racsa.co.cr>
 *
 * Derived from the file of the same name in the original THTTPD package:
 *
 *   Copyright � 1999,2000 by Jef Poskanzer <jef@mail.acme.com>.
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
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

#include <nuttx/config.h>
#include <sys/types.h>

#include <stdlib.h>
#include <debug.h>

#if 0
#include <sys/time.h>

#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#endif
#include <poll.h>

#include "config.h"
#include "fdwatch.h"

#ifdef CONFIG_THTTPD

/****************************************************************************
 * Pre-Processor Definitions
 ****************************************************************************/

#ifndef MIN
#  define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

#if defined(CONFIG_DEBUG) && defined(CONFIG_DEBUG_NET)
static long nwatches = 0;
#endif

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int fdwatch_pollndx(FAR struct fdwatch_s *fw, int fd)
{
  int pollndx;

  /* Get the index associated with the fd */

  for (pollndx = 0; pollndx < fw->nwatched; pollndx++)
    {
      if (fw->pollfds[pollndx].fd == fd)
        {
          nvdbg("pollndx: %d\n", pollndx);
          return pollndx;
        }
    }

  ndbg("No poll index for fd %d: %d\n", fd);
  return -1;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/* Initialize the fdwatch data structures.  Returns -1 on failure. */

struct fdwatch_s *fdwatch_initialize(int nfds)
{
  FAR struct fdwatch_s *fw;

  /* Allocate the fdwatch data structure */

  fw = (struct fdwatch_s*)zalloc(sizeof(struct fdwatch_s));
  if (!fw)
    {
      ndbg("Failed to allocate fdwatch\n");
      return NULL;
    }

  /* Initialize the fdwatch data structures. */

  fw->nfds = nfds;

  fw->client = (struct fw_fd_s*)malloc(sizeof(struct fw_fd_s) * nfds);
  if (!fw->client)
    {
      goto errout_with_allocations;
    }

  fw->pollfds = (struct pollfd*)malloc(sizeof(struct pollfd) * nfds);
  if (!fw->pollfds)
    {
      goto errout_with_allocations;
    }

  fw->ready = (uint8*)malloc(sizeof(uint8) * nfds);
  if (!fw->ready)
    {
      goto errout_with_allocations;
    }
  return fw;

errout_with_allocations:
  fdwatch_uninitialize(fw);
  return NULL;
}

/* Uninitialize the fwdatch data structure */

void fdwatch_uninitialize(struct fdwatch_s *fw)
{
  if (fw)
    {
      if (fw->client)
        {
          free(fw->client);
        }

      if (fw->pollfds)
        {
          free(fw->pollfds);
        }

      if (fw->ready)
        {
          free(fw->ready);
        }

      free(fw);
    }
}

/* Add a descriptor to the watch list.  rw is either FDW_READ or FDW_WRITE.  */

void fdwatch_add_fd(struct fdwatch_s *fw, int fd, void *client_data, int rw)
{
  nvdbg("fd: %d\n", fd);

#ifdef CONFIG_DEBUG
  if (fd < CONFIG_NFILE_DESCRIPTORS ||
      fd >= CONFIG_NFILE_DESCRIPTORS+fw->nfds)
    {
      ndbg("Received bad fd (%d)\n", fd);
      return;
    }
#endif

  if (fw->nwatched >= fw->nfds)
    {
      ndbg("too many fds\n");
      return;
    }

  /* Save the new fd at the end of the list */

  fw->pollfds[fw->nwatched].fd  = fd;
  fw->client[fw->nwatched].rw   = rw;
  fw->client[fw->nwatched].data = client_data;

  if (rw == FDW_READ)
    {
      fw->pollfds[fw->nwatched].events = POLLIN;
    }
  else
    {
      fw->pollfds[fw->nwatched].events = POLLOUT;
    }

  /* Increment the count of watched descriptors */

  fw->nwatched++;
}

/* Remove a descriptor from the watch list. */

void fdwatch_del_fd(struct fdwatch_s *fw, int fd)
{
  int pollndx;

  nvdbg("fd: %d\n", fd);

#ifdef CONFIG_DEBUG
  if (fd < CONFIG_NFILE_DESCRIPTORS ||
      fd >= CONFIG_NFILE_DESCRIPTORS+fw->nfds)
    {
      ndbg("Received bad fd: %d\n", fd);
      return;
    }
#endif

  /* Get the index associated with the fd */

  pollndx = fdwatch_pollndx(fw, fd);
  if (pollndx >= 0)
    {
      /* Decrement the number of fds in the poll table */

      fw->nwatched--;

      /* Replace the deleted one with the one at the the end
       * of the list.
       */

      if (pollndx != fw->nwatched)
        {
          fw->pollfds[pollndx]      = fw->pollfds[fw->nwatched];
          fw->client[pollndx].rw    = fw->client[fw->nwatched].rw;
          fw->client[pollndx].data  = fw->client[fw->nwatched].data;
        }
    }
}

/* Do the watch.  Return value is the number of descriptors that are ready,
 * or 0 if the timeout expired, or -1 on errors.  A timeout of INFTIM means
 * wait indefinitely.
 */

int fdwatch(struct fdwatch_s *fw, long timeout_msecs)
{
  int ret;
  int i;

#if defined(CONFIG_DEBUG) && defined(CONFIG_DEBUG_NET)
  nwatches++;
#endif

  /* Wait for activity on any of the desciptors.  When poll() returns, ret
   * will hold the number of descriptors with activity (or zero on a timeout
   * or <0 on an error.
   */

  nvdbg("Waiting...\n");
  fw->nactive = 0;
  fw->next    = 0;
  ret = poll(fw->pollfds, fw->nwatched, (int)timeout_msecs);
  nvdbg("Awakened: %d\n", ret);

  /* Look through all of the descriptors and make a list of all of them than
   * have activity.
   */

  if (ret > 0)
    {
      for (i = 0; i < fw->nwatched; i++)
        {
          /* Is there activity on this descriptor? */

          if (fw->pollfds[i].revents & (POLLIN | POLLOUT | POLLERR | POLLHUP | POLLNVAL))
            {
              /* Yes... save it in a shorter list */

              fw->ready[fw->nactive++] = fw->pollfds[i].fd;
              if (fw->nactive == ret)
                {
                  /* We have all of them, break out early */

                  break;
                }
            }
        }
    }

  /* Return the number of descriptors with activity */

  nvdbg("nactive: %d\n", fw->nactive);
  return ret;
}

/* Check if a descriptor was ready. */

int fdwatch_check_fd(struct fdwatch_s *fw, int fd)
{
  int pollndx;

  nvdbg("fd: %d\n", fd);

#ifdef CONFIG_DEBUG
  if (fd < CONFIG_NFILE_DESCRIPTORS ||
      fd >= CONFIG_NFILE_DESCRIPTORS + fw->nfds)
    {
      ndbg("Bad fd: %d\n", fd);
      return 0;
    }
#endif

  /* Get the index associated with the fd */

   pollndx = fdwatch_pollndx(fw, fd);
   if (pollndx >= 0 && (fw->pollfds[pollndx].revents & POLLERR) == 0)
    {
      if (fw->client[pollndx].rw == FDW_READ)
        {
          return fw->pollfds[pollndx].revents & (POLLIN | POLLHUP | POLLNVAL);
        }
      else
        {
          return fw->pollfds[pollndx].revents & (POLLOUT | POLLHUP | POLLNVAL);
        }
    }

  nvdbg("POLLERR fd: %d\n", fd);
  return 0;
}

void *fdwatch_get_next_client_data(struct fdwatch_s *fw)
{
  if (fw->next >= fw->nfds)
    {
      ndbg("All client data returned: %d\n", fw->next);
      return NULL;
    }
  return fw->client[fw->next++].data;
}

/* Generate debugging statistics ndbg message. */

#if defined(CONFIG_DEBUG) && defined(CONFIG_DEBUG_NET)
void fdwatch_logstats(struct fdwatch_s *fw, long secs)
{
  if (secs > 0)
    {
      ndbg("fdwatch - %ld polls (%g/sec)\n", nwatches, (float)nwatches / secs);
    }
  nwatches = 0;
}
#endif

#endif /* CONFIG_THTTPD */

