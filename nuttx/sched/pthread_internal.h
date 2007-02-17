/************************************************************
 * pthread_internal.h
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

#ifndef __PTHREAD_INTERNAL_H
#define __PTHREAD_INTERNAL_H

/************************************************************
 * Included Files
 ************************************************************/

#include <sys/types.h>
#include <pthread.h>
#include <nuttx/compiler.h>

/************************************************************
 * Definitions
 ************************************************************/

/************************************************************
 * Public Type Declarations
 ************************************************************/

/* The following defines an entry in the pthread logic's
 * local data set.  Note that this structure is used to
 * implemented a singly linked list.  This structure
 * is used (instead of, say, a binary search tree) because
 * the data set will be searched using the pid as
 * a key -- a process IDs will always be created in a
 * montonically increasing fashion.
 */

struct join_s 
{
  struct join_s *next;           /* Implements link list */
  boolean        started;        /* TRUE: pthread started. */
  boolean        detached;       /* TRUE: pthread_detached'ed */
  boolean        terminated;     /* TRUE: detach'ed+exit'ed */
  pthread_t      thread;         /* Includes pid */
  sem_t          exit_sem;       /* Implements join */
  sem_t          data_sem;       /* Implements join */
  pthread_addr_t exit_value;     /* Returned data */

};
typedef struct join_s join_t;

/************************************************************
 * Public Variables
 ************************************************************/

/* This is the head of a private singly linked list.  It
 * is used to retain information about the spawned threads.
 */

extern join_t *g_pthread_head;
extern join_t *g_pthread_tail;

/* Mutually exclusive access to this data set is enforced with
 * the following (un-named) semaphore.
 */

extern sem_t g_join_semaphore;

/* This keys track of the number of global keys that have been
 * allocated.
 */

extern ubyte g_pthread_num_keys;

/* Default pthread attributes */

extern pthread_attr_t g_default_pthread_attr;

/************************************************************
 * Public Function Prototypes
 ************************************************************/

#ifdef __cplusplus
#define EXTERN extern "C"
extern "C" {
#else
#define EXTERN extern
#endif

EXTERN void weak_function pthread_initialize(void);
EXTERN int                pthread_completejoin(pid_t pid, void *exit_value);
EXTERN join_t            *pthread_findjoininfo(int pid);
EXTERN int                pthread_givesemaphore(sem_t *sem);
EXTERN join_t            *pthread_removejoininfo(int pid);
EXTERN int                pthread_takesemaphore(sem_t *sem);

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* __PTHREAD_INTERNAL_H */

