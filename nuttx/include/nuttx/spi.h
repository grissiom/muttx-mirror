/****************************************************************************
 * include/nuttx/spi.h
 *
 *   Copyright(C) 2008-2009 Gregory Nutt. All rights reserved.
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
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES(INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

#ifndef __NUTTX_SPI_H
#define __NUTTX_SPI_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <sys/types.h>

/****************************************************************************
 * Definitions
 ****************************************************************************/

/* Access macros */

/****************************************************************************
 * Name: SPI_SELECT
 *
 * Description:
 *   Enable/disable the SPI chip select.   The implementation of this method
 *   must include handshaking:  If a device is selected, it must hold off
 *   all other attempts to select the device until the device is deselected.
 *   Required.
 *
 * Input Parameters:
 *   dev -      Device-specific state data
 *   devid -    Identifies the device to select
 *   selected - TRUE: slave selected, FALSE: slave de-selected
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

#define SPI_SELECT(d,id,s) ((d)->ops->select(d,id,s))

/****************************************************************************
 * Name: SPI_SETFREQUENCY
 *
 * Description:
 *   Set the SPI frequency. Required.
 *
 * Input Parameters:
 *   dev -       Device-specific state data
 *   frequency - The SPI frequency requested
 *
 * Returned Value:
 *   Returns the actual frequency selected
 *
 ****************************************************************************/

#define SPI_SETFREQUENCY(d,f) ((d)->ops->setfrequency(d,f))

/****************************************************************************
 * Name: SPI_SETMODE
 *
 * Description:
 *   Set the SPI mode. Optional.  See enum spi_mode_e for mode definitions
 *
 * Input Parameters:
 *   dev -  Device-specific state data
 *   mode - The SPI mode requested
 *
 * Returned Value:
 *   none
 *
 ****************************************************************************/

#define SPI_SETMODE(d,m) ((d)->ops->mode ? (d)->ops->mode(d,m) : (void))

/****************************************************************************
 * Name: SPI_STATUS
 *
 * Description:
 *   Get SPI/MMC status.  Optional.
 *
 * Input Parameters:
 *   dev -   Device-specific state data
 *   devid - Identifies the device to report status on
 *
 * Returned Value:
 *   Returns a bitset of status values (see SPI_STATUS_* defines
 *
 ****************************************************************************/

#define SPI_STATUS(d,id) \
  ((d)->ops->status ? (d)->ops->status(d, id) : SPI_STATUS_PRESENT)

/* SPI status bits -- Some dedicated for SPI MMC/SD support and may have no
 * relationship to SPI other than needed by the SPI MMC/SD interface
 */

#define SPI_STATUS_PRESENT     0x01 /* Bit 0=1: MMC/SD card present */
#define SPI_STATUS_WRPROTECTED 0x02 /* Bit 1=1: MMC/SD card write protected */

/****************************************************************************
 * Name: SPI_SNDBYTE
 *
 * Description:
 *   Send one byte on SPI. Required.
 *
 * Input Parameters:
 *   dev - Device-specific state data
 *   ch  - The byte to send
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

#define SPI_SNDBYTE(d,ch) ((d)->ops->sndbyte(d,ch))

/****************************************************************************
 * Name: SPI_SNDBLOCK
 *
 * Description:
 *   Send a block of data on SPI. Required.
 *
 * Input Parameters:
 *   dev -   Device-specific state data
 *   buffer - A pointer to the buffer of data to be sent
 *   buflen - the length of data to send from the buffer
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

#define SPI_SNDBLOCK(d,b,l) ((d)->ops->sndblock(d,b,l))

/****************************************************************************
 * Name: SPI_RECVBLOCK
 *
 * Description:
 *   Revice a block of data from SPI. Required.
 *
 * Input Parameters:
 *   dev -    Device-specific state data
 *   buffer - A pointer to the buffer in which to recieve data
 *   buflen - the length of data that can be received in the buffer
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

#define SPI_RECVBLOCK(d,b,l) ((d)->ops->recvblock(d,b,l))

/****************************************************************************
 * Name: SPI_REGISTERCALLBACK
 *
 * Description:
 *   Register a callback that that will be invoked on any media status
 *   change (i.e, anything that would be reported differently by SPI_STATUS).
 *   Optional
 *
 * Input Parameters:
 *   dev -      Device-specific state data
 *   callback - The funtion to call on the media change
 *   arg -      A caller provided value to return with the callback
 *
 * Returned Value:
 *   0 on success; negated errno on failure.
 *
 ****************************************************************************/

#define SPI_REGISTERCALLBACK(d,c,a) \
  ((d)->ops->registercallback ? (d)->ops->registercallback(d,c,a) : -ENOSYS)

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* The type of the media change callback function */

typedef void (*mediachange_t)(void *arg);

/* If the board supports multiple SPI devices, this enumeration identifies
 * which is selected or de-seleted.
 */

enum spi_dev_e
{
  SPIDEV_NONE = 0,  /* Not a valid value */
  SPIDEV_MMCSD,     /* Select SPI MMC/SD device */
  SPIDEV_ETHERNET   /* Select SPI ethernet device */
};

/* Certain SPI devices may required differnt clocking modes */

enum spi_mode_e
{
  SPIDEV_MODE0 = 0,  /* CPOL=0 CHPHA=0 */
  SPIDEV_MODE1,      /* CPOL=0 CHPHA=1 */
  SPIDEV_MODE2,      /* CPOL=1 CHPHA=0 */
  SPIDEV_MODE3       /* CPOL=1 CHPHA=1 */
};

/* The SPI vtable */

struct spi_dev_s;
struct spi_ops_s
{
  void   (*select)(FAR struct spi_dev_s *dev, enum spi_dev_e devid, boolean selected);
  uint32 (*setfrequency)(FAR struct spi_dev_s *dev, uint32 frequency);
  void   (*setmode)(FAR struct spi_dev_s *dev, enum spi_mode_e mode);
  ubyte  (*status)(FAR struct spi_dev_s *dev, enum spi_dev_e devid);
  ubyte  (*sndbyte)(FAR struct spi_dev_s *dev, ubyte ch);
  void   (*sndblock)(FAR struct spi_dev_s *dev, FAR const ubyte *buffer, size_t buflen);
  void   (*recvblock)(FAR struct spi_dev_s *dev, FAR ubyte *buffer, size_t buflen);
  int    (*registercallback)(FAR struct spi_dev_s *dev, mediachange_t callback, void *arg);
};

/* SPI private data.  This structure only defines the initial fields of the
 * structure visible to the SPI client.  The specific implementation may 
 * add additional, device specific fields
 */

struct spi_dev_s
{
  const struct spi_ops_s *ops;
};

/****************************************************************************
 * Public Functions
 ****************************************************************************/

#undef EXTERN
#if defined(__cplusplus)
#define EXTERN extern "C"
extern "C" {
#else
#define EXTERN extern
#endif

/****************************************************************************
 * Name: up_spiinitialize
 *
 * Description:
 *   Initialize the selected SPI port.
 *
 * Input Parameter:
 *   Port number (for hardware that has mutiple SPI interfaces)
 *
 * Returned Value:
 *   Valid SPI device structre reference on succcess; a NULL on failure
 *
 ****************************************************************************/

EXTERN FAR struct spi_dev_s *up_spiinitialize(int port);

#undef EXTERN
#if defined(__cplusplus)
}
#endif
#endif /* __NUTTX_SPI_H */
