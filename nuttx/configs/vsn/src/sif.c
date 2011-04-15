/****************************************************************************
 * config/vsn/src/sif.c
 * arch/arm/src/board/sif.c
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
 *  \brief VSN Sensor Interface
 * 
 * Public interface:
 *  - sif_init(): should be called just once after system starts, to 
 *    initialize internal data structures, device driver and hardware
 *  - individual starts() and stops() that control gpio, usart, i2c, ...
 *    are wrapped throu open() and close()
 *  - read() and write() are used for streaming
 *  - ioctl() for configuration    
 * 
 * STDOUT Coding 16-bit (little endian):
 *  - MSB = 0 GPIOs, followed by the both GPIO config bytes
 *  - MSB = 1 Input AD, centered around 0x4000
 * 
 * STDIN Coding 16-bit (little endian):
 *  - MSB = 0 GPIOs, followed by the both GPIO config bytes
 *    - MSB-1 = 0 Analog Output (PWM or Power)
 *    - MSB-1 = 1 Analog Reference Tap
 * 
 * GPIO Update cycle:
 *  - if they follow the Analog Output, they are synced with them
 *  - if they follow the Analog Reference Tap, they are synced with them
 *  - if either is configured without sample rate value, they are updated
 *    immediately, same as them
 * 
 * Implementation:
 *  - Complete internal states and updateing is made via the struct 
 *    vsn_sif_s, which is also accessible thru the ioctl() with 
 *    SNP Message descriptor.
 **/

#include <nuttx/config.h>
#include <nuttx/fs.h>
#include <semaphore.h>
#include <nuttx/clock.h>
#include <nuttx/time.h>
#include <nuttx/rtc.h>

#include <nuttx/i2c/i2c.h>
#include <nuttx/i2c/st_lis331dl.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "vsn.h"



/****************************************************************************
 * Declarations and Structures
 ****************************************************************************/ 
 
#define VSN_SIF_READ_BUFSIZE    128
#define VSN_SIF_WRITE_BUFSIZE   128


typedef unsigned char vsn_sif_state_t;

#   define VSN_SIF_STATE_POWERDOWN  0x00    ///< power-down

#   define VSN_SIF_STATE_ACT_GPIO   0x01    ///< gpio is active
#   define VSN_SIF_STATE_ACT_USART  0x02    ///< usart is active
#   define VSN_SIF_STATE_ACT_I2C    0x04    ///< i2c is active
#   define VSN_SIF_STATE_ACT_OWIR1  0x08    ///< 1-wire is active on first GPIO
#   define VSN_SIF_STATE_ACT_OWIR2  0x10    ///< 1-wire is active on second GPIO

#   define VSN_SIF_STATE_ACT_ANOUT  0x20    ///< analog output is active
#   define VSN_SIF_STATE_ACT_ANIN   0x40    ///< analog input is active


typedef unsigned char vsn_sif_gpio_t;

#   define VSN_SIF_GPIO_STATE_MASK  7
#   define VSN_SIF_GPIO_HIGHZ       0       ///< High-Z
#   define VSN_SIF_GPIO_PULLUP      1       ///< Pull-Up
#   define VSN_SIF_GPIO_PULLDOWN    2       ///< Pull-Down
#   define VSN_SIF_GPIO_OUTLOW      3       ///< Set Low
#   define VSN_SIF_GPIO_OUTHIGH     4       ///< Set High

#   define VSN_SIF_GPIO_DISALT_MASK 0x10    ///< Disable Alternate Function, Mask Bit
#   define VSN_SIF_GPIO_TRIG_MASK   0x20    ///< Send data change to the stdout
#   define VSN_SIF_GPIO_READ_MASK   0x40    ///< Readout mask


#define VSN_SIF_ANOUT_LOW       0	// Pseudo Analog Output acts as GPIO
#define VSN_SIF_ANOUT_HIGH      1	// Pseudo Analog Output acts as GPIO high
#define	VSN_SIF_ANOUT_HIGHPWR   2	// ... acts as high power output
#define VSN_SIF_ANOUT_PWM       3	// ... acts as PWM output 
#define VSN_SIF_ANOUT_PWMPWR    4	// acts as power PWM output

