/****************************************************************************
 * sched/sem_internal.h
 *
 *   Copyright (C) 2007, 2009 Gregory Nutt. All rights reserved.
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

#ifndef __SEM_INTERNAL_H
#define __SEM_INTERNAL_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <sys/types.h>
#include <semaphore.h>
#include <sched.h>
#include <queue.h>
#include <nuttx/compiler.h>

/****************************************************************************
 * Definitions
 ****************************************************************************/

/****************************************************************************
 * Public Type Declarations
 ****************************************************************************/

/* This is the named semaphore structure */

struct nsem_s
{
  FAR struct nsem_s *flink;     /* Forward link */
  FAR struct nsem_s *blink;     /* Backward link */
  uint16             nconnect;  /* Number of connections to semaphore */
  FAR char          *name;      /* Semaphore name (NULL if un-named) */
  boolean            unlinked;  /* TRUE if the semaphore has been unlinked */
  sem_t              sem;       /* The semaphore itself */
};
typedef struct nsem_s nsem_t;

/****************************************************************************
 * Public Variables
 ****************************************************************************/

/* This is a list of dyanamically allocated named semaphores */

extern dq_queue_t g_nsems;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifdef __cplusplus
#define EXTERN extern "C"
extern "C" {
#else
#define EXTERN extern
#endif

EXTERN void weak_function sem_initialize(void);
EXTERN void               sem_waitirq(FAR _TCB *wtcb);
EXTERN FAR nsem_t        *sem_findnamed(const char *name);

#ifdef CONFIG_PRIORITY_INHERITANCE
EXTERN void sem_initholders(void);
EXTERN void sem_destroyholder(sem_t *sem);
EXTERN void sem_addholder(sem_t *sem);
EXTERN void sem_boostpriority(sem_t *sem);
EXTERN void sem_releaseholder(sem_t *sem);
EXTERN void sem_restorebaseprio(sem_t *sem);
#else
#  define sem_initholders()
#  define sem_destroyholder(sem)
#  define sem_addholder(sem)
#  define sem_boostpriority(sem)
#  define sem_releaseholder(sem)
#  define sem_restorebaseprio(sem)
#endif

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* __SEM_INTERNAL_H */

