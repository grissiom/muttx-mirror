/************************************************************************************
 * arch/arm/src/stm32/stm32_internal.h
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
 ************************************************************************************/

#ifndef __ARCH_ARM_SRC_STM32_STM32_INTERNAL_H
#define __ARCH_ARM_SRC_STM32_STM32_INTERNAL_H

/************************************************************************************
 * Included Files
 ************************************************************************************/

#include <nuttx/config.h>
#include <sys/types.h>

#include "up_internal.h"
#include "chip.h"

/************************************************************************************
 * Definitions
 ************************************************************************************/

/* NVIC priority levels */

#define NVIC_SYSH_PRIORITY_MIN     0xff /* All bits set in minimum priority */
#define NVIC_SYSH_PRIORITY_DEFAULT 0x80 /* Midpoint is the default */
#define NVIC_SYSH_PRIORITY_MAX     0x00 /* Zero is maximum priority */

/* Bit-encoded input to stm32_configgpio() *******************************************/

/* Encoding:
 * .... .... .... .... OFFS S... VPPP BBBB
 */

/* Output mode:
 *
 * .... .... .... .... O... .... VPPP BBBB
 */

#define GPIO_OUTPUT                   (1 << 15)                  /* Bit 15: Output mode */
#define GPIO_INPUT                    (0)
#define GPIO_ALTERNATE                (0)

/* These bits set the primary function of the pin:
 * .... .... .... .... FFF. .... .... ....
 */

#define GPIO_CNF_SHIFT                13                         /* Bits 13-14: GPIO function */
#define GPIO_CNF_MASK                 (3 << GPIO_CNF_SHIFT)

#  define GPIO_CNF_ANALOGIN           (0 << GPIO_CNF_SHIFT)      /* Analog input */
#  define GPIO_CNF_INFLOAT            (1 << GPIO_CNF_SHIFT)      /* Input floating */
#  define GPIO_CNF_INPULLUP           (2 << GPIO_CNF_SHIFT)      /* Input pull-up */
#  define GPIO_CNF_INPULLDWN          (3 << GPIO_CNF_SHIFT)      /* Input pull-down */

#  define GPIO_CNF_OUTPP              (0 << GPIO_CNF_SHIFT)      /* Output push-pull */
#  define GPIO_CNF_OUTOD              (1 << GPIO_CNF_SHIFT)      /* Output open-drain */
#  define GPIO_CNF_AFPP               (2 << GPIO_CNF_SHIFT)      /* Altnernate function push-pull */
#  define GPIO_CNF_AFOD               (3 << GPIO_CNF_SHIFT)      /* Altnernate function open-drain */

/* Maximum frequency selection:
 * .... .... .... .... ...S S... .... ....
 */

#define GPIO_MODE_SHIFT               11                         /* Bits 11-12: GPIO frequency selection */
#define GPIO_MODE_MASK                (3 << GPIO_MODE_SHIFT)
#  define GPIO_MODE_INPUT             (0 << GPIO_MODE_SHIFT)     /* Input mode (reset state) */
#  define GPIO_MODE_10MHz             (1 << GPIO_MODE_SHIFT)     /* Output mode, max speed 10 MHz */
#  define GPIO_MODE_2MHz              (2 << GPIO_MODE_SHIFT)     /* Output mode, max speed 2 MHz */
#  define GPIO_MODE_50MHz             (3 << GPIO_MODE_SHIFT)     /* Output mode, max speed 50 MHz */

/* If the pin is an GPIO digital output, then this identifies the initial output value:
 * .... .... .... .... .... .... V... ....
 */

#define GPIO_OUTPUT_SET               (1 << 7)                   /* Bit 7: If output, inital value of output */
#define GPIO_OUTPUT_CLEAR             (0) 

/* This identifies the GPIO port:
 * .... .... .... .... .... .... .PPP ....
 */

