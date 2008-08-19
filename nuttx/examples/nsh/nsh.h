/****************************************************************************
 * examples/nsh/nsh.h
 *
 *   Copyright (C) 2007, 2008 Gregory Nutt. All rights reserved.
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

#ifndef __NSH_H
#define __HSH_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#ifdef CONFIG_EXAMPLES_NSH_CONSOLE
# include <stdio.h>
#endif

/****************************************************************************
 * Definitions
 ****************************************************************************/

#if !defined(CONFIG_EXAMPLES_NSH_CONSOLE) && !defined(CONFIG_EXAMPLES_NSH_TELNET)
#  error "No NSH front end defined"
#endif

/* This is the maximum number of arguments that will be accepted for a command */

#define NSH_MAX_ARGUMENTS 6

/* strerror() produces much nicer output but is, however, quite large and
 * will only be used if CONFIG_EXAMPLES_NSH_STRERROR is defined.
 */

#ifdef CONFIG_EXAMPLES_NSH_STRERROR
#  define NSH_ERRNO strerror(errno)
#else
#  define NSH_ERRNO errno
#endif

/* Maximum size of one command line (telnet or serial) */

#ifndef CONFIG_EXAMPLES_NSH_LINELEN
#  define CONFIG_EXAMPLES_NSH_LINELEN 80
#endif

/* The following two settings are used only in the telnetd interface */

#ifndef CONFIG_EXAMPLES_NSH_IOBUFFER_SIZE
# define CONFIG_EXAMPLES_NSH_IOBUFFER_SIZE 512
#endif

/* As threads are created to handle each request, a stack must be allocated
 * for the thread.  Use a default if the user provided no stacksize.
 */

#ifndef CONFIG_EXAMPLES_NSH_STACKSIZE
# define CONFIG_EXAMPLES_NSH_STACKSIZE 4096
#endif

/* The maximum number of nested if-then[-else]-fi sequences that
 * are permissable.
 */

#ifndef CONFIG_EXAMPLES_NSH_NESTDEPTH
# define CONFIG_EXAMPLES_NSH_NESTDEPTH 3
#endif

/* Define to enable dumping of all input/output buffers */

#undef CONFIG_EXAMPLES_NSH_TELNETD_DUMPBUFFER
#undef CONFIG_EXAMPLES_NSH_FULLPATH

#define nsh_clone(v)           (v)->clone(v)
#define nsh_addref(v)          (v)->addref(v)
#define nsh_release(v)         (v)->release(v)
#define nsh_linebuffer(v)      (v)->linebuffer(v)
#define nsh_redirect(v,f,s)    (v)->redirect(v,f,s)
#define nsh_undirect(v,s)      (v)->undirect(v,s)
#define nsh_exit(v)            (v)->exit(v)

#ifdef CONFIG_CPP_HAVE_VARARGS
# define nsh_output(v, fmt...) (v)->output(v, ##fmt)
#else
# define nsh_output            vtbl->output
#endif

#define SAVE_SIZE (sizeof(int) + sizeof(FILE*) + sizeof(boolean))

/****************************************************************************
 * Public Types
 ****************************************************************************/

enum nsh_parser_e
{
   NSH_PARSER_NORMAL = 0,
   NSH_PARSER_IF,
   NSH_PARSER_THEN,
   NSH_PARSER_ELSE
};

struct nsh_state_s
{
  ubyte     ns_ifcond   : 1; /* Value of command in 'if' statement */
  ubyte     ns_disabled : 1; /* TRUE: Unconditionally disabled */
  ubyte     ns_unused   : 4;
  ubyte     ns_state    : 2; /* Parser state (see enum nsh_parser_e) */
};

struct nsh_parser_s
{
  boolean   np_bg;       /* TRUE: The last command executed in background */
  boolean   np_redirect; /* TRUE: Output from the last command was re-directed */
  boolean   np_fail;     /* TRUE: The last command failed */
  ubyte     np_ndx;      /* Current index into np_st[] */
  int       np_nice;     /* "nice" value applied to last background cmd */

