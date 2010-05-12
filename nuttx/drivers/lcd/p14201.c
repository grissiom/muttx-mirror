/**************************************************************************************
 * drivers/lcd/p14201.c
 * Driver for RiT P14201 series display (wih sd1329 IC controller)
 *
 *   Copyright (C) 2010 Gregory Nutt. All rights reserved.
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
 **************************************************************************************/

/**************************************************************************************
 * Included Files
 **************************************************************************************/

#include <nuttx/config.h>

#include <sys/types.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <debug.h>

#include <nuttx/arch.h>
#include <nuttx/spi.h>
#include <nuttx/lcd.h>

#include <arch/irq.h>

#include "sd1329.h"

/**************************************************************************************
 * Pre-processor Definitions
 **************************************************************************************/

/* Configuration **********************************************************************/

/* P14201 Configuration Settings:
 *
 * CONFIG_LCD_P14201 - Enable P14201 support
 * CONFIG_P14201_OWNBUS - Set if the P14201 is the only active device on the SPI bus.
 *   No locking or SPI configuration will be performed. All transfers will be performed
 *   from the ENC2J60 interrupt handler.
 * CONFIG_P14201_SPIMODE - Controls the SPI mode
 * CONFIG_P14201_FREQUENCY - Define to use a different bus frequency
 * CONFIG_P14201_NINTERFACES - Specifies the number of physical P14201 devices that
 *   will be supported.
 */

/* The P14201 spec says that is supports SPI mode 0,0 only.  However,
 * somtimes you need to tinker with these things.
 */

#ifndef CONFIG_P14201_SPIMODE
#  define CONFIG_P14201_SPIMODE SPIDEV_MODE2
#endif

/* CONFIG_P14201_NINTERFACES determines the number of physical interfaces
 * that will be supported.
 */

#ifndef CONFIG_P14201_NINTERFACES
#  define CONFIG_P14201_NINTERFACES 1
#endif

/* Check contrast selection */

#if !defined(CONFIG_LCD_MAXCONTRAST)
#  define CONFIG_LCD_MAXCONTRAST 255
#endif

#if CONFIG_LCD_MAXCONTRAST > 255
#  error "CONFIG_LCD_MAXCONTRAST exceeds supported maximum"
#endif

/* Check power setting */

#if !defined(CONFIG_LCD_MAXPOWER)
#  define CONFIG_LCD_MAXPOWER 1
#endif

#if CONFIG_LCD_MAXCONTRAST > 1
#  warning "CONFIG_LCD_MAXPOWER exceeds supported maximum"
#  undef CONFIG_LCD_MAXPOWER
#  define CONFIG_LCD_MAXPOWER 1
#endif

/* Define the following to enable register-level debug output */

#undef CONFIG_LCD_RITDEBUG

/* Verbose debug must also be enabled */

#ifndef CONFIG_DEBUG
#  undef CONFIG_DEBUG_VERBOSE
#  undef CONFIG_DEBUG_GRAPHICS
#endif

#ifndef CONFIG_DEBUG_VERBOSE
#  undef CONFIG_LCD_RITDEBUG
#endif

/* Color Properties *******************************************************************/

/* Display Resolution */

#define RIT_XRES         128
#define RIT_YRES         96

/* Color depth and format */

#define RIT_BPP          4
#define RIT_COLORFMT     FB_FMT_Y4

/* Default contrast */

#define RIT_CONTRAST    183  /* 183/255 */

/* Helper Macros **********************************************************************/

#define rit_sndcmd(s,b,l) rit_sndbytes(s,b,l,false);
#define rit_snddata(s,b,l) rit_sndbytes(s,b,l,true);


/* Debug ******************************************************************************/

#ifdef CONFIG_LCD_RITDEBUG
# define ritdbg(format, arg...)  vdbg(format, ##arg)
#else
# define ritdbg(x...)
#endif

/**************************************************************************************
 * Private Type Definition
 **************************************************************************************/

/* This structure describes the state of this driver */

