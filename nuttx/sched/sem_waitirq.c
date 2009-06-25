/****************************************************************************
 * sched/sem_waitirq.c
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <sys/types.h>
#include <sched.h>
#include <errno.h>
#include <nuttx/arch.h>
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
 * Function:  sem_waitirq
 *
 * Description:
 *   This function is called when a signal is received by a
 *   task that is waiting on a semaphore.  According to the
 *   POSIX spec, "...the calling thread shall not return
 *   from the call to [sem_wait] until it either locks the
 *   semaphore or the call is interrupted by a signal."
 *
 * Parameters:
 *   wtcb - A pointer to the TCB of the task that is waiting
 *      on a semphare, but has received a signal instead.
 *
 * Return Value:
 *   None
 *
 * Assumptions:
 *
 ****************************************************************************/

void sem_waitirq(FAR _TCB *wtcb)
{
  irqstate_t saved_state;

  /* Disable interrupts.  This is necessary (unfortunately) because an
   * interrupt handler may attempt to post the semaphore while we are
   * doing this.
   */

  saved_state = irqsave();

  /* It is possible that an interrupt/context switch beat us to the
   * punch and already changed the task's state.
   */

  if (wtcb->task_state == TSTATE_WAIT_SEM)
    {
      /* Indicate that the semaphore wait is over. */

      wtcb->waitsem = NULL;

      /* Mark the errno value for the thread. */

      wtcb->pterrno = EINTR;

      /* Restart the task. */

      up_unblock_task(wtcb);
    }

  /* Interrupts may now be enabled. */

  irqrestore(saved_state);
}

