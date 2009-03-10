/****************************************************************************
 * sched/sem_wait.c
 *
 *   Copyright (C) 2007-2009 Gregory Nutt. All rights reserved.
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

#include <sys/types.h>
#include <semaphore.h>
#include <errno.h>
#include <assert.h>
#include <nuttx/arch.h>
#include "os_internal.h"
#include "sem_internal.h"

/****************************************************************************
 * Compilation Switches
 ****************************************************************************/

/****************************************************************************
 * Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Type Declarations
 ****************************************************************************/

/****************************************************************************
 * Global Variables
 ****************************************************************************/

/****************************************************************************
 * Private Variables
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Function:  sem_wait
 *
 * Description:
 *   This function attempts to lock the semaphore referenced by 'sem'.  If
 *   the semaphore value is (<=) zero, then the calling task will not return
 *   until it successfully acquires the lock.
 *
 * Parameters:
 *   sem - Semaphore descriptor.
 *
 * Return Value:
 *   0 (OK), or -1 (ERROR) is unsuccessful
 *   If this function returns -1 (ERROR), then the cause of the failure will
 *   be reported in 'errno' as:
 *   - EINVAL:  Invalid attempt to get the semaphore
 *   - EINTR:   The wait was interrupted by the receipt of a signal.
 *
 * Assumptions:
 *
 ****************************************************************************/

int sem_wait(FAR sem_t *sem)
{
  FAR _TCB  *rtcb = (FAR _TCB*)g_readytorun.head;
#ifdef CONFIG_PRIORITY_INHERITANCE
  FAR _TCB  *htcb;
#endif
  int        ret = ERROR;
  irqstate_t saved_state;

  /* This API should not be called from interrupt handlers */

  DEBUGASSERT(up_interrupt_context() == FALSE)

  /* Assume any errors reported are due to invalid arguments. */

  errno = EINVAL;

  if (sem)
    {
      /* The following operations must be performed with interrupts
       * disabled because sem_post() may be called from an interrupt
       * handler.
       */

      saved_state = irqsave();

      /* Check if the lock is available */

      if (sem->semcount > 0)
        {
          /* It is, let the task take the semaphore. */

          sem->semcount--;
#ifdef CONFIG_PRIORITY_INHERITANCE
          sem->holder = rtcb;
#endif
          rtcb->waitsem = NULL;
          ret = OK;
        }

      /* The semaphore is NOT available, We will have to block the
       * current thread of execution.
       */

      else
        {
          /* First, verify that the task is not already waiting on a
           * semaphore
           */

          if (rtcb->waitsem != NULL)
            {
              PANIC(OSERR_BADWAITSEM);
            }

          /* Handle the POSIX semaphore (but don't set the owner yet) */

          sem->semcount--;

          /* Save the waited on semaphore in the TCB */

          rtcb->waitsem = sem;

          /* If priority inheritance is enabled, then check the priority of
           * the holder of the semaphore.
           */

#ifdef CONFIG_PRIORITY_INHERITANCE
          /* Disable context switching.  The following operations must be
           * atomic with regard to the scheduler.
           */

          sched_lock();
          htcb = sem->holder;
          if (htcb && htcb->sched_priority < rtcb->sched_priority)
            {
              /* Raise the priority of the holder of the semaphore.  This
               * cannot cause a context switch because we have preemption
               * disabled.  The task will be marked "pending" and the switch
               * will occur during up_block_task() processing.
               *
               * NOTE that we have to restore base_priority because
               * sched_setparam() should set both.
               */

              int base_priority = htcb->base_priority;
              (void)sched_settcbprio(htcb, rtcb->sched_priority);
              htcb->base_priority = base_priority;
            }
#endif
          /* Add the TCB to the prioritized semaphore wait queue */

          errno = 0;
          up_block_task(rtcb, TSTATE_WAIT_SEM);

#ifdef CONFIG_PRIORITY_INHERITANCE
          sched_unlock();
#endif
          /* When we resume at this point, either (1) the semaphore has been
           * assigned to this thread of execution, or (2) the semaphore wait
           * has been interrupted by a signal.  We can detect the latter case
           * be examining the errno value.
           */

          if (errno != EINTR)
            {
              /* We hold the semaphore */

#ifdef CONFIG_PRIORITY_INHERITANCE
              sem->holder = rtcb;
#endif
              ret = OK;
            }
          else
            {
              sem->semcount++;
            }
        }

      /* Interrupts may now be enabled. */

      irqrestore(saved_state);
    }

  return ret;
}