#define GPIO_PORT_SHIFT               4                          /* Bit 4-6:  Port number */
#define GPIO_PORT_MASK                (7 << GPIO_PORT_SHIFT)
#define GPIO_PORTA                    (0 << GPIO_PORT_SHIFT)     /*   GPIOA */
#define GPIO_PORTB                    (1 << GPIO_PORT_SHIFT)     /*   GPIOB */
#define GPIO_PORTC                    (2 << GPIO_PORT_SHIFT)     /*   GPIOC */
#define GPIO_PORTD                    (3 << GPIO_PORT_SHIFT)     /*   GPIOD */
#define GPIO_PORTE                    (4 << GPIO_PORT_SHIFT)     /*   GPIOE */
#define GPIO_PORTF                    (5 << GPIO_PORT_SHIFT)     /*   GPIOF */
#define GPIO_PORTG                    (6 << GPIO_PORT_SHIFT)     /*   GPIOG */

/* This identifies the bit in the port:
 * .... .... .... .... .... .... .... BBBB
 */

#define GPIO_PIN_SHIFT                 0                           /* Bits 0-3: GPIO number: 0-15 */
#define GPIO_PIN_MASK                  (15 << GPIO_PIN_SHIFT)
#define GPIO_PIN1                      (1 << GPIO_PIN_SHIFT)
#define GPIO_PIN2                      (2 << GPIO_PIN_SHIFT)
#define GPIO_PIN3                      (3 << GPIO_PIN_SHIFT)
#define GPIO_PIN4                      (4 << GPIO_PIN_SHIFT)
#define GPIO_PIN5                      (5 << GPIO_PIN_SHIFT)
#define GPIO_PIN6                      (6 << GPIO_PIN_SHIFT)
#define GPIO_PIN7                      (7 << GPIO_PIN_SHIFT)
#define GPIO_PIN8                      (8 << GPIO_PIN_SHIFT)
#define GPIO_PIN9                      (9 << GPIO_PIN_SHIFT)
#define GPIO_PIN10                     (10 << GPIO_PIN_SHIFT)
#define GPIO_PIN11                     (11 << GPIO_PIN_SHIFT)
#define GPIO_PIN12                     (12 << GPIO_PIN_SHIFT)
#define GPIO_PIN13                     (13 << GPIO_PIN_SHIFT)
#define GPIO_PIN14                     (14 << GPIO_PIN_SHIFT)
#define GPIO_PIN15                     (15 << GPIO_PIN_SHIFT)

/* Alternate Pin Functions */
/* SPI1 */

#define GPIO_SPI1_NSS  (GPIO_ALTERNATE|GPIO_CNF_AFPP|GPIO_MODE_50MHz|GPIO_PORTA|GPIO_PIN4)
#define GPIO_SPI1_SCK  (GPIO_ALTERNATE|GPIO_CNF_AFPP|GPIO_MODE_50MHz|GPIO_PORTA|GPIO_PIN5)
#define GPIO_SPI1_MISO (GPIO_ALTERNATE|GPIO_CNF_AFPP|GPIO_MODE_50MHz|GPIO_PORTA|GPIO_PIN6)
#define GPIO_SPI1_MOSI (GPIO_ALTERNATE|GPIO_CNF_AFPP|GPIO_MODE_50MHz|GPIO_PORTA|GPIO_PIN7)

/************************************************************************************
 * Public Types
 ************************************************************************************/

/************************************************************************************
 * Inline Functions
 ************************************************************************************/

#ifndef __ASSEMBLY__

/************************************************************************************
 * Public Data
 ************************************************************************************/

