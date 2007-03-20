/********************************************************************************
 * sched.h
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
 ********************************************************************************/

#ifndef __SCHED_H
#define __SCHED_H

/********************************************************************************
 * Included Files
 ********************************************************************************/

#include <nuttx/config.h>
#include <nuttx/sched.h>

/********************************************************************************
 * Definitions
 ********************************************************************************/

/* Task Management Definitins ***************************************************/

/* POSIX-like scheduling policies */

#define SCHED_FIFO  1  /* FIFO per priority scheduling policy */
#define SCHED_RR    2  /* Round robin scheduling policy */
#define SCHED_OTHER 4  /* Not used */

/* Pthread definitions **********************************************************/

#define PTHREAD_KEYS_MAX CONFIG_NPTHREAD_KEYS

/********************************************************************************
 * Global Type Definitions
 ********************************************************************************/

/* This is the POSIX-like scheduling parameter structure */

struct sched_param
{
  int sched_priority;
};

/********************************************************************************
 * Global Function Prototypes
 ********************************************************************************/

#ifndef __ASSEMBLY__
#undef EXTERN
#if defined(__cplusplus)
#define EXTERN extern "C"
extern "C" {
#else
#define EXTERN extern
#endif

/* Task Control Interfaces (non-standard) */

#ifndef CONFIG_CUSTOM_STACK
EXTERN STATUS  task_init(FAR _TCB *tcb, const char *name, int priority,
                         FAR uint32 *stack, uint32 stack_size,
                         main_t entry, char *argv[]);
#else
EXTERN STATUS  task_init(FAR _TCB *tcb, const char *name, int priority,
                         main_t entry, char *argv[]);
#endif
EXTERN STATUS  task_activate(FAR _TCB *tcb);
#ifndef CONFIG_CUSTOM_STACK
EXTERN int     task_create(const char *name, int priority, int stack_size,
                           main_t entry, char *argv[]);
#else
EXTERN int     task_create(const char *name, int priority,
                           main_t entry, char *argv[]);
#endif
EXTERN STATUS  task_delete(pid_t pid);
EXTERN STATUS  task_restart(pid_t pid);

/* Task Scheduling Interfaces (based on POSIX APIs) */

EXTERN int     sched_setparam(pid_t pid, const struct sched_param *param);
EXTERN int     sched_getparam(pid_t pid, struct sched_param *param);
EXTERN int     sched_setscheduler(pid_t pid, int policy,
                                  const struct sched_param *param);
EXTERN int     sched_getscheduler(pid_t pid);
EXTERN int     sched_yield(void);
EXTERN int     sched_get_priority_max(int policy);
EXTERN int     sched_get_priority_min(int policy);
EXTERN int     sched_rr_get_interval(pid_t pid, struct timespec *interval);

/* Task Switching Interfaces (non-standard) */

EXTERN STATUS  sched_lock(void);
EXTERN STATUS  sched_unlock(void);
EXTERN sint32  sched_lockcount(void);

/* If instrumentation of the scheduler is enabled, then some
 * outboard logic must provide the following interfaces.
 */

#ifdef CONFIG_SCHED_INSTRUMENTATION

EXTERN void    sched_note_start(FAR _TCB *tcb );
EXTERN void    sched_note_stop(FAR _TCB *tcb );
EXTERN void    sched_note_switch(FAR _TCB *pFromTcb, FAR _TCB *pToTcb);

#else
# define sched_note_start(t)
# define sched_note_stop(t)
# define sched_note_switch(t1, t2)
#endif /* CONFIG_SCHED_INSTRUMENTATION */

#undef EXTERN
#if defined(__cplusplus)
}
#endif
#endif /* __ASSEMBLY__ */

#endif /* __SCHED_H */
