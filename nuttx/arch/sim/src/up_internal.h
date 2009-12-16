/**************************************************************************
 * up_internal.h
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

#ifndef __ARCH_UP_INTERNAL_H
#define __ARCH_UP_INTERNAL_H

/**************************************************************************
 * Included Files
 **************************************************************************/

#include <sys/types.h>
#include <nuttx/irq.h>

/**************************************************************************
 * Public Definitions
 **************************************************************************/

/* Storage order: %ebx, $esi, %edi, %ebp, sp, and return PC */
#ifdef __ASSEMBLY__
# define JB_EBX (0*4)
# define JB_ESI (1*4)
# define JB_EDI (2*4)
# define JB_EBP (3*4)
# define JB_SP  (4*4)
# define JB_PC  (5*4)
#else
# define JB_EBX (0)
# define JB_ESI (1)
# define JB_EDI (2)
# define JB_EBP (3)
# define JB_SP  (4)
# define JB_PC  (5)
#endif /* __ASSEMBLY__ */

#define SIM_HEAP_SIZE (4*1024*1024)

/* These definitions characterize the compressed filesystem image */

#define BLOCK_COUNT         1024
#define SECTOR_OF_BACKUPT   6
#define NUMBER_OF_FATS      2
#define FAT_SIZE            32
#define NUM_HIDDEN_SECTORS  0
#define VOLUME_NAME         "NuttXTestVol"
#define USE_WHOLE_DEVICE    1
#define ROOT_DIR_ENTRIES    512
#define RESERVED_SECTORS    32
#define SECTORS_PER_CLUSTER 4
#define LOGICAL_SECTOR_SIZE 512

/**************************************************************************
 * Public Types
 **************************************************************************/

/**************************************************************************
 * Public Variables
 **************************************************************************/

/**************************************************************************
 * Public Function Prototypes
 **************************************************************************/

#ifndef __ASSEMBLY__

/* up_setjmp.S ************************************************************/

extern int  up_setjmp(int *jb);
extern void up_longjmp(int *jb, int val) __attribute__ ((noreturn));

/* up_devconsole.c ********************************************************/

extern void up_devconsole(void);
extern void up_registerblockdevice(void);

/* up_deviceimage.c *******************************************************/

extern char *up_deviceimage(void);

/* up_stdio.c *************************************************************/

extern size_t up_hostread(void *buffer, size_t len);
extern size_t up_hostwrite(const void *buffer, size_t len);

/* up_tapdev.c ************************************************************/

#ifdef CONFIG_NET 
extern unsigned long up_getwalltime( void );
extern void tapdev_init(void);
extern int tapdev_getmacaddr(unsigned char *macaddr);
extern unsigned int tapdev_read(unsigned char *buf, unsigned int buflen);
extern void tapdev_send(unsigned char *buf, unsigned int buflen);
#endif

/* up_uipdriver.c *********************************************************/

#if defined(CONFIG_NET) && defined(linux)
extern int uipdriver_init(void);
extern void uipdriver_loop(void);
#endif

#endif /* __ASSEMBLY__ */
#endif /* __ARCH_UP_INTERNAL_H */