struct rit_dev_s
{
  struct lcd_dev_s      dev;      /* Publically visible device structure */
  FAR struct spi_dev_s *spi;      /* Cached SPI device reference */
  uint8_t               contrast; /* Current contrast setting */
  bool                  on;       /* true: display is on */
};

/**************************************************************************************
 * Private Function Protototypes
 **************************************************************************************/

/* Low-level SPI helpers */

static inline void rit_configspi(FAR struct spi_dev_s *spi);
#ifdef CONFIG_P14201_OWNBUS
static inline void rit_select(FAR struct spi_dev_s *spi);
static inline void rit_deselect(FAR struct spi_dev_s *spi);
#else
static void rit_select(FAR struct spi_dev_s *spi);
static void rit_deselect(FAR struct spi_dev_s *spi);
#endif
static void rit_sndbytes(FAR struct spi_dev_s *spi, FAR const uint8_t *buffer,
              size_t buflen, bool data);
static void rit_sndcmds(FAR struct spi_dev_s *spi, FAR const uint8_t *table);

/* LCD Data Transfer Methods */

static int rit_putrun(fb_coord_t row, fb_coord_t col, FAR const uint8_t *buffer,
             size_t npixels);
static int rit_getrun(fb_coord_t row, fb_coord_t col, FAR uint8_t *buffer,
             size_t npixels);

/* LCD Configuration */

static int rit_getvideoinfo(FAR struct lcd_dev_s *dev,
             FAR struct fb_videoinfo_s *vinfo);
static int rit_getplaneinfo(FAR struct lcd_dev_s *dev, unsigned int planeno,
             FAR struct lcd_planeinfo_s *pinfo);

/* LCD RGB Mapping */

#ifdef CONFIG_FB_CMAP
#  error "RGB color mapping not supported by this driver"
#endif

/* Cursor Controls */

#ifdef CONFIG_FB_HWCURSOR
#  error "Cursor control not supported by this driver"
#endif

/* LCD Specific Controls */

static int rit_getpower(struct lcd_dev_s *dev);
static int rit_setpower(struct lcd_dev_s *dev, int power);
static int rit_getcontrast(struct lcd_dev_s *dev);
static int rit_setcontrast(struct lcd_dev_s *dev, unsigned int contrast);

/**************************************************************************************
 * Private Data
 **************************************************************************************/

/* This is working memory allocated by the LCD driver for each LCD device
 * and for each color plane.  This memory will hold one raster line of data.
 * The size of the allocated run buffer must therefore be at least
 * (bpp * xres / 8).  Actual alignment of the buffer must conform to the
 * bitwidth of the underlying pixel type.
 *
 * If there are multiple planes, they may share the same working buffer
 * because different planes will not be operate on concurrently.  However,
 * if there are multiple LCD devices, they must each have unique run buffers.
 */

static uint8_t g_runbuffer[RIT_XRES/2];

/* This structure describes the overall LCD video controller */

static const struct fb_videoinfo_s g_videoinfo =
{
  .fmt     = RIT_COLORFMT,        /* Color format: RGB16-565: RRRR RGGG GGGB BBBB */
  .xres    = RIT_XRES,            /* Horizontal resolution in pixel columns */
  .yres    = RIT_YRES,            /* Vertical resolution in pixel rows */
  .nplanes = 1,                   /* Number of color planes supported */
};

/* This is the standard, NuttX Plane information object */

static const struct lcd_planeinfo_s g_planeinfo = 
{
  .putrun = rit_putrun,            /* Put a run into LCD memory */
  .getrun = rit_getrun,            /* Get a run from LCD memory */
  .buffer = (uint8_t*)g_runbuffer, /* Run scratch buffer */
  .bpp    = RIT_BPP,               /* Bits-per-pixel */
};

/* This is the standard, NuttX LCD driver object */

static struct rit_dev_s g_oleddev[CONFIG_P14201_NINTERFACES];


/* A table of magic initialization commands. This initialization sequence is
 * derived from RiT Application Note for the P14201 (with a few tweaked values
 * as discovered in some Luminary code examples).
 */

