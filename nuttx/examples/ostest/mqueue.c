/**************************************************************************
 * mqueue.c
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
 **************************************************************************/

/**************************************************************************
 * Included Files
 **************************************************************************/

#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <mqueue.h>
#include <sched.h>

#include "ostest.h"

/**************************************************************************
 * Private Definitions
 **************************************************************************/

#define TEST_MESSAGE "This is a test and only a test"
#define TEST_MSGLEN  (strlen(TEST_MESSAGE)+1)

/**************************************************************************
 * Private Types
 **************************************************************************/

/**************************************************************************
 * Private Function Prototypes
 **************************************************************************/

/**************************************************************************
 * Global Variables
 **************************************************************************/

/**************************************************************************
 * Private Variables
 **************************************************************************/

/**************************************************************************
 * Private Functions
 **************************************************************************/

/**************************************************************************
 * Public Functions
 **************************************************************************/

static void *sender_thread(void *arg)
{
  mqd_t mqfd;
  char msg_buffer[TEST_MSGLEN];
  struct mq_attr attr;
  int status = 0;
  int nerrors = 0;
  int i;

  printf("sender_thread: Starting\n");

  /* Fill in attributes for message queue */

  attr.mq_maxmsg  = 20;
  attr.mq_msgsize = TEST_MSGLEN;
  attr.mq_flags   = 0;

  /* Set the flags for the open of the queue.
   * Make it a blocking open on the queue, meaning it will block if
   * this process tries to send to the queue and the queue is full.
   *
   *   O_CREAT - the queue will get created if it does not already exist.
   *   O_WRONLY - we are only planning to write to the queue.
   *
   * Open the queue, and create it if the receiving process hasn't
   * already created it.
   */

  mqfd = mq_open("testmq", O_WRONLY|O_CREAT, 0666, &attr);
  if (mqfd < 0)
    {
	printf("sender_thread: ERROR mq_open failed\n");
        pthread_exit((void*)1);
    }

  /* Fill in a test message buffer to send */

  memcpy(msg_buffer, TEST_MESSAGE, TEST_MSGLEN);

  /* Perform the send 10 times */

  for (i = 0; i < 10; i++)
    {
      status = mq_send(mqfd, msg_buffer, TEST_MSGLEN, 42);
      if (status < 0)
        {
          printf("sender_thread: ERROR mq_send failure=%d on msg %d\n", status, i);
          nerrors++;
        }
      else
        {
          printf("sender_thread: mq_send succeeded on msg %d\n", i);
        }
    }

  /* Close the queue and return success */

  if (mq_close(mqfd) < 0)
    {
      printf("sender_thread: ERROR mq_close failed\n");
    }

  printf("sender_thread: returning nerrors=%d\n", nerrors);
  return (void*)nerrors;
}

static void *receiver_thread(void *arg)
{
  mqd_t mqfd;
  char msg_buffer[TEST_MSGLEN];
  struct mq_attr attr;
  int nbytes;
  int nerrors = 0;
  int i;

  printf("receiver_thread: Starting\n");

  /* Fill in attributes for message queue */

  attr.mq_maxmsg  = 20;
  attr.mq_msgsize = TEST_MSGLEN;
  attr.mq_flags   = 0;

  /* Set the flags for the open of the queue.
   * Make it a blocking open on the queue, meaning it will block if
   * this process tries to* send to the queue and the queue is full.
   *
   *   O_CREAT - the queue will get created if it does not already exist.
   *   O_RDONLY - we are only planning to write to the queue.
   *
   * Open the queue, and create it if the sending process hasn't
   * already created it.
   */

   mqfd = mq_open("testmq", O_RDONLY|O_CREAT, 0666, &attr);
   if (mqfd < 0)
     {
       printf("receiver_thread: ERROR mq_open failed\n");
       pthread_exit((void*)1);
     }

   /* Perform the receive 10 times */

   for (i = 0; i < 10; i++)
    {
      nbytes = mq_receive(mqfd, msg_buffer, TEST_MSGLEN, 0);
      if (nbytes < 0)
        {
          printf("receiver_thread: ERROR mq_receive failure on msg %d\n", i);
          nerrors++;
        }
      else if (nbytes != TEST_MSGLEN)
        {
          printf("receiver_thread: mq_receive return bad size %d on msg %d\n", nbytes, i);
          nerrors++;
        }
      else if (memcmp(TEST_MESSAGE, msg_buffer, nbytes) != 0)
        {
          int j;

          printf("receiver_thread: mq_receive returned corrupt message on msg %d\n", i);
          printf("receiver_thread:                  i  Expected Received\n");

          for (j = 0; j < TEST_MSGLEN-1; j++)
            {
              printf("receiver_thread:                  %2d %02x (%c) %02x\n",
                     j, TEST_MESSAGE[j], TEST_MESSAGE[j], msg_buffer[j] & 0x0ff);
            }
          printf("receiver_thread:                  %2d 00     %02x\n",
                  j, msg_buffer[j] & 0xff);
        }
      else
        {
          printf("receiver_thread: mq_receive succeeded on msg %d\n", i);
        }
    }

  /* Close the queue and return success */

  if (mq_close(mqfd) < 0)
    {
      printf("receiver_thread: ERROR mq_close failed\n");
      nerrors++;
    }

  pthread_exit((void*)nerrors);

  /* Destroy the queue */

  if (mq_unlink("testmq") < 0)
    {
      printf("receiver_thread: ERROR mq_close failed\n");
      nerrors++;
    }

  printf("receiver_thread: returning nerrors=%d\n", nerrors);
  return (void*)nerrors;
}

