/************************************************************
 * lib_puts.c
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
 ************************************************************/

/************************************************************
 * Compilation Switches
 ************************************************************/

/************************************************************
 * Included Files
 ************************************************************/

#include <stdio.h>
#include "lib_internal.h"

/************************************************************
 * Definitions
 ************************************************************/

/************************************************************
 * Private Type Declarations
 ************************************************************/

/************************************************************
 * Private Function Prototypes
 ************************************************************/

/************************************************************
 * Global Function Prototypes
 ************************************************************/

/************************************************************
 * Public Constant Data
 ************************************************************/

/************************************************************
 * Public Variables
 ************************************************************/

/************************************************************
 * Private Constant Data
 ************************************************************/

/************************************************************
 * Private Variables
 ************************************************************/

/************************************************************
 * Public Functions
 ************************************************************/

/************************************************************
 * Name: puts
 *
 * Description:
 *   puts() writes the string s and a trailing newline to
 *   stdout.
 ************************************************************/

#if CONFIG_NFILE_STREAMS > 0

int puts(const char *s)
{
  int nwritten;
  int nput = EOF;

  /* Write the string (the next two steps must be atomic) */

  lib_take_semaphore(stdout);

  /* Write the string without its trailing '\0' */

  nwritten = fputs(s, stdout);
  if (nwritten > 0)
    {
      /* Followed by a newline */
      char newline = '\n';
      if (lib_fwrite(&newline, 1, stdout) > 0)
        {
          nput = nwritten + 1;
        }
    }
  lib_give_semaphore(stdout);
  return nput;
}

#endif /* CONFIG_NFILE_STREAMS */