#undef EXTERN
#if defined(__cplusplus)
#define EXTERN extern "C"
extern "C" {
#else
#define EXTERN extern
#endif

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: stm32_lowsetup
 *
 * Description:
 *   Called at the very beginning of _start.  Performs low level initialization.
 *
 ****************************************************************************/

EXTERN void stm32_lowsetup(void);

/****************************************************************************
 * Name: stm32_clockconfig
 *
 * Description:
 *   Called to change to new clock based on settings in board.h
 *
 ****************************************************************************/

EXTERN void stm32_clockconfig(void);

/****************************************************************************
 * Name: stm32_configgpio
 *
 * Description:
 *   Configure a GPIO pin based on bit-encoded description of the pin.
 *
 ****************************************************************************/

EXTERN int stm32_configgpio(uint32 cfgset);

/****************************************************************************
 * Name: stm32_gpiowrite
 *
 * Description:
 *   Write one or zero to the selected GPIO pin
 *
 ****************************************************************************/

EXTERN void stm32_gpiowrite(uint32 pinset, boolean value);

/****************************************************************************
 * Name: stm32_gpioread
 *
 * Description:
 *   Read one or zero from the selected GPIO pin
 *
 ****************************************************************************/

EXTERN boolean stm32_gpioread(uint32 pinset);

/****************************************************************************
 * Function:  stm32_dumpgpio
 *
 * Description:
 *   Dump all GPIO registers associated with the provided base address
 *
 ****************************************************************************/

#ifdef CONFIG_DEBUG
EXTERN int stm32_dumpgpio(uint32 pinset, const char *msg);
#else
#  define stm32_dumpgpio(p,m)
#endif

/****************************************************************************
 * Name: gpio_irqinitialize
 *
 * Description:
 *   Initialize all vectors to the unexpected interrupt handler
 *
 ****************************************************************************/

EXTERN int weak_function gpio_irqinitialize(void);

/****************************************************************************
 * Function: stm32_ethinitialize
 *
 * Description:
 *   Initialize the Ethernet driver for one interface.  If the STM32 chip
 *   supports multiple Ethernet controllers, then bould specific logic
 *   must implement up_netinitialize() and call this function to initialize
 *   the desiresed interfaces.
 *
 * Parameters:
 *   None
 *
 * Returned Value:
 *   OK on success; Negated errno on failure.
 *
 * Assumptions:
 *
 ****************************************************************************/

#if STM32_NTHERNET > 1
EXTERN int stm32_ethinitialize(int intf);
#endif

/****************************************************************************
 * The external functions, stm32_spi1/2select and stm32_spi1/2status must be
 * provided by board-specific logic.  They are implementations of the select
 * and status methods of the SPI interface defined by struct spi_ops_s (see
 * include/nuttx/spi.h). All other methods (including up_spiinitialize())
 * are provided by common STM32 logic.  To use this common SPI logic on your
 * board:
 *
 *   1. Provide logic in stm32_boardinitialize() to configure SPI chip select
 *      pins.
 *   2. Provide stm32_spi1/2select() and stm32_spi1/2status() functions in your
 *      board-specific logic.  These functions will perform chip selection and
 *      status operations using GPIOs in the way your board is configured.
 *   3. Add a calls to up_spiinitialize() in your low level application
 *      initialization logic
 *   4. The handle returned by up_spiinitialize() may then be used to bind the
 *      SPI driver to higher level logic (e.g., calling 
 *      mmcsd_spislotinitialize(), for example, will bind the SPI driver to
 *      the SPI MMC/SD driver).
 *
 ****************************************************************************/

struct spi_dev_s;
enum spi_dev_e;
EXTERN void  stm32_spi1select(FAR struct spi_dev_s *dev, enum spi_dev_e devid, boolean selected);
EXTERN ubyte stm32_spi1status(FAR struct spi_dev_s *dev, enum spi_dev_e devid);
EXTERN void  stm32_spi2select(FAR struct spi_dev_s *dev, enum spi_dev_e devid, boolean selected);
EXTERN ubyte stm32_spi2status(FAR struct spi_dev_s *dev, enum spi_dev_e devid);

#undef EXTERN
#if defined(__cplusplus)
}
#endif

#endif /* __ASSEMBLY__ */
#endif /* __ARCH_ARM_SRC_STM32_STM32_INTERNAL_H */