#define VSN_SIF_ANIN_GAINMASK   7
#define VSN_SIF_ANIN_GAIN1      0
#define VSN_SIF_ANIN_GAIN2      1
#define VSN_SIF_ANIN_GAIN4      2
#define VSN_SIF_ANIN_GAIN8      3
#define VSN_SIF_ANIN_GAIN16     4
#define VSN_SIF_ANIN_GAIN32     5
#define VSN_SIF_ANIN_GAIN64     6
#define VSN_SIF_ANIN_GAIN128    7

#define VSN_SIF_ANIN_BITS8
#define VSN_SIF_ANIN_BITS9
#define VSN_SIF_ANIN_BITS10
#define VSN_SIF_ANIN_BITS11
#define VSN_SIF_ANIN_BITS12
#define VSN_SIF_ANIN_BITS13
#define VSN_SIF_ANIN_BITS14

#define VSN_SIF_ANIN_OVERSMP1
#define VSN_SIF_ANIN_OVERSMP2
#define VSN_SIF_ANIN_OVERSMP4
#define VSN_SIF_ANIN_OVERSMP8	
#define VSN_SIF_ANIN_OVERSMP16


struct vsn_sif_s {
    vsn_sif_state_t     state;              // activity 
    unsigned char       opencnt;            // open count
	
    vsn_sif_gpio_t      gpio[2];

    unsigned char       anout_opts;
    unsigned short int  anout_width;
    unsigned short int  anout_period;       // setting it to 0, disables PWM
    unsigned short int  anout_samplerate;   // as written by write()
	 		
    unsigned short int  anref_width;
    unsigned short int  anref_period;       // setting it to 0, disables PWM
    unsigned short int  anref_samplerate;   // as written by write()
    
    unsigned char       anin_opts;
    unsigned int        anin_samplerate;    // returned on read() as 16-bit results
    
        /*--- Private Data ---*/
    
    struct stm32_tim_dev_s * tim3;          // Timer3 is used for PWM, and Analog RefTap
    struct stm32_tim_dev_s * tim8;          // Timer8 is used for Power Switch
    
    struct i2c_dev_s    * i2c1;
    struct i2c_dev_s    * i2c2;
    
    sem_t               exclusive_access;
};


/****************************************************************************
 * Private data
 ****************************************************************************/ 

struct vsn_sif_s vsn_sif;


/****************************************************************************
 * Semaphores
 ****************************************************************************/ 

void sif_sem_wait(void)
{
    while( sem_wait( &vsn_sif.exclusive_access ) != 0 ) {
        ASSERT(errno == EINTR);
    }
}


void inline sif_sem_post(void)
{
    sem_post( &vsn_sif.exclusive_access );
}


/****************************************************************************
 * GPIOs and Alternative Functions
 ****************************************************************************/ 


void sif_gpios_reset(void)
{
    vsn_sif.gpio[0] = vsn_sif.gpio[1] = VSN_SIF_GPIO_HIGHZ;
    
    stm32_configgpio(GPIO_GP1_HIZ);
    stm32_configgpio(GPIO_GP2_HIZ);
}


void sif_gpio1_update(void)
{
    uint32_t val;
    
    switch(vsn_sif.gpio[0] & VSN_SIF_GPIO_STATE_MASK) {
        case VSN_SIF_GPIO_HIGHZ:    val = GPIO_GP1_HIZ; break;
        case VSN_SIF_GPIO_PULLUP:   val = GPIO_GP1_PUP; break;
        case VSN_SIF_GPIO_PULLDOWN: val = GPIO_GP1_PDN; break;
        case VSN_SIF_GPIO_OUTLOW:   val = GPIO_GP1_LOW; break;
        case VSN_SIF_GPIO_OUTHIGH:  val = GPIO_GP1_HIGH;break;
        default: return;
    }
    if (stm32_configgpio(val) == ERROR)
        printf("Error updating1\n");
    
    if ( stm32_gpioread(val) ) 
        vsn_sif.gpio[0] |= VSN_SIF_GPIO_READ_MASK;
    else vsn_sif.gpio[0] &= ~VSN_SIF_GPIO_READ_MASK;
}