void mqueue_test(void)
{
  pthread_t sender;
  pthread_t receiver;
  void *result;
  pthread_attr_t attr;
  struct sched_param sparam;
  int prio_min;
  int prio_max;
  int prio_mid;
  int status;

  /* Start the sending thread at higher priority */

  printf("mqueue_test: Starting receiver\n");
  status = pthread_attr_init(&attr);
  if (status != 0)
    {
      printf("mqueue_test: pthread_attr_init failed, status=%d\n", status);
    }

  status = pthread_attr_setstacksize(&attr, 16384);
  if (status != 0)
    {
      printf("mqueue_test: pthread_attr_setstacksize failed, status=%d\n", status);
    }

  prio_min = sched_get_priority_min(SCHED_FIFO);
  prio_max = sched_get_priority_max(SCHED_FIFO);
  prio_mid = (prio_min + prio_max) / 2;

  sparam.sched_priority = prio_mid;
  status = pthread_attr_setschedparam(&attr,&sparam);
  if (status != OK)
    {
      printf("mqueue_test: pthread_attr_setschedparam failed, status=%d\n", status);
    }
  else
    {
      printf("mqueue_test: Set receiver priority to %d\n", sparam.sched_priority);
    }

  status = pthread_create(&receiver, &attr, receiver_thread, NULL);
  if (status != 0)
    {
      printf("mqueue_test: pthread_create failed, status=%d\n", status);
    }

  /* Start the sending thread at lower priority */

  printf("mqueue_test: Starting sender\n");
  status = pthread_attr_init(&attr);
  if (status != 0)
    {
      printf("mqueue_test: pthread_attr_init failed, status=%d\n", status);
    }

  status = pthread_attr_setstacksize(&attr, 16384);
  if (status != 0)
    {
      printf("mqueue_test: pthread_attr_setstacksize failed, status=%d\n", status);
    }

  sparam.sched_priority = (prio_min + prio_mid) / 2;
  status = pthread_attr_setschedparam(&attr,&sparam);
  if (status != OK)
    {
      printf("mqueue_test: pthread_attr_setschedparam failed, status=%d\n", status);
    }
  else
    {
      printf("mqueue_test: Set sender thread priority to %d\n", sparam.sched_priority);
    }

  status = pthread_create(&sender, &attr, sender_thread, NULL);
  if (status != 0)
    {
      printf("mqueue_test: pthread_create failed, status=%d\n", status);
    }

  pthread_join(sender, &result);
  if (result != (void*)0)
    {
      printf("mqueue_test: ERROR sender thread exited with %d errors\n", (int)result);
    }

  pthread_cancel(receiver);
  pthread_join(receiver, &result);
  if (result != (void*)0)
    {
      printf("mqueue_test: ERROR receiver thread exited with %d errors\n", (int)result);
    }
}


