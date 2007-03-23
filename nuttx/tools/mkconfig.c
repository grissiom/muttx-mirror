/************************************************************
 * mkconfig.c
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

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>

#define DEFCONFIG ".config"
#define LINESIZE  256

static char line[LINESIZE+1];

static char *skip_space(char *ptr)
{
  while (*ptr && isspace(*ptr)) ptr++;
  return ptr;
}

static char *find_name_end(char *ptr)
{
  while (*ptr && (isalnum(*ptr) || *ptr == '_')) ptr++;
  return ptr;
}

static char *find_value_end(char *ptr)
{
  while (*ptr && !isspace(*ptr))
    {
      if (*ptr == '"')
        {
           do ptr++; while (*ptr && *ptr != '"');
           if (*ptr) ptr++;
        }
      else
        {
           do ptr++; while (*ptr && !isspace(*ptr) && *ptr != '"');
        }
    }
  return ptr;
}

static char *read_line(FILE *stream)
{
  char *ptr;

  for (;;)
    {
      line[LINESIZE] = '\0';
      if (!fgets(line, LINESIZE, stream))
        {
          return NULL;
        }
      else
        {
          ptr = skip_space(line);
          if (*ptr && *ptr != '#' && *ptr != '\n')
            {
              return ptr;
            }
        }
    }
}

static void parse_line(char *ptr, char **varname, char **varval)
{
  *varname = ptr;
  *varval = NULL;

   ptr = find_name_end(ptr);
   if (*ptr && *ptr != '=')
    {
      *ptr = '\0';
       ptr = skip_space(ptr + 1);
    }

  if (*ptr == '=')
    {
      *ptr = '\0';
      ptr = skip_space(ptr + 1);
      if (*ptr)
        {
          *varval = ptr;
          ptr = find_value_end(ptr);
          *ptr = '\0';
        }
    }
}

static void parse_file(FILE *stream)
{
  char *varname;
  char *varval;
  char *ptr;

  do
    {
      ptr = read_line(stream);
      if (ptr)
        {
          parse_line(ptr, &varname, &varval);
          if (varname)
            {
              if (!varval || strcmp(varval, "n") == 0)
                {
                  printf("#undef %s\n", varname);
                }
              else if (strcmp(varval, "y") == 0)
                {
                  printf("#define %s 1\n", varname);
                }
              else
                {
                  printf("#define %s %s\n", varname, varval);
                }
            }
        }
    }
  while (ptr);
}

static void show_usage(const char *progname)
{
  fprintf(stderr, "USAGE: %s <abs path to .config>\n", progname);
  exit(1);
}

int main(int argc, char **argv, char **envp)
{
  char *filepath;
  FILE *stream;
  int status;

  if (argc != 2)
    {
      fprintf(stderr, "Unexpected number of arguments\n");
      show_usage(argv[0]);
    }

  status = asprintf(&filepath, "%s/" DEFCONFIG, argv[1]);
  if (status < 0)
    {
      fprintf(stderr, "asprintf failed\n");
      exit(2);
    }

  stream= fopen(filepath, "r");
  if (!stream)
    {
      fprintf(stderr, "open %s failed: %s\n", filepath, strerror(errno));
      exit(3);
    }

  printf("/* config.h -- Autogenerated! Do not edit. */\n\n");
  printf("#ifndef __ARCH_CONFIG_H\n");
  printf("#define __ARCH_CONFIG_H\n\n");
  printf("/* Architecture-specific options *************************/\n\n");
  parse_file(stream);
  printf("\n/* Sanity Checks *****************************************/\n\n");
  printf("/* The correct way to disable RR scheduling is to set the\n");
  printf(" * timeslice to zero.\n");
  printf(" */\n\n");
  printf("#ifndef CONFIG_RR_INTERVAL\n");
  printf("# define CONFIG_RR_INTERVAL 0\n");
  printf("#endif\n\n");
  printf("/* The correct way to disable filesystem supuport is to set the\n");
  printf(" * number of file descriptors to zero.\n");
  printf(" */\n\n");
  printf("#ifndef CONFIG_NFILE_DESCRIPTORS\n");
  printf("# define CONFIG_NFILE_DESCRIPTORS 0\n");
  printf("#endif\n\n");
  printf("/* If a console is selected, then make sure that there are\n");
  printf(" * resources for 3 file descriptors and, if any streams are\n");
  printf(" * selected, also for 3 file streams.\n");
  printf(" */\n\n");
  printf("#ifdef CONFIG_DEV_CONSOLE\n");
  printf("# if CONFIG_NFILE_DESCRIPTORS < 3\n");
  printf("#   undef CONFIG_NFILE_DESCRIPTORS\n");
  printf("#   define CONFIG_NFILE_DESCRIPTORS 3\n");
  printf("# endif\n\n");
  printf("# if CONFIG_NFILE_STREAMS > 0 && CONFIG_NFILE_STREAMS < 3\n");
  printf("#  undef CONFIG_NFILE_STREAMS\n");
  printf("#  define CONFIG_NFILE_STREAMS 3\n");
  printf("# endif\n");
  printf("#endif\n\n");
  printf("/* If no file descriptors are configured, then make certain no\n");
  printf(" * streams are configured either.\n");
  printf(" */\n\n");
  printf("#if CONFIG_NFILE_DESCRIPTORS == 0\n");
  printf("# undef CONFIG_NFILE_STREAMS\n");
  printf("# define CONFIG_NFILE_STREAMS 0\n");
  printf("#endif\n\n");
  printf("/* There must be at least one memory region. */\n\n");
  printf("#ifndef CONFIG_MM_REGIONS\n");
  printf("# define CONFIG_MM_REGIONS 1\n");
  printf("#endif\n\n");
  printf("/* If no file streams are configured, then make certain that\n");
  printf(" * buffered I/O support is disabled.\n");
  printf(" */\n\n");
  printf("#if CONFIG_NFILE_STREAMS == 0\n");
  printf("# undef CONFIG_STDIO_BUFFER_SIZE\n");
  printf("# define CONFIG_STDIO_BUFFER_SIZE 0\n");
  printf("#endif\n\n");
  printf("/* Verbose debug only makes sense if debug is enabled */\n\n");
  printf("#ifndef CONFIG_DEBUG\n");
  printf("# undef CONFIG_DEBUG_VERBOSE\n");
  printf("#endif\n\n");
  printf("#endif /* __ARCH_CONFIG_H */\n");
  fclose(stream);
  return 0;
}