void sif_gpio2_update(void)
{
    uint32_t val;
    
    switch(vsn_sif.gpio[1]) {
        case VSN_SIF_GPIO_HIGHZ:    val = GPIO_GP2_HIZ; break;
        case VSN_SIF_GPIO_PULLUP:   val = GPIO_GP2_PUP; break;
        case VSN_SIF_GPIO_PULLDOWN: val = GPIO_GP2_PDN; break;
        case VSN_SIF_GPIO_OUTLOW:   val = GPIO_GP2_LOW; break;
        case VSN_SIF_GPIO_OUTHIGH:  val = GPIO_GP2_HIGH;break;
        default: return;
    }
    if (stm32_configgpio(val) == ERROR)
        printf("Error updating2\n");
    
    if ( stm32_gpioread(val) ) 
        vsn_sif.gpio[1] |= VSN_SIF_GPIO_READ_MASK;
    else vsn_sif.gpio[1] &= ~VSN_SIF_GPIO_READ_MASK;
}


int sif_gpios_lock(vsn_sif_state_t peripheral)
{
    return ERROR;
}


int sif_gpios_unlock(vsn_sif_state_t peripheral)
{
    return ERROR;
}




/****************************************************************************
 * Analog Outputs
 ****************************************************************************/ 
 
static volatile int test = 0, test_irq;


static int sif_anout_isr(int irq, void *context)
{
    STM32_TIM_ACKINT(vsn_sif.tim8, 0);

    test++;
    test_irq = irq;
    
    return OK;
}


int sif_anout_init(void)
{
    vsn_sif.tim3 = stm32_tim_init(3);
    vsn_sif.tim8 = stm32_tim_init(8);
        
    if (!vsn_sif.tim3 || !vsn_sif.tim8) return ERROR;
    
    // Use the TIM3 as PWM modulated analogue output
    
    STM32_TIM_SETPERIOD(vsn_sif.tim3, 4096);
    STM32_TIM_SETCOMPARE(vsn_sif.tim3, GPIO_OUT_PWM_TIM3_CH, 1024);

    STM32_TIM_SETCLOCK(vsn_sif.tim3, 36e6);
    STM32_TIM_SETMODE(vsn_sif.tim3, STM32_TIM_MODE_UP);
    //STM32_TIM_SETCHANNEL(vsn_sif.tim3, GPIO_OUT_PWM_TIM3_CH, STM32_TIM_CH_OUTPWM | STM32_TIM_CH_POLARITY_NEG);
    
    // Use the TIM8 to drive the upper power mosfet
    
    STM32_TIM_SETISR(vsn_sif.tim8, sif_anout_isr, 0);
    STM32_TIM_ENABLEINT(vsn_sif.tim8, 0);
    
    STM32_TIM_SETPERIOD(vsn_sif.tim8, 4096);
    STM32_TIM_SETCOMPARE(vsn_sif.tim8, GPIO_OUT_PWRPWM_TIM8_CH, 5000);
    
    STM32_TIM_SETCLOCK(vsn_sif.tim8, 36e6);
    STM32_TIM_SETMODE(vsn_sif.tim8, STM32_TIM_MODE_UP);
    STM32_TIM_SETCHANNEL(vsn_sif.tim8, GPIO_OUT_PWRPWM_TIM8_CH, STM32_TIM_CH_OUTPWM | STM32_TIM_CH_POLARITY_NEG);

    vsn_sif.i2c1 = up_i2cinitialize(1);
    vsn_sif.i2c2 = up_i2cinitialize(2);

    return OK;
}


void sif_anout_update(void)
{
}


void sif_anout_callback(void)
{
	// called at rate of PWM interrupt
}


/****************************************************************************
 * Analog Input Reference Tap
 ****************************************************************************/ 


void sif_anref_init(void)
{
}


/****************************************************************************
 * Analog Input Sampler Unit
 ****************************************************************************/ 


void sif_anin_reset(void)
{
}


/****************************************************************************
 * Device driver functions
 ****************************************************************************/ 

int devsif_open(FAR struct file *filep)
{
    sif_sem_wait();
    vsn_sif.opencnt++;
    
    // Start Hardware
        
    sif_sem_post();
    return 0;
}


int devsif_close(FAR struct file *filep)
{
    sif_sem_wait();
    
    if (--vsn_sif.opencnt) {
    
        // suspend (powerdown) hardware
        
        sif_gpios_reset();
        
        //STM32_TIM_SETCLOCK(vsn_sif.tim3, 0);
        //STM32_TIM_SETCLOCK(vsn_sif.tim8, 0);
    }
    
    sif_sem_post();
    return 0;
}


static ssize_t devsif_read(FAR struct file *filp, FAR char *buffer, size_t len)
{
    sif_sem_wait();
    memset(buffer, 0, len);
    sif_sem_post();
    return len;
}


