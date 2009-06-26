/****************************************************************************
 * examples/nxflat/tests/mutex/mutex.c
 *
 *   Copyright (C) 2009 Gregory Nutt. All rights reserved.
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

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

/****************************************************************************
 * Private Data
 ****************************************************************************/

static pthread_mutex_t mut;
static volatile int my_mutex = 0;
static unsigned long nloops[2] = {0, 0};
static unsigned long nerrors[2] = {0, 0};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void thread_func(void *parameter)
{
  int my_id = (int)parameter;
  int my_ndx = my_id - 1;
  volatile int i;
    
  for (;;)
    {
      if ((pthread_mutex_lock(&mut)) != 0)
	{
	  printf("ERROR thread %d: pthread_mutex_lock failed\n", my_id);
	}

      if (my_mutex == 1)
	{
	  printf("ERROR thread=%d: "
		 "my_mutex should be zero, instead my_mutex=%d\n",
		 my_id, my_mutex);
	  nerrors[my_ndx]++;
	}

      my_mutex = 1;	
      for (i = 0; i < 1000; i++);
      my_mutex = 0;
	
      if ((pthread_mutex_unlock(&mut)) != 0)
	{
	  printf("ERROR thread %d: pthread_mutex_unlock failed\n", my_id);
	}

      nloops[my_ndx]++;
    }	
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, char **argv)
{
  pthread_t thread1, thread2;

  printf("Starting threads\n");

  /* Initialize the mutex */

  pthread_mutex_init(&mut, NULL);

  /* Start two thread instances */

  if ((pthread_create(&thread1, NULL, (void*)&thread_func, (void*)1)) != 0)
    {
      fprintf(stderr, "Error in thread#1 creation\n");
    }

  if ((pthread_create(&thread2, NULL, (void*)&thread_func, (void*)2)) != 0)
    {
      fprintf(stderr, "Error in thread#2 creation\n");
    }

  printf("Press control-C to terminate the example\n");

  pthread_join(thread1, NULL);
  pthread_join(thread2, NULL);
  return 0;
}