static const uint8_t g_initcmds[] =
{
  3,  SSD1329_CMD_LOCK,                 /* Set lock command */
      SSD1329_LOCK_OFF,                 /* Disable locking */
      SSD1329_NOOP,
  2,  SSD1329_SLEEP_ON,                 /* Matrix display OFF */
      SSD1329_NOOP,
  3,  SSD1329_ICON_ALL,                 /* Set all ICONs to OFF */
      SSD1329_ICON_OFF,                 /* OFF selection */
      SSD1329_NOOP,
  3,  SSD1329_MUX_RATIO,                /* Set MUX ratio */
      95,                               /* 96 MUX */
      SSD1329_NOOP,
  3,  SSD1329_SET_CONTRAST,             /* Set contrast */
      RIT_CONTRAST,                     /* Default contrast */
      SSD1329_NOOP,
  3,  SSD1329_PRECHRG2_SPEED,           /* Set second pre-charge speed */
     (31 << 1) | SSD1329_PRECHRG2_DBL,  /* Pre-charge speed == 32, doubled */
      SSD1329_NOOP,
  3,  SSD1329_GDDRAM_REMAP,             /* Set GDDRAM re-map */
     (SSD1329_COM_SPLIT|                /* Enable COM slip even/odd */
      SSD1329_COM_REMAP|                /* Enable COM re-map */
      SSD1329_NIBBLE_REMAP),            /* Enable nibble re-map */
      SSD1329_NOOP,
  3,  SSD1329_VERT_START,               /* Set Display Start Line */
      0,                                /* Line = 0 */
      SSD1329_NOOP,
  3,  SSD1329_VERT_OFFSET,              /* Set Display Offset */
      0,                                /* Offset = 0 */
      SSD1329_NOOP,
  2,  SSD1329_DISP_NORMAL,              /* Display mode normal */
      SSD1329_NOOP,
  3,  SSD1329_PHASE_LENGTH,             /* Set Phase Length */
      1 |                               /* Phase 1 period = 1 DCLK */
     (1 << 4),                          /* Phase 2 period = 1 DCLK */
      SSD1329_NOOP,
  3,  SSD1329_FRAME_FREQ,
      35,                               /* 35 DCLK's per row */
      SSD1329_NOOP,
  3,  SSD1329_DCLK_DIV,                 /* Set Front Clock Divider / Oscillator Frequency */
      2 |                               /* Divide ration = 3 */
     (14 << 4),                         /* Oscillator Frequency, FOSC, setting */
      SSD1329_NOOP,
  17, SSD1329_GSCALE_LOOKUP,            /* Look Up Table for Gray Scale Pulse width */
      1,   2,   3, 4,  5,  6, 8, 10,    /* Value for GS1-8 level Pulse width */
      12, 14, 16, 19, 22, 26, 30,       /* Value for GS9-15 level Pulse width */
      SSD1329_NOOP,
  3,  SSD1329_PRECHRG2_PERIOD,          /* Set Second Pre-charge Period */
      1,                                /* 1 DCLK */
      SSD1329_NOOP,
    // Pre-charge voltage
  3,  SSD1329_PRECHRG1_VOLT,            /* Set First Precharge voltage, VP */
      0x3f,                             /* 1.00 x Vcc */
      SSD1329_NOOP,
  0                                     /* Zero length command terminates table */
};

/* Turn the maxtrix display on (sleep mode off) */

static const uint8_t g_sleepoff[] =
{
  SSD1329_SLEEP_OFF,                    /* Matrix display ON */
  SSD1329_NOOP,
};

/* Turn the maxtrix display off (sleep mode on) */

static const uint8_t g_sleepon[] =
{
  SSD1329_SLEEP_ON,                     /* Matrix display OFF */
  SSD1329_NOOP,
};

/**************************************************************************************
 * Private Functions
 **************************************************************************************/

/**************************************************************************************
 * Function: rit_configspi
 *
 * Description:
 *   Configure the SPI for use with the P14201
 *
 * Parameters:
 *   spi  - Reference to the SPI driver structure
 *
 * Returned Value:
 *   None
 *
 * Assumptions:
 *
 **************************************************************************************/
 
