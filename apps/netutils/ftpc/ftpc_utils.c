/****************************************************************************
 * apps/netutils/ftpc/ftpc_utils.c
 *
 *   Copyright (C) 2011 Gregory Nutt. All rights reserved.
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

#include <stdlib.h>

#include "ftpc_internal.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Types
 ****************************************************************************/

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
 * Name: ftpc_nibble
 *
 * Description:
 *   Convert a ASCII hex 'digit' to binary
 *
 ****************************************************************************/

int ftpc_nibble(char ch)
{
  if (ch >= '0' && ch <= '9')
    {
      return (unsigned int)ch - '0';
    }
  else if (ch >= 'A' && ch <= 'F')
    {
      return (unsigned int)ch - 'A' + 10;
    }
  else if (ch >= 'a' && ch <= 'f')
    {
      return (unsigned int)ch - 'a' + 10;
    }
  return ERROR;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ftpc_reset
 *
 * Description:
 *   Reset the FTP session.
 *
 ****************************************************************************/

void ftpc_reset(struct ftpc_session_s *session)
{
  ftpc_sockclose(&session->data);
  ftpc_sockclose(&session->cmd);
  free(session->uname);
  session->uname      = NULL;
  free(session->pwd);
  session->pwd        = NULL;
  free(session->initdir);
  session->initdir    = NULL;
  session->flags      = FTPC_FLAGS_INIT;
  session->xfrmode    = FTPC_XFRMODE_UNKNOWN;
  session->code       = 0;
  session->replytimeo = CONFIG_FTP_DEFTIMEO * CLOCKS_PER_SEC;
  session->conntimeo  = CONFIG_FTP_DEFTIMEO * CLOCKS_PER_SEC;
}

/****************************************************************************
 * Name: ftpc_curdir
 *
 * Description:
 *   Update the current working directory
 *
 ****************************************************************************/

void ftpc_curdir(struct ftpc_session_s *session)
{
  free(session->prevdir);
  session->prevdir = session->curdir;
  session->curdir = ftpc_pwd((SESSION)session);
}

/****************************************************************************
 * Name: ftpc_stripcrlf
 *
 * Description:
 *   Strip any trailing carriage returns or line feeds from a string.
 *
 ****************************************************************************/

void ftpc_stripcrlf(FAR char *str)
{
  FAR char *ptr;
  int len;

  if (str)
    {
      len = strlen(str);
      if (len > 0)
        {
          ptr = str + len - 1;
          while (*ptr == '\r' || *ptr == '\n')
            {
              *ptr = '\0';
              ptr--;
            }
        }
    }
}

/****************************************************************************
 * Name: ftpc_stripslash
 *
 * Description:
 *   Strip any trailing slashes from a string.
 *
 ****************************************************************************/

FAR char *ftpc_stripslash(FAR char *str)
{
  FAR char *ptr;

  if (!str || !*str)
    return str;

  ptr = strchr(str, 0);
  if(!ptr)
    return str;
  ptr--;
  if(*ptr == '/') {
    if(ptr != str) /* root directory */
      *ptr = 0;
  }
  return str;
}

/****************************************************************************
 * Name: ftpc_stripslash
 *
 * Description:
 *   Convert quoted hexadecimal constants to binary values.
 *
 ****************************************************************************/

FAR char *ftpc_dequote(FAR const char *str)
{
  FAR char *allocstr = NULL;
  FAR char *ptr;
  int ms;
  int ls;
  int len;

  if (str)
    {
      /* Allocate space for a modifiable copy of the string */

      len      = strlen(str);
      allocstr = (FAR char*)malloc(len+1);
      if (allocstr)
        {
          /* Search the string */
 
          ptr = allocstr;
          while (*str)
            {
              /* Check for a quoted hex value */

              if (str[0] == '%')
                {
                  /* Extract the hex value */

                  ms = ftpc_nibble(str[1]);
                  if (ms >= 0)
                    {
                      ls = ftpc_nibble(str[2]);
                      if (ls >= 0)
                        {
                          /* Save the binary value and skip ahead by 3 */

                          *ptr++ = (char)(ms << 8 | ls);
                          str += 3;
                          continue;
                        }
                    }
                }

              /* Just transfer the character */

              *ptr++ = *str++;
            }

          /* NUL terminate */

          *ptr = '\0';
        }
    }

  return allocstr;
}
