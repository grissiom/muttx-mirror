/************************************************************
 * task_setup.c
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
 * Included Files
 ************************************************************/

#include <nuttx/config.h>
#include <sys/types.h>
#include <sched.h>
#include <string.h>
#include <errno.h>
#include <debug.h>
#include <nuttx/arch.h>
#include "os_internal.h"

/************************************************************
 * Definitions
 ************************************************************/

/************************************************************
 * Private Type Declarations
 ************************************************************/

/************************************************************
 * Global Variables
 ************************************************************/

/************************************************************
 * Private Variables
 ************************************************************/

/* This is the name for un-named tasks */

static const char g_noname[] = "<noname>";

/************************************************************
 * Private Function Prototypes
 ************************************************************/

static STATUS task_assignpid(FAR _TCB* tcb);

/************************************************************
 * Private Functions
 ************************************************************/

/************************************************************
 * Name: task_assignpid
 *
 * Description:
 *   This function assigns the next unique task ID to a task.
 *
 * Inputs:
 *   tcb - TCB of task
 *
 * Return:
 *   OK on success; ERROR on failure (errno is not set)
 *
 ************************************************************/

static STATUS task_assignpid(FAR _TCB *tcb)
{
  pid_t next_pid;
  int   hash_ndx;
  int   tries = 0;

  /* Disable pre-emption.  This should provide sufficient protection
   * for the following operation.
   */

  (void)sched_lock();

  /* We'll try every allowable pid */

  for (tries = 0; tries < CONFIG_MAX_TASKS; tries++)
    {
      /* Get the next process ID candidate */

      next_pid = ++g_lastpid;

      /* Verify that the next_pid is in the valid range */

      if (next_pid <= 0)
        {
          g_lastpid = 1;
          next_pid  = 1;
        }

      /* Get the hash_ndx associated with the next_pid */

      hash_ndx = PIDHASH(next_pid);

      /* Check if there is a (potential) duplicate of this pid */

      if (!g_pidhash[hash_ndx].tcb)
        {
          g_pidhash[hash_ndx].tcb = tcb;
          g_pidhash[hash_ndx].pid = next_pid;
          tcb->pid = next_pid;
          (void)sched_unlock();
          return OK;
        }
    }

  /* If we get here, then the g_pidhash[] table is completely full.
   * We cannot allow another task to be started.
   */

  (void)sched_unlock();
  return ERROR;
}

/************************************************************
 * Public Functions
 ************************************************************/

/************************************************************
 * Name: task_schedsetup
 *
 * Description:
 *   This functions initializes a Task Control Block (TCB)
 *   in preparation for starting a new thread.
 *
 *   task_schedsetup() is called from task_init(),
 *   task_start(), and pthread_create();
 *
 * Input Parameters:
 *   tcb        - Address of the new task's TCB
 *   priority   - Priority of the new task
 *   entry      - Entry point of a new task
 *   main       - Application start point of the new task
 *
 * Return Value:
 *   OK on success; ERROR on failure.
 *
 *   This function can only failure is it is unable to assign
 *   a new, unique task ID to the TCB (errno is not set).
 *
 ************************************************************/

STATUS task_schedsetup(FAR _TCB *tcb, int priority,
                       start_t start, main_t main)
{
  STATUS ret;

  /* Assign a unique task ID to the task. */

  ret = task_assignpid(tcb);
  if (ret == OK)
    {
      /* Save task priority and entry point in the TCB */

      tcb->init_priority  = (ubyte)priority;
      tcb->sched_priority = (ubyte)priority;
      tcb->start          = start;
      tcb->entry.main     = main;

      /* Initialize other (non-zero) elements of the TCB */

#ifndef CONFIG_DISABLE_SIGNALS
      tcb->sigprocmask  = ALL_SIGNAL_SET;
#endif
      tcb->task_state   = TSTATE_TASK_INVALID;

      /* Initialize the processor-specific portion of the TCB */

      up_initial_state(tcb);

      /* Add the task to the inactive task list */

      sched_lock();
      dq_addfirst((FAR dq_entry_t*)tcb, (dq_queue_t*)&g_inactivetasks);
      tcb->task_state = TSTATE_TASK_INACTIVE;
      sched_unlock();
    }

  return ret;
}

/************************************************************
 * Name: task_argsetup
 *
 * Description:
 *   This functions sets up parameters in the Task Control
 *   Block (TCB) in preparation for starting a new thread.
 *
 *   task_argsetup() is called only from task_init() and
 *   task_start() to create a new task.  Argumens are
 *   cloned via strdup.
 *
 * Input Parameters:
 *   tcb        - Address of the new task's TCB
 *   name       - Name of the new task (not used)
 *   argv       - A pointer to an array of input parameters.
 *                Up to CONFIG_MAX_TASK_ARG parameters may be
 *                provided. If fewer than CONFIG_MAX_TASK_ARG
 *                parameters are passed, the list should be
 *                terminated with a NULL argv[] value.
 *                If no parameters are required, argv may be NULL.
 *
 * Return Value:
 *  OK
 *
 ************************************************************/

STATUS task_argsetup(FAR _TCB *tcb, const char *name, char *argv[])
{
  int i;

#if CONFIG_TASK_NAME_SIZE > 0
  /* Give a name to the unnamed tasks */

  if (!name)
    {
      name = (char *)g_noname;
    }

  /* Copy the name into the TCB */

  strncpy(tcb->name, name, CONFIG_TASK_NAME_SIZE);

  /* Save the name as the first argument */

  tcb->argv[0] = tcb->name;
#else
  /* Save the name as the first argument */

  tcb->argv[0] = (char *)g_noname;
#endif /* CONFIG_TASK_NAME_SIZE */

  /* For tasks, the life of the argument must be as long as
   * the life of the task and the arguments must be strings.
   * So for tasks, we have to to dup the strings.
   *
   * The first NULL argument terminates the list of 
   * arguments.  The argv pointer may be NULL if no
   * parameters are passed.
   */

  i = 1;
  if (argv)
    {
      for (; i < CONFIG_MAX_TASK_ARGS+1 && argv[i-1]; i++)
        {
          tcb->argv[i] = strdup(argv[i-1]);
        }
    }

  /* Nullify any unused argument storage */

  for (; i < CONFIG_MAX_TASK_ARGS+1; i++)
    {
      tcb->argv[i] = NULL;
    }

  return OK;
}