static inline void rit_configspi(FAR struct spi_dev_s *spi)
{
  /* Configure SPI for the P14201.  But only if we own the SPI bus.  Otherwise, don't
   * bother because it might change.
   */

#ifdef CONFIG_P14201_OWNBUS
  SPI_SETMODE(spi, CONFIG_P14201_SPIMODE);
  SPI_SETBITS(spi, 8);
#ifdef CONFIG_P14201_FREQUENCY
  SPI_SETFREQUENCY(spi, CONFIG_P14201_FREQUENCY)
#endif
#endif
}

/**************************************************************************************
 * Function: rit_select
 *
 * Description:
 *   Select the SPI, locking and  re-configuring if necessary
 *
 * Parameters:
 *   spi  - Reference to the SPI driver structure
 *
 * Returned Value:
 *   None
 *
 * Assumptions:
 *
 **************************************************************************************/

#ifdef CONFIG_P14201_OWNBUS
static inline void rit_select(FAR struct spi_dev_s *spi)
{
  /* We own the SPI bus, so just select the chip */

  SPI_SELECT(spi, SPIDEV_DISPLAY, true);
}
#else
static void rit_select(FAR struct spi_dev_s *spi)
{
  /* Select P14201 chip (locking the SPI bus in case there are multiple
   * devices competing for the SPI bus
   */

  SPI_LOCK(spi, true);
  SPI_SELECT(spi, SPIDEV_DISPLAY, true);

  /* Now make sure that the SPI bus is configured for the P14201 (it
   * might have gotten configured for a different device while unlocked)
   */

  SPI_SETMODE(spi, CONFIG_P14201_SPIMODE);
  SPI_SETBITS(spi, 8);
#ifdef CONFIG_P14201_FREQUENCY
  SPI_SETFREQUENCY(spi, CONFIG_P14201_FREQUENCY);
#endif
}
#endif

/**************************************************************************************
 * Function: rit_deselect
 *
 * Description:
 *   De-select the SPI
 *
 * Parameters:
 *   spi  - Reference to the SPI driver structure
 *
 * Returned Value:
 *   None
 *
 * Assumptions:
 *
 **************************************************************************************/

#ifdef CONFIG_P14201_OWNBUS
static inline void rit_deselect(FAR struct spi_dev_s *spi)
{
  /* We own the SPI bus, so just de-select the chip */

  SPI_SELECT(spi, SPIDEV_DISPLAY, false);
}
#else
static void rit_deselect(FAR struct spi_dev_s *spi)
{
  /* De-select P14201 chip and relinquish the SPI bus. */

  SPI_SELECT(spi, SPIDEV_DISPLAY, false);
  SPI_LOCK(spi, false);
}
#endif

/**************************************************************************************
 * Function: rit_sndbytes
 *
 * Description:
 *   Send a sequence of command or data bytes to the SSD1329 controller.
 *
 * Parameters:
 *   spi    - Reference to the SPI driver structure
 *   buffer - A reference to memory containing the command bytes to be sent.
 *   buflen - The number of command bytes in buffer to be sent
 *
 * Returned Value:
 *   None
 *
 * Assumptions:
 *
 **************************************************************************************/

static void rit_sndbytes(FAR struct spi_dev_s *spi, FAR const uint8_t *buffer,
                         size_t buflen, bool data)
{
  uint8_t tmp;

  DEBUGASSERT(spi);

  /* Select the SD1329 controller */

  rit_select(spi);

  /* Clear the D/Cn bit to enable command mode */

  rit_seldata(spi, data);

  /* Loop until the entire command is transferred */

  while (buflen-- > 0)
    {
      /* Write the next byte to the controller */
 
      tmp = *buffer++;
      (void)SPI_SEND(spi, tmp);

      /* Send a dummy byte */

      (void)SPI_SEND(spi, 0xff);
   }

 /* De-select the SD1329 controller */

 rit_deselect(spi);
}

/**************************************************************************************
 * Function: rit_sndcmd
 *
 * Description:
 *   Send multiple commands from a table of commands.
 *
 * Parameters:
 *   spi    - Reference to the SPI driver structure
 *   table  - A reference to table containing all of the commands to be sent.
 *
 * Returned Value:
 *   None
 *
 * Assumptions:
 *
 **************************************************************************************/

