/************************************************************
 * arch.h
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

#ifndef __ARCH_H
#define __ARCH_H

/************************************************************
 * Included Files
 ************************************************************/

#include <nuttx/config.h>
#include <sys/types.h>
#include <sched.h>
#include <arch/arch.h>

/************************************************************
 * Definitions
 ************************************************************/

/************************************************************
 * Public Types
 ************************************************************/

/************************************************************
 * Public Variables
 ************************************************************/

typedef void  (*sig_deliver_t)(_TCB *tcb);

/************************************************************
 * Public Function Prototypes
 ************************************************************/

#ifdef __cplusplus
#define EXTERN extern "C"
extern "C" {
#else
#define EXTERN extern
#endif

/************************************************************
 * These are standard interfaces that must be exported to the
 * scheduler from architecture-specific code.
 ************************************************************/

/************************************************************
 * Name: up_initialize
 *
 * Description:
 *   up_initialize will be called once during OS
 *   initialization after the basic OS services have been
 *   initialized.  The architecture specific details of
 *   initializing the OS will be handled here.  Such things as
 *   setting up interrupt service routines, starting the
 *   clock, and registering device drivers are some of the
 *   things that are different for each processor and hardware
 *   platform.
 *
 *   up_initialize is called after the OS initialized but
 *   before the init process has been started and before the
 *   libraries have been initialized.  OS services and driver
 *   services are available.
 *
 ************************************************************/

EXTERN void up_initialize(void);

/************************************************************
 * Name: up_idle
 *
 * Description:
 *   up_idle() is the logic that will be executed
 *   when their is no other ready-to-run task.  This is processor
 *   idle time and will continue until some interrupt occurs to
 *   cause a context switch from the idle task.
 *
 *   Processing in this state may be processor-specific. e.g.,
 *   this is where power management operations might be performed.
 *
 ************************************************************/

EXTERN void up_idle(void);

/************************************************************
 * Name: up_initial_state
 *
 * Description:
 *   A new thread is being started and a new TCB
 *   has been created. This function is called to initialize
 *   the processor specific portions of the new TCB.
 *
 *   This function must setup the intial architecture registers
 *   and/or  stack so that execution will begin at tcb->start
 *   on the next context switch.
 *
 ************************************************************/

EXTERN void up_initial_state(_TCB *tcb);

/************************************************************
 * Name: up_create_stack
 *
 * Description:
 *   Allocate a stack for a new thread and setup
 *   up stack-related information in the TCB.
 *
 *   The following TCB fields must be initialized:
 *   adj_stack_size: Stack size after adjustment for hardware,
 *     processor, etc.  This value is retained only for debug
 *     purposes.
 *   stack_alloc_ptr: Pointer to allocated stack
 *   adj_stack_ptr: Adjusted StatckAllocPtr for HW.  The
 *     initial value of the stack pointer.
 *
 * Inputs:
 *   tcb: The TCB of new task
 *   stack_size:  The requested stack size.  At least this much
 *     must be allocated.
 ************************************************************/

EXTERN STATUS up_create_stack(_TCB *tcb, size_t stack_size);

/************************************************************
 * Name: up_use_stack
 *
 * Description:
 *   Setup up stack-related information in the TCB
 *   using pre-allocated stack memory
 *
 *   The following TCB fields must be initialized:
 *   adj_stack_size: Stack size after adjustment for hardware,
 *     processor, etc.  This value is retained only for debug
 *     purposes.
 *   stack_alloc_ptr: Pointer to allocated stack
 *   adj_stack_ptr: Adjusted StatckAllocPtr for HW.  The
 *     initial value of the stack pointer.
 *
 * Inputs:
 *   tcb: The TCB of new task
 *   stack_size:  The allocated stack size.
 *
 ************************************************************/

EXTERN STATUS up_use_stack(_TCB *tcb, void *stack, size_t stack_size);

/************************************************************
 * Name: up_release_stack
 *
 * Description:
 *   A task has been stopped. Free all stack
 *   related resources retained int the defunct TCB.
 *
 ************************************************************/

EXTERN void up_release_stack(_TCB *dtcb);

/************************************************************
 * Name: up_unblock_task
 *
 * Description:
 *   A task is currently in an inactive task list
 *   but has been prepped to execute.  Move the TCB to the
 *   ready-to-run list, restore its context, and start execution.
 *
 * Inputs:
 *   tcb: Refers to the tcb to be unblocked.  This tcb is
 *     in one of the waiting tasks lists.  It must be moved to
 *     the ready-to-run list and, if it is the highest priority
 *     ready to run taks, executed.
 *
 ************************************************************/

EXTERN void up_unblock_task(_TCB *tcb);

/************************************************************
 * Name: up_block_task
 *
 * Description:
 *   The currently executing task at the head of
 *   the ready to run list must be stopped.  Save its context
 *   and move it to the inactive list specified by task_state.
 *
 * Inputs:
 *   tcb: Refers to a task in the ready-to-run list (normally
 *     the task at the the head of the list).  It most be
 *     stopped, its context saved and moved into one of the
 *     waiting task lists.  It it was the task at the head
 *     of the ready-to-run list, then a context to the new
 *     ready to run task must be performed.
 *   task_state: Specifies which waiting task list should be
 *     hold the blocked task TCB.
 *
 ************************************************************/

EXTERN void up_block_task(_TCB *tcb, tstate_t task_state);

/************************************************************
 * Name: up_release_pending
 *
 * Description:
 *   Release and ready-to-run tasks that have
 *   collected in the pending task list.  This can call a
 *   context switch if a new task is placed at the head of
 *   the ready to run list.
 *
 ************************************************************/

EXTERN void up_release_pending(void);

/************************************************************
 * Name: up_reprioritize_rtr
 *
 * Description:
 *   Called when the priority of a running or
 *   ready-to-run task changes and the reprioritization will 
 *   cause a context switch.  Two cases:
 *
 *   1) The priority of the currently running task drops and the next
 *      task in the ready to run list has priority.
 *   2) An idle, ready to run task's priority has been raised above the
 *      the priority of the current, running task and it now has the
 *      priority.
 *
 * Inputs:
 *   tcb: The TCB of the task that has been reprioritized
 *   priority: The new task priority
 *
 ************************************************************/

EXTERN void up_reprioritize_rtr(_TCB *tcb, ubyte priority);

/************************************************************
 * Name: _exit
 *
 * Description:
 *   This function causes the currently executing task to cease
 *   to exist.  This is a special case of task_delete().
 *
 ************************************************************/
/* Prototype is in unistd.h */

/************************************************************
 * Name: ip_assert and up_assert_code
 *
 * Description:
 *   Assertions may be handled in an architecture-specific
 *   way.
 *
 ************************************************************/
/* Prototype is in assert.h */

/************************************************************
 * Name: up_schedule_sigaction
 *
 * Description:
 *   This function is called by the OS when one or more
 *   signal handling actions have been queued for execution.
 *   The architecture specific code must configure things so
 *   that the 'igdeliver' callback is executed on the thread
 *   specified by 'tcb' as soon as possible.
 *
 *   This function may be called from interrupt handling logic.
 *
 *   This operation should not cause the task to be unblocked
 *   nor should it cause any immediate execution of sigdeliver.
 *   Typically, a few cases need to be considered:
 *
 *   (1) This function may be called from an interrupt handler
 *       During interrupt processing, all xcptcontext structures
 *       should be valid for all tasks.  That structure should
 *       be modified to invoke sigdeliver() either on return
 *       from (this) interrupt or on some subsequent context
 *       switch to the recipient task.
 *   (2) If not in an interrupt handler and the tcb is NOT
 *       the currently executing task, then again just modify
 *       the saved xcptcontext structure for the recipient
 *       task so it will invoke sigdeliver when that task is
 *       later resumed.
 *   (3) If not in an interrupt handler and the tcb IS the
 *       currently executing task -- just call the signal
 *       handler now.
 *
 ************************************************************/

EXTERN void up_schedule_sigaction(_TCB *tcb, sig_deliver_t sigdeliver);

/************************************************************
 * Name: up_allocate_heap
 *
 * Description:
 *   The heap may be statically allocated by
 *   defining CONFIG_HEAP_BASE and CONFIG_HEAP_SIZE.  If these
 *   are not defined, then this function will be called to
 *   dynamically set aside the heap region.
 *
 ************************************************************/

#ifndef CONFIG_HEAP_BASE
EXTERN void up_allocate_heap(void **heap_start, size_t *heap_size);
#endif

/************************************************************
 * Name: up_interrupt_context
 *
 * Description:
 *   Return TRUE is we are currently executing in
 *   the interrupt handler context.
 *
 ************************************************************/

EXTERN boolean up_interrupt_context(void);

/************************************************************
 * Name: up_disable_irq
 *
 * Description:
 *   Disable the IRQ specified by 'irq'
 *
 ************************************************************/

EXTERN void up_disable_irq(int irq);

/************************************************************
 * Name: up_enable_irq
 *
 * Description:
 *   Enable the IRQ specified by 'irq'
 *
 ************************************************************/

EXTERN void up_enable_irq(int irq);

/************************************************************
 * Name: up_disable_irq
 *
 * Description:
 *   Disable the IRQ specified by 'irq'
 *
 ************************************************************/

EXTERN void up_disable_irq(int irq);

/************************************************************
 * These are standard interfaces that are exported by the OS
 * for use by the architecture specific logic
 ************************************************************/

/************************************************************
 * Name: sched_process_timer
 *
 * Description:
 *   This function handles system timer events.
 *   The timer interrupt logic itself is implemented in the
 *   architecture specific code, but must call the following OS
 *   function periodically -- the calling interval must be
 *   MSEC_PER_TICK.
 *
 ************************************************************/

EXTERN void sched_process_timer(void);

/************************************************************
 * Name: irq_dispatch
 *
 * Description:
 *   This function must be called from the achitecture-
 *   specific logic in order to dispaly an interrupt to
 *   the appropriate, registered handling logic.
 *
 ***********************************************************/

EXTERN void irq_dispatch(int irq, void *context);

/************************************************************
 * Debug interfaces exported by the architecture-specific
 * logic
 ************************************************************/

/************************************************************
 * Name: up_putc
 *
 * Description:
 *   Output one character on the console
 *
 ************************************************************/

# ifdef CONFIG_ARCH_LOWPUTC
EXTERN int up_putc(int ch);
#endif

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* __ARCH_H */

