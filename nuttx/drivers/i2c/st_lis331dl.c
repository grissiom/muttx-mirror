/****************************************************************************
 * drivers/i2c/st_lis331dl.c
 *
 *   Copyright (C) 2011 Uros Platise. All rights reserved.
 *
 *   Authors: Uros Platise <uros.platise@isotel.eu>
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

/** \file
 *  \author Uros Platise
 *  \brief ST LIS331DL I2C Device Driver
 **/ 
 
#include <nuttx/config.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

#include <nuttx/i2c/st_lis331dl.h>
 
/************************************************************************************
 * Private Data Types
 ************************************************************************************/

struct st_lis331dl_dev_s {
    struct i2c_dev_s *          i2c;
    
    uint8_t                     address;
    struct st_lis331dl_vector_s a;
    uint8_t                     cr1;
    uint8_t                     cr2;    
    uint8_t                     cr3;
};

 
/****************************************************************************
 * Private Functions
 ****************************************************************************/

/** LIS331DL Access with range check
 * 
 * \param dev LIS331 DL Private Structure
 * \param subaddr LIS331 Sub Address
 * \param buf Pointer to buffer, either for read or write access
 * \param length when >0 it denotes read access, when <0 it denotes write access of -length
 * \return OK on success or errno is set.
 **/
int st_lis331dl_access(struct st_lis331dl_dev_s * dev, uint8_t subaddr, uint8_t *buf, int length)
{
    uint16_t flags = 0;
    int      retval;
    
    if (length > 0) {
        flags = I2C_M_READ; 
    }
    else {
        flags = I2C_M_NORESTART;
        length = -length;
    }

    /* Check valid address ranges and set auto address increment flag */
    
    if (subaddr == 0x0F) {
        if (length > 1) length = 1;
    }
    else if (subaddr >= 0x20 && subaddr < 0x24) {
        if (length > (0x24 - subaddr) ) length = 0x24 - subaddr;
    }
    else if (subaddr >= 0x27 && subaddr < 0x2E) {
        if (length > (0x2E - subaddr) ) length = 0x2E - subaddr;
    }
    else if (subaddr >= 0x30 && subaddr < 0x40) {
        if (length > (0x40 - subaddr) ) length = 0x40 - subaddr;
    }
    else {
        errno = EFAULT;
        return ERROR;
    }

    subaddr |= 0x80;
    
    /* Create message and send */
    
    struct i2c_msg_s msgv[2] = {
      {
        .addr   = dev->address,
        .flags  = 0,
        .buffer = &subaddr,
        .length = 1
      },
      {
        .addr   = dev->address,
        .flags  = flags,
        .buffer = buf,
        .length = length
      }
    };
    
    if ( (retval = I2C_TRANSFER(dev->i2c, msgv, 2)) == OK )
        return length;
        
    return retval;
}


int st_lis331dl_readregs(struct st_lis331dl_dev_s * dev)
{
    if (st_lis331dl_access(dev, ST_LIS331DL_CTRL_REG1, &dev->cr1, 3) == ERROR) return ERROR;    
    
    printf("CR1=%2x, CR2=%2x, CR3=%2x\n", dev->cr1, dev->cr2, dev->cr3 );
    return OK;
}


/****************************************************************************
 * Public Functions
 ****************************************************************************/

struct st_lis331dl_dev_s * st_lis331dl_init(struct i2c_dev_s * i2c, uint16_t address)
{
    struct st_lis331dl_dev_s * dev;
    uint8_t retval;
    
    ASSERT(i2c);
    ASSERT(address);
    
    if ( (dev = malloc( sizeof(struct st_lis331dl_dev_s) )) == NULL )
        return NULL;
        
    memset(dev, 0, sizeof(struct st_lis331dl_dev_s));
    dev->i2c     = i2c;
    dev->address = address;
    
    /* Probe device */
    
    if (st_lis331dl_access(dev, ST_LIS331DL_WHOAMI, &retval, 1) > 0) {
    
        /* Check chip identification, in the future several more compatible parts
         * may be added here.
         */
         
        if (retval == ST_LIS331DL_WHOAMI_VALUE) {
        
            /* Copy LIS331DL registers to our private structure and power-up device */
            
            if ( st_lis331dl_readregs(dev)==OK && st_lis331dl_powerup(dev)==OK) {
      
                /* Normal exit point */
                errno = 0;
                return dev;
            }
            retval = errno;
        }
        
        /* Otherwise, we mark an invalid device found at given address */
        retval = ENODEV;
    }
    else {
        /* No response at given address is marked as */
        retval = EFAULT;
    }

    /* Error exit */
    free(dev);
    errno = retval;
    return NULL;
}


int st_lis331dl_deinit(struct st_lis331dl_dev_s * dev)
{
    ASSERT(dev);
    
//  st_lis331dl_powerdown(dev);
    free(dev);
    
    return OK;
}


int st_lis331dl_powerup(struct st_lis331dl_dev_s * dev)
{
    dev->cr1 = ST_LIS331DL_CR1_PD |
        ST_LIS331DL_CR1_ZEN | ST_LIS331DL_CR1_YEN | ST_LIS331DL_CR1_XEN;
        
    st_lis331dl_access(dev, ST_LIS331DL_CTRL_REG1, &dev->cr1, -1);    
    return OK;
}


int st_lis331dl_powerdown(struct st_lis331dl_dev_s * dev)
{
    dev->cr1 = ST_LIS331DL_CR1_ZEN | ST_LIS331DL_CR1_YEN | ST_LIS331DL_CR1_XEN;
        
    return st_lis331dl_access(dev, ST_LIS331DL_CTRL_REG1, &dev->cr1, -1);
}


int st_lis331dl_setconversion(struct st_lis331dl_dev_s * dev, bool full, bool fast)
{
    dev->cr1 = ST_LIS331DL_CR1_PD | 
        (full ? ST_LIS331DL_CR1_FS : 0) | (fast ? ST_LIS331DL_CR1_DR : 0) |
        ST_LIS331DL_CR1_ZEN | ST_LIS331DL_CR1_YEN | ST_LIS331DL_CR1_XEN;
        
    st_lis331dl_access(dev, ST_LIS331DL_CTRL_REG1, &dev->cr1, -1);    
    return OK;
}


float st_lis331dl_getprecision(struct st_lis331dl_dev_s * dev)
{
    if (dev->cr1 & ST_LIS331DL_CR1_FS)
        return 9.0/127.0;   /* ~9g full scale */
    return 2.0/127.0;       /* ~2g full scale */
}


int st_lis331dl_getsamplerate(struct st_lis331dl_dev_s * dev)
{   
    if (dev->cr1 & ST_LIS331DL_CR1_DR)
        return 400;
    return 100;
}


const struct st_lis331dl_vector_s * st_lis331dl_getreadings(struct st_lis331dl_dev_s * dev)
{
    uint8_t retval[5];

    ASSERT(dev);
    
    if (st_lis331dl_access(dev, ST_LIS331DL_OUT_X, retval, 5) == 5) {    
        dev->a.x = retval[0];
        dev->a.y = retval[2];
        dev->a.z = retval[4];
        return &dev->a;
    }
    
    return NULL;
}