static void rit_sndcmds(FAR struct spi_dev_s *spi, FAR const uint8_t *table)
{
  int cmdlen;

  /* Table terminates with a zero length command */

  while ((cmdlen = *table++) != 0)
    {
      rit_sndcmd(spi, table, cmdlen);
      table += cmdlen;
    }
}

/**************************************************************************************
 * Name:  rit_putrun
 *
 * Description:
 *   This method can be used to write a partial raster line to the LCD:
 *
 *   row     - Starting row to write to (range: 0 <= row < yres)
 *   col     - Starting column to write to (range: 0 <= col <= xres-npixels)
 *   buffer  - The buffer containing the run to be written to the LCD
 *   npixels - The number of pixels to write to the LCD
 *             (range: 0 < npixels <= xres-col)
 *
 **************************************************************************************/

static int rit_putrun(fb_coord_t row, fb_coord_t col, FAR const uint8_t *buffer,
                       size_t npixels)
{
  /* Buffer must be provided and aligned to a 16-bit address boundary */

  gvdbg("row: %d col: %d npixels: %d\n", row, col, npixels);
  DEBUGASSERT(buffer && ((uintptr_t)buffer & 1) == 0);

  /* Set up to write the run. */

  /* Write the run to GRAM. */

  return OK;
}

/**************************************************************************************
 * Name:  rit_getrun
 *
 * Description:
 *   This method can be used to read a partial raster line from the LCD:
 *
 *  row     - Starting row to read from (range: 0 <= row < yres)
 *  col     - Starting column to read read (range: 0 <= col <= xres-npixels)
 *  buffer  - The buffer in which to return the run read from the LCD
 *  npixels - The number of pixels to read from the LCD
 *            (range: 0 < npixels <= xres-col)
 *
 **************************************************************************************/

static int rit_getrun(fb_coord_t row, fb_coord_t col, FAR uint8_t *buffer,
                       size_t npixels)
{
  /* Buffer must be provided and aligned to a 16-bit address boundary */

  gvdbg("row: %d col: %d npixels: %d\n", row, col, npixels);
  DEBUGASSERT(buffer && ((uintptr_t)buffer & 1) == 0);

  /* Set up to read the run */

  /* Read the run from GRAM. */

  return OK;
}

/**************************************************************************************
 * Name:  rit_getvideoinfo
 *
 * Description:
 *   Get information about the LCD video controller configuration.
 *
 **************************************************************************************/

static int rit_getvideoinfo(FAR struct lcd_dev_s *dev,
                              FAR struct fb_videoinfo_s *vinfo)
{
  DEBUGASSERT(dev && vinfo);
  gvdbg("fmt: %d xres: %d yres: %d nplanes: %d\n",
         g_videoinfo.fmt, g_videoinfo.xres, g_videoinfo.yres, g_videoinfo.nplanes);
  memcpy(vinfo, &g_videoinfo, sizeof(struct fb_videoinfo_s));
  return OK;
}

/**************************************************************************************
 * Name:  rit_getplaneinfo
 *
 * Description:
 *   Get information about the configuration of each LCD color plane.
 *
 **************************************************************************************/

static int rit_getplaneinfo(FAR struct lcd_dev_s *dev, unsigned int planeno,
                              FAR struct lcd_planeinfo_s *pinfo)
{
  DEBUGASSERT(dev && pinfo && planeno == 0);
  gvdbg("planeno: %d bpp: %d\n", planeno, g_planeinfo.bpp);
  memcpy(pinfo, &g_planeinfo, sizeof(struct lcd_planeinfo_s));
  return OK;
}

/**************************************************************************************
 * Name:  rit_getpower
 *
 * Description:
 *   Get the LCD panel power status (0: full off - CONFIG_LCD_MAXPOWER: full on. On
 *   backlit LCDs, this setting may correspond to the backlight setting.
 *
 **************************************************************************************/

static int rit_getpower(struct lcd_dev_s *dev)
{
  struct rit_dev_s *priv = (struct rit_dev_s *)dev;
  DEBUGASSERT(priv);

  gvdbg("power: %s\n", priv->on ? "ON" : "OFF");
  return (int)priv->on;
}