  /* This is a stack of parser state information.  It supports nested
   * execution of commands that span multiple lines (like if-then-else-fi)
   */

  struct nsh_state_s np_st[CONFIG_EXAMPLES_NSH_NESTDEPTH];
};

struct nsh_vtbl_s
{
  /* This function pointers are "hooks" into the front end logic to
   * handle things like output of command results, redirection, etc.
   * -- all of which must be done in a way that is unique to the nature
   * of the front end.
   */
   
  FAR struct nsh_vtbl_s *(*clone)(FAR struct nsh_vtbl_s *vtbl);
  void (*addref)(FAR struct nsh_vtbl_s *vtbl);
  void (*release)(FAR struct nsh_vtbl_s *vtbl);
  int (*output)(FAR struct nsh_vtbl_s *vtbl, const char *fmt, ...);
  FAR char *(*linebuffer)(FAR struct nsh_vtbl_s *vtbl);
  void (*redirect)(FAR struct nsh_vtbl_s *vtbl, int fd, FAR ubyte *save);
  void (*undirect)(FAR struct nsh_vtbl_s *vtbl, FAR ubyte *save);
  void (*exit)(FAR struct nsh_vtbl_s *vtbl);

  /* Parser state data */

  struct nsh_parser_s np;
};

typedef int (*cmd_t)(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);

/****************************************************************************
 * Public Data
 ****************************************************************************/

extern const char g_nshprompt[];
extern const char g_fmtargrequired[];
extern const char g_fmtarginvalid[];
extern const char g_fmtcmdnotfound[];
extern const char g_fmtcmdnotimpl[];
extern const char g_fmtnosuch[];
extern const char g_fmttoomanyargs[];
extern const char g_fmtdeepnesting[];
extern const char g_fmtcontext[];
extern const char g_fmtcmdfailed[];
extern const char g_fmtcmdoutofmemory[];
extern const char g_fmtinternalerror[];

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/* Message handler */

extern int nsh_parse(FAR struct nsh_vtbl_s *vtbl, char *cmdline);

/* I/O interfaces */

#ifdef CONFIG_EXAMPLES_NSH_CONSOLE
extern int nsh_consolemain(int argc, char *argv[]);
#endif

#ifdef CONFIG_EXAMPLES_NSH_TELNET
extern int nsh_telnetmain(int argc, char *argv[]);
#endif

/* Shell command handlers */

extern int cmd_echo(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
extern int cmd_exec(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
extern int cmd_ps(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);

#if CONFIG_NFILE_DESCRIPTORS > 0
  extern int cmd_cat(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
  extern int cmd_cp(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
  extern int cmd_ls(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
# if CONFIG_NFILE_STREAMS > 0
  extern int cmd_sh(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
# endif  /* CONFIG_NFILE_STREAMS */
# ifndef CONFIG_DISABLE_MOUNTPOINT
    extern int cmd_mkdir(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
    extern int cmd_mkfifo(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
    extern int cmd_rm(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
    extern int cmd_rmdir(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
#   ifdef CONFIG_FS_FAT
      extern int cmd_mkfatfs(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
      extern int cmd_mount(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
      extern int cmd_umount(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
#   endif /* CONFIG_FS_FAT */
# endif /* !CONFIG_DISABLE_MOUNTPOINT */
#endif /* CONFIG_NFILE_DESCRIPTORS */

#if defined(CONFIG_NET) && CONFIG_NSOCKET_DESCRIPTORS > 0
  extern int cmd_ifconfig(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
#endif

#ifndef CONFIG_DISABLE_ENVIRON
  extern int cmd_set(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
  extern int cmd_unset(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
#endif /* CONFIG_DISABLE_ENVIRON */

#ifndef CONFIG_DISABLE_SIGNALS
  extern int cmd_sleep(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
  extern int cmd_usleep(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
#endif /* CONFIG_DISABLE_SIGNALS */

#endif /* __NSH_H */
