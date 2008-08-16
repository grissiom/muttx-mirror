/****************************************************************************
 * examples/nsh/nsh_serial.h
 *
 *   Copyright (C) 2007 Gregory Nutt. All rights reserved.
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
 * 3. Neither the name Gregory Nutt nor the names of its contributors may be
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
#include <sys/types.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>

#include "nsh.h"

/****************************************************************************
 * Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct serial_s
{
  struct nsh_vtbl_s vtbl;
  int    ss_refs;    /* Reference counts on the instance */
  int    ss_fd;      /* Re-direct file descriptor */
  FILE  *ss_stream;  /* Redirect file descriptor */
  char   ss_line[CONFIG_EXAMPLES_NSH_LINELEN];
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static FAR struct nsh_vtbl_s *nsh_consoleclone(FAR struct nsh_vtbl_s *vtbl);
static void nsh_consoleaddref(FAR struct nsh_vtbl_s *vtbl);
static void nsh_consolerelease(FAR struct nsh_vtbl_s *vtbl);
static int nsh_consoleoutput(FAR struct nsh_vtbl_s *vtbl, const char *fmt, ...);
static FAR char *nsh_consolelinebuffer(FAR struct nsh_vtbl_s *vtbl);
static FAR void *nsh_consoleredirect(FAR struct nsh_vtbl_s *vtbl, int fd);
static void nsh_consoleundirect(FAR struct nsh_vtbl_s *vtbl, FAR void *direct);

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nsh_allocstruct
 ****************************************************************************/

static inline FAR struct serial_s *nsh_allocstruct(void)
{
  struct serial_s *pstate = (struct serial_s *)malloc(sizeof(struct serial_s));
  if (pstate)
    {
      pstate->vtbl.clone      = nsh_consoleclone;
      pstate->vtbl.addref     = nsh_consoleaddref;
      pstate->vtbl.release    = nsh_consolerelease;
      pstate->vtbl.output     = nsh_consoleoutput;
      pstate->vtbl.linebuffer = nsh_consolelinebuffer;
      pstate->vtbl.redirect   = nsh_consoleredirect;
      pstate->vtbl.undirect   = nsh_consoleundirect;

      pstate->ss_refs         = 1;
      pstate->ss_fd           = 1;
      pstate->ss_stream       = stdout;
    }
  return pstate;
}

/****************************************************************************
 * Name: nsh_openifnotopen
 ****************************************************************************/

static int nsh_openifnotopen(struct serial_s *pstate)
{
  /* The stream is open in a lazy fashion.  This is done because the file
   * descriptor may be opened on a different task than the stream.
   */

  if (!pstate->ss_stream)
    {
      pstate->ss_stream = fdopen(pstate->ss_fd, "w");
      if (!pstate->ss_stream)
        {
          return ERROR;
        }
    }
  return 0;
}

/****************************************************************************
 * Name: nsh_closeifnotclosed
 ****************************************************************************/

static void nsh_closeifnotclosed(struct serial_s *pstate)
{
  if (pstate->ss_stream == stdout)
    {
      fflush(stdout);
      pstate->ss_fd = 1;
    }
  else
    {
      if (pstate->ss_stream)
        {
          fflush(pstate->ss_stream);
          fclose(pstate->ss_stream);
        }
      else if (pstate->ss_fd >= 0 && pstate->ss_fd != 1)
        {
          close(pstate->ss_fd);
        }

      pstate->ss_fd     = -1;
      pstate->ss_stream = NULL;
    }
}

/****************************************************************************
 * Name: nsh_consoleoutput
 *
 * Description:
 *   Print a string to the currently selected stream.
 *
 ****************************************************************************/

static int nsh_consoleoutput(FAR struct nsh_vtbl_s *vtbl, const char *fmt, ...)
{
  FAR struct serial_s *pstate = (FAR struct serial_s *)vtbl;
  va_list ap;
  int     ret;

  /* The stream is open in a lazy fashion.  This is done because the file
   * descriptor may be opened on a different task than the stream.  The
   * actual open will then occur with the first output from the new task.
   */

  if (nsh_openifnotopen(pstate) != 0)
   {
     return ERROR;
   }
 
  va_start(ap, fmt);
  ret = vfprintf(pstate->ss_stream, fmt, ap);
  va_end(ap);
 
  return ret;
}