/**************************************************************************************
 * Name:  rit_setpower
 *
 * Description:
 *   Enable/disable LCD panel power (0: full off - CONFIG_LCD_MAXPOWER: full on). On
 *   backlit LCDs, this setting may correspond to the backlight setting.
 *
 **************************************************************************************/

static int rit_setpower(struct lcd_dev_s *dev, int power)
{
  struct rit_dev_s *priv = (struct rit_dev_s *)dev;
  DEBUGASSERT(priv && (unsigned)power <= CONFIG_LCD_MAXPOWER && priv->spi);

  gvdbg("power: %d\n", power);
  if (power > 0)
    {
      /* Re-initialize the SSD1329 controller */

      rit_sndcmds(priv->spi, g_initcmds);

      /* Take the display out of sleep mode */

      rit_sndcmd(priv->spi, g_sleepoff, sizeof(g_sleepon));
    }
  else
    {
      /* Put the display into sleep mode */

      rit_sndcmd(priv->spi, g_sleepon, sizeof(g_sleepon));
    }
  return -ENOSYS; /* Not implemented */
}

/**************************************************************************************
 * Name:  rit_getcontrast
 *
 * Description:
 *   Get the current contrast setting (0-CONFIG_LCD_MAXCONTRAST).
 *
 **************************************************************************************/

static int rit_getcontrast(struct lcd_dev_s *dev)
{
  struct rit_dev_s *priv = (struct rit_dev_s *)dev;

  gvdbg("contrast: %d\n", priv->contrast);
  return priv->contrast;
}

/**************************************************************************************
 * Name:  rit_setcontrast
 *
 * Description:
 *   Set LCD panel contrast (0-CONFIG_LCD_MAXCONTRAST).
 *
 **************************************************************************************/

static int rit_setcontrast(struct lcd_dev_s *dev, unsigned int contrast)
{
  struct rit_dev_s *priv = (struct rit_dev_s *)dev;
  uint8_t cmd[3];

  gvdbg("contrast: %d\n", contrast);
  DEBUGASSERT(contrast <= CONFIG_LCD_MAXCONTRAST);

  /* Set new contrast */

  cmd[0] = SSD1329_SET_CONTRAST;
  cmd[1] = contrast;
  cmd[2] = SSD1329_NOOP;
  rit_sndcmd(priv->spi, cmd, 3);

  priv->contrast = contrast;
  return OK;
}

/**************************************************************************************
 * Public Functions
 **************************************************************************************/

/**************************************************************************************
 * Name:  rit_initialize
 *
 * Description:
 *   Initialize the LCD video hardware.  The initial state of the LCD is fully
 *   initialized, display memory cleared, and the LCD ready to use, but with the power
 *   setting at 0 (full off).
 *
 *   Return a a reference to the LCD object for the specified OLED.  This allows
 *   support for multiple OLED devices.
 *
 **************************************************************************************/

FAR struct lcd_dev_s *rit_initialize(FAR struct spi_dev_s *spi, int devno)
{
  DEBUGASSERT(devno == 0 && spi);

  gvdbg("Initializing devno: %d\n", devno);
  if ((unsigned)devno < CONFIG_P14201_NINTERFACES)
    {
      FAR struct rit_dev_s *priv = (FAR struct rit_dev_s *)&g_oleddev[devno].dev;

      /* Configure and enable LCD */

      rit_configspi(spi);
      rit_sndcmds(spi, g_initcmds);

      /* Initialize device structure */

      priv->dev.getvideoinfo = rit_getvideoinfo;
      priv->dev.getplaneinfo = rit_getplaneinfo;
      priv->dev.getpower     = rit_getpower;
      priv->dev.setpower     = rit_setpower;
      priv->dev.getcontrast  = rit_getcontrast;
      priv->dev.setcontrast  = rit_setcontrast;
      priv->spi              = spi;
      priv->contrast         = RIT_CONTRAST;
      priv->on               = false;
      return &priv->dev;
    }
  return NULL;
}
