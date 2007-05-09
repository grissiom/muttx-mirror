/************************************************************
 * dev_null.c
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
 * Compilation Switches
 ************************************************************/

/************************************************************
 * Included Files
 ************************************************************/

#include <nuttx/config.h>

#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <nuttx/fs.h>

/************************************************************
 * Private Function Prototypes
 ************************************************************/

static ssize_t devnull_read(struct file *, char *, size_t);
static ssize_t devnull_write(struct file *, const char *, size_t);

/************************************************************
 * Private Data
 ************************************************************/

static struct file_operations devnull_fops =
{
  0,             /* open */
  0,             /* close */
  devnull_read,  /* read */
  devnull_write, /* write */
  0,             /* seek */
  0              /* ioctl */
};

/************************************************************
 * Private Functions
 ************************************************************/

static ssize_t devnull_read(struct file *filp, char *buffer, size_t len)
{
  return 0; /* Return EOF */
}

static ssize_t devnull_write(struct file *filp, const char *buffer, size_t len)
{
  return len; /* Say that everything was written */
}

/************************************************************
 * Public Functions
 ************************************************************/

void devnull_register(void)
{
  (void)register_driver("/dev/null", &devnull_fops, 0666, NULL);
}