/****************************************************************************
 * Name: nsh_consolelinebuffer
 *
 * Description:
 *   Return a reference to the current line buffer
 *
 ****************************************************************************/

static FAR char *nsh_consolelinebuffer(FAR struct nsh_vtbl_s *vtbl)
{
  FAR struct serial_s *pstate = (FAR struct serial_s *)vtbl;
  return pstate->ss_line;
}

/****************************************************************************
 * Name: nsh_consoleclone
 *
 * Description:
 *   Make an independent copy of the vtbl
 *
 ****************************************************************************/

static FAR struct nsh_vtbl_s *nsh_consoleclone(FAR struct nsh_vtbl_s *vtbl)
{
  FAR struct serial_s *pstate = (FAR struct serial_s *)vtbl;
  FAR struct serial_s *pclone = nsh_allocstruct();
  pclone->ss_fd     = pstate->ss_fd;
  pclone->ss_stream = NULL;
  return &pclone->vtbl;
}

/****************************************************************************
 * Name: nsh_consoleaddref
 *
 * Description:
 *   Increment the reference count on the vtbl.
 *
 ****************************************************************************/

static void nsh_consoleaddref(FAR struct nsh_vtbl_s *vtbl)
{
  FAR struct serial_s *pstate = (FAR struct serial_s *)vtbl;
  pstate->ss_refs++;
}

/****************************************************************************
 * Name: nsh_consolerelease
 *
 * Description:
 *   Decrement the reference count on the vtbl, releasing it when the count
 *   decrements to zero.
 *
 ****************************************************************************/

static void nsh_consolerelease(FAR struct nsh_vtbl_s *vtbl)
{
  FAR struct serial_s *pstate = (FAR struct serial_s *)vtbl;
  if (pstate->ss_refs > 1)
    {
      pstate->ss_refs--;
    }
  else
    {
      nsh_closeifnotclosed(pstate);
      free(vtbl);
    }
}

/****************************************************************************
 * Name: nsh_consoleredirect
 *
 * Description:
 *   Set up for redirected output
 *
 ****************************************************************************/

static FAR void *nsh_consoleredirect(FAR struct nsh_vtbl_s *vtbl, int fd)
{
  FAR struct serial_s *pstate = (FAR struct serial_s *)vtbl;
  void *ret;

  (void)nsh_openifnotopen(pstate);
  ret = pstate->ss_stream;
  fflush(pstate->ss_stream);

  pstate->ss_fd     = fd;
  pstate->ss_stream = NULL;  
  return ret;
}

/****************************************************************************
 * Name: nsh_consoleredirect
 *
 * Description:
 *   Set up for redirected output
 *
 ****************************************************************************/

static void nsh_consoleundirect(FAR struct nsh_vtbl_s *vtbl, FAR void *direct)
{
  FAR struct serial_s *pstate = (FAR struct serial_s *)vtbl;
  nsh_closeifnotclosed(pstate);
  pstate->ss_fd = -1;  
  pstate->ss_stream = (FILE*)direct;  
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nsh_main
 ****************************************************************************/

int nsh_consolemain(int argc, char *argv[])
{
  FAR struct serial_s *pstate = nsh_allocstruct();

  printf("NuttShell (NSH)\n");
  fflush(pstate->ss_stream);

  for (;;)
    {
      /* Display the prompt string */

      fputs(g_nshprompt, pstate->ss_stream);
      fflush(pstate->ss_stream);

      /* Get the next line of input */

      if (fgets(pstate->ss_line, CONFIG_EXAMPLES_NSH_LINELEN, stdin))
        {
          /* Parse process the command */

          (void)nsh_parse(&pstate->vtbl, pstate->ss_line);
          fflush(pstate->ss_stream);
        }
    }
}

/****************************************************************************
 * Name: cmd_exit
 *
 * Description:
 *   Exit the shell task
 *
 ****************************************************************************/

void cmd_exit(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv)
{
  exit(0);
}