static ssize_t devsif_write(FAR struct file *filp, FAR const char *buffer, size_t len)
{
    sif_sem_wait();
    printf("getpid: %d\n", getpid() );
    sif_sem_post();
    return len;
}


#ifndef CONFIG_DISABLE_POLL
static int devsif_poll(FAR struct file *filp, FAR struct pollfd *fds,
                        bool setup)
{
    if (setup) {
        fds->revents |= (fds->events & (POLLIN|POLLOUT));
        
        if (fds->revents != 0) {
            sem_post(fds->sem);
        }
    }
    return OK;
}
#endif


int devsif_ioctl(FAR struct file *filep, int cmd, unsigned long arg)
{
    sif_sem_wait();
    sif_sem_post();
    return 0;
}


/****************************************************************************
 * Public Functions
 ****************************************************************************/ 


static const struct file_operations devsif_fops = {
    devsif_open,    /* open */
    devsif_close,   /* close */
    devsif_read,    /* read */
    devsif_write,   /* write */
    0,              /* seek */
    devsif_ioctl    /* ioctl */
#ifndef CONFIG_DISABLE_POLL
  , devsif_poll     /* poll */
#endif
};

 
/** Bring up the Sensor Interface by initializing all of the desired 
 *  hardware components.
 **/
  
int sif_init(void)
{
    /* Initialize data-structure */
    
    vsn_sif.state   = VSN_SIF_STATE_POWERDOWN;
    vsn_sif.opencnt = 0;
    sem_init(&vsn_sif.exclusive_access, 0, 1);
    
    /* Initialize hardware */
    
    sif_gpios_reset();
    if ( sif_anout_init() != OK ) return -1;

    /* If everything is okay, register the driver */
    
    (void)register_driver("/dev/sif0", &devsif_fops, 0666, NULL);
    return OK;
}


/** SIF Utility
 * 
 * Provides direct access to the sensor connector, readings, and diagnostic.
 **/
 
int sif_main(int argc, char *argv[])
{
    if (argc >= 2) {	
        if (!strcmp(argv[1], "init")) {          
          return sif_init();
        }
        else if (!strcmp(argv[1], "gpio") && argc == 4) {
            vsn_sif.gpio[0] = atoi(argv[2]);
            vsn_sif.gpio[1] = atoi(argv[3]);
            sif_gpio1_update();
            sif_gpio2_update();
            printf("GPIO States: %2x %2x\n", vsn_sif.gpio[0], vsn_sif.gpio[1] );
            return 0;
        }
        else if (!strcmp(argv[1], "pwr") && argc == 3) {
            int val = atoi(argv[2]);
            STM32_TIM_SETCOMPARE(vsn_sif.tim8, GPIO_OUT_PWRPWM_TIM8_CH, val);
            return 0;
        }
        else if (!strcmp(argv[1], "time") && argc == 3) {
            int val = atoi(argv[2]);
            up_rtc_settime(val);
        }
        else if (!strcmp(argv[1], "i2c") && argc == 3) {
            int val = atoi(argv[2]);
            
            I2C_SETFREQUENCY(vsn_sif.i2c2, 100000);
            
            struct st_lis331dl_dev_s * lis = st_lis331dl_init(vsn_sif.i2c2, val);

            if (lis) {
                struct st_lis331dl_vector_s * a;
                int i;
                uint32_t time_stamp = clock_systimer();
                
                /* Set to 400 Hz : 3 = 133 Hz/axis */
                
                st_lis331dl_setconversion(lis, false, true);
                
                /* Sample some values */
                
                for (i=0; i<1000; ) {
                    if ( (a = st_lis331dl_getreadings(lis)) ) {
                        i++;
                        printf("%d %d %d\n", a->x, a->y, a->z);
                    }
                    else if (errno != 11) {
                        printf("Readings errno %d\n", errno);
                        break;
                    }
                }
                
                printf("Time diff = %d\n", clock_systimer() - time_stamp);
                
                st_lis331dl_deinit(lis);
            }
            else printf("Exit point: errno=%d\n", errno);

            return 0;
        }
    }

    fprintf(stderr, "%s:\tinit\n\tgpio\tA B\n\tpwr\tval\n", argv[0]);
    fprintf(stderr, "time = %d / %d, time = %d\n", 
        up_rtc_gettime(), up_rtc_getclock(), time(NULL) );
    return -1;
}
