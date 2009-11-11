/****************************************************************************
 * include/nuttx/sdio.h
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

#ifndef __NUTTX_SDIO_H
#define __NUTTX_SDIO_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <sys/types.h>

/****************************************************************************
 * Pre-Processor Definitions
 ****************************************************************************/

/* MMC/SD events needed by the driver */

#define SDIOEVENT_EJECTED       (1 << 0) /* Bit 0: CD/DAT3 transition low, media removed */
#define SDIOEVENT_INSERTED      (1 << 1) /* Bit 1: CD/DAT3 transition high, media inserted */
#define SDIOEVENT_CMDDONE       (1 << 2) /* Bit 2: Command+response complete */
#define SDIOEVENT_READCMDDONE   (1 << 3) /* Bit 3: Read command done */
#define SDIOEVENT_WRITECMDDONE  (1 << 4) /* Bit 4: Write command done */
#define SDIOEVENT_READDATADONE  (1 << 5) /* Bit 5: Read data done */
#define SDIOEVENT_WRITEDATADONE (1 << 6) /* Bit 6: Write data done */
#define SDIOEVENT_CMDBUSYDONE   (1 << 7) /* Bit 7: Command with transition to not busy */

#define SDIOEVENT_ALLEVENTS     0xff

/* Commands are bit-encoded to provide as much information to the SDIO driver as
 * possible in 32-bits.  The encoding is as follows:
 *
 * ---- ---- ---- ---- ---- --RR RRCC CCCC
 *
 * CCCCCC - Bits 0-5: 6-bit command index (Range 9-63)
 * RRRR   - Bits 6-9: 4-bit response code (R1, R1B, R2-5)
 */

/* MMC, SD, SDIO Common Indices */

#define MMCSD_CMDIDX_SHIFT (0)
#define MMCSD_CMDIDX_MASK  (0x3f << MMCSD_CMDIDX_SHIFT)
#  define MMCSD_CMDIDX0    0  /* GO_IDLE_STATE: Resets all cards to idle state
                               * -Broadcast, no response */
#  define MMC_CMDIDX1      1  /* SEND_OP_COND: Sends capacity support information
                               * -Broadcast, R3 response, 31:0=OCR */
#  define MMCSD_CMDIDX2    2  /* ALL_SEND_CID
                               * -Broadcast, R2 response */
#  define MMC_CMDIDX3      3  /* SET_RELATIVE_ADDR
                               * -Addressed Command, R1 response 31:16=RCA */
#  define SD_CMDIDX3       3  /* SEND_RELATIVE_ADDR
                               * -Addressed Command, R6 response 31:16=RCA */
#  define MMCSD_CMDIDX4    4  /* SET_DSR
                               * -Broadcast command, no response 31:16=RCA */
#  define SDIO_CMDIDX5     5  /* SDIO_SEND_OP_COND
                               * -Addressed Command, R4 response 47:16=IO_OCR */
#  define MMCSD_CMDIDX6    6  /* HS_SWITCH: Checks switchable function */
#  define MMCSD_CMDIDX7    7  /* SELECT/DESELECT CARD
                               * -Addressed Command, R1 response 31:16=RCA */
#  define SD_CMDIDX8       8  /* IF_COND: Sends SD Memory Card interface condition
                               * R7 response */
#  define MMCSD_CMDIDX9    9  /* SEND_CSD: Asks  card to send its card specific data (CSD)
                               * -Addressed Command, R2 response 31:16=RCA */
#  define MMCSD_CMDIDX10  10  /* SEND_CID: Asks card to send its card identification (CID)
                               * -Addressed Command, R2 response 31:16=RCA */
#  define MMC_CMDIDX11    11  /* READ_DAT_UNTIL_STOP   
                               * -Addressed data transfer command, R1 response 31:0=DADR */
#  define MMCSD_CMDIDX12  12  /* STOP_TRANSMISSION: Forces the card to stop transmission
                               * -Addressed Command, R1b response */
#  define MMCSD_CMDIDX13  13  /* SEND_STATUS: Asks card to send its status register
                               * -Addressed Command, R1 response 31:16=RCA */
#  define MMCSD_CMDIDX14  14  /* HS_BUSTEST_READ: */
#  define MMCSD_CMDIDX15  15  /* GO_INACTIVE_STATE
                               * Addressed Command, Response 31:16=RCA */
#  define MMCSD_CMDIDX16  16  /* SET_BLOCKLEN: Sets a block length (in bytes)
                               * -Addressed Command, R1 response 31:0=BLEN */
#  define MMCSD_CMDIDX17  17  /* READ_SINGLE_BLOCK: Reads a block of the selected size
                               * -Addressed data transfer command, R1 response 31:0=DADR */
#  define MMCSD_CMDIDX18  18  /* READ_MULTIPLE_BLOCK: Continuously transfers blocks from card to host
                               * -Addressed data transfer command, R1 response 31:0=DADR */
#  define MMCSD_CMDIDX19  19  /* HS_BUSTEST_WRITE: */
#  define MMC_CMDIDX20    20  /* WRITE_DAT_UNTIL_STOP: (MMC)
                               * -Addressed data transfer command, R1 response 31:0=DADR R1 */
#  define MMC_CMDIDX23    23  /* SET_BLOCK_COUNT: (MMC)
                               * -Addressed data transfer command, R1 response 31:0=DADR */
#  define MMCSD_CMDIDX24  24  /* WRITE_BLOCK: Writes a block of the selected size
                               * -Addressed data transfer command, R1 response 31:0=DADR */
#  define MMCSD_CMDIDX25  25  /* WRITE_MULTIPLE_BLOCK: Continuously writes blocks of data
                               * -Addressed data transfer command, R1 response 31:0=DADR */
#  define MMCSD_CMDIDX26  26  /* PROGRAM_CID: (Manufacturers only)
                               * -Addressed data transfer command, R1 response */
#  define MMCSD_CMDIDX27  27  /* PROGRAM_CSD: Set programmable bits of the CSD
                               * -Addressed data transfer command, R1 response */
#  define MMCSD_CMDIDX28  28  /* SET_WRITE_PROT: Sets the write protection bit of group
                               * -Addressed Command, R1b response 31:0=DADR */
#  define MMCSD_CMDIDX29  29  /* CLR_WRITE_PROT: Clears the write protection bit of group
                               * -Addressed Command, R1b response 31:0=DADR */
#  define MMCSD_CMDIDX30  30  /* SEND_WRITE_PROT: Asks card to send state of write protection bits
                               * -Addressed data transfer command, R1 response 31:0=WADR */
#  define SD_CMDIDX32     32  /* ERASE_GRP_START: Sets address of first block to erase (SD)
                               * -Addressed Command, R1 response 31:0=DADR */
#  define SD_CMDIDX33     33  /* ERASE_GRP_END: Sets address of last block to erase (SD)
                               * -Addressed Command, R1 response 31:0=DADR */
#  define MMC_CMDIDX34    34  /* UNTAG_SECTOR: (MMC)
                               * -Addressed Command, R1 response 31:0=DADR */
#  define MMC_CMDIDX35    35  /* TAG_ERASE_GROUP_START: Sets address of first block to erase (MMC)
                               * -Addressed Command, R1 response 31:0=DADR */
#  define MMC_CMDIDX36    36  /* TAG_ERASE_GROUP_END: Sets address of last block to erase (MMC)
                               * -Addressed Command, R1 response 31:0=DADR */
#  define MMC_CMDIDX37    37  /* UNTAG_ERASE_GROUP: (MMC)
                               * -Addressed Command, R1 response 31:0=DADR */
#  define MMCSD_CMDIDX38  38  /* ERASE: Erases all previously selected write blocks
                               * -Addressed Command, R1b response */
#  define MMC_CMDIDX39    39  /* FAST_IO: (MMC)
                               * -Addressed Command, R4 response (Complex) */
#  define MMC_CMDIDX40    40  /* GO_IRQ_STATE: (MMC)
                               * -Broadcast command, R5 response */
#  define MMCSD_CMDIDX42  42  /* LOCK_UNLOCK: Used to Set/Reset the Password or lock/unlock card
                               * -Addressed data transfer command, R1b response */
#  define SD_CMDIDX55     55  /* APP_CMD: Tells card that the next command is an application specific command
                               * - Addressed Command, R1 response 31:16=RCA */
#  define MMCSD_CMDIDX56  56  /* GEN_CMD: Used transfer a block to or get block from card
                               * -Addressed data transfer command, R1 Response */

/* SD/SDIO APP commands (must be preceded by CMD55) */

#  define SD_ACMDIDX6      6  /* SET_BUS_WIDTH:
                               * -Addressed Command, R1 response 1:0=BUSW */
#  define SD_ACMDIDX13    13  /* SD_STATUS: Send the SD Status
                               * -Addressed data transfer command, R1 response */
#  define SD_ACMDIDX18    18  /* SECURE_READ_MULTIPLE_BLOCK: */
#  define SD_ACMDIDX22    22  /* SEND_NUM_WR_BLOCKS: Send number of the errorfree blocks
                               * -Addressed data transfer command, R1 response */
#  define SD_ACMDIDX23    23  /* SET_WR_BLK_ERASE_COUNT: Set number blocks to erase before writing
                               * -Addressed Command, R1 response 22:0=NBLK */
#  define SD_ACMDIDX25    25  /* SECURE_WRITE_MULTIPLE_BLOCK: */
#  define SD_ACMDIDX38    38  /* SECURE_ERASE: */
#  define SD_ACMDIDX41    41  /* SD_SEND_OP_COND: Sends host capacity support information
                               * -Broadcast command, R3 response 31:0=OCR */
#  define SD_ACMDIDX42    42  /* SET_CLR_CARD_DETECT: Connect/disconnect pull-up resistor on CS
                               * Addressed Command, R1 response 0:0=CD */
#  define SD_ACMDIDX43    43  /* GET_MKB: */
#  define SD_ACMDIDX44    44  /* GET_MID: */
#  define SD_ACMDIDX45    45  /* SET_CER_RN1: */
#  define SD_ACMDIDX46    46  /* GET_CER_RN2: */
#  define SD_ACMDIDX47    47  /* SET_CER_RES2: */
#  define SD_ACMDIDX48    48  /* GET_CER_RES1/WRITE_MKB: */
#  define SD_ACMDIDX49    49  /* CHANGE_SECURE_AREA: */
#  define SD_ACMDIDX51    51  /* SEND_SCR: Reads the SD Configuration Register (SCR)
                               * Addressed data transfer command, R1 response */
#  define SDIO_ACMDIDX52  52  /* IO_RW_DIRECT: (SDIO only)
                               * -R5 response, 23:16=status 15:8=data */
#  define SDIO_ACMDIDX53  53  /* IO_RW_EXTENDED: (SDIO only)
                               * -R5 response, 23:16=status */

/* Response Encodings */

#define MMCSD_RESPONSE_SHIFT (6)
#define MMCSD_RESPONSE_MASK  (15 << MMCSD_RESPONSE_SHIFT)
#  define MMCSD_NO_RESPONSE  (0 << MMCSD_RESPONSE_SHIFT)
#  define MMCSD_R1_RESPONSE  (1 << MMCSD_RESPONSE_SHIFT)
#  define MMCSD_R1B_RESPONSE (2 << MMCSD_RESPONSE_SHIFT)
#  define MMCSD_R2_RESPONSE  (3 << MMCSD_RESPONSE_SHIFT)
#  define MMCSD_R3_RESPONSE  (4 << MMCSD_RESPONSE_SHIFT)
#  define MMCSD_R4_RESPONSE  (5 << MMCSD_RESPONSE_SHIFT)
#  define MMCSD_R5_RESPONSE  (6 << MMCSD_RESPONSE_SHIFT)
#  define MMCSD_R6_RESPONSE  (7 << MMCSD_RESPONSE_SHIFT)
#  define MMCSD_R7_RESPONSE  (8 << MMCSD_RESPONSE_SHIFT)

/* Fully decorated MMC, SD, SDIO commands */

#define MMCSD_CMD0      (MMCSD_CMDIDX0 |MMCSD_NO_RESPONSE)
#define MMC_CMD1        (MMC_CMDIDX1   |MMCSD_R3_RESPONSE)
#define MMCSD_CMD2      (MMCSD_CMDIDX2 |MMCSD_R2_RESPONSE)
#define MMC_CMD3        (MMC_CMDIDX3   |MMCSD_R1_RESPONSE)
#define SD_CMD3         (SD_CMDIDX3    |MMCSD_R6_RESPONSE)
#define MMCSD_CMD4      (MMCSD_CMDIDX4 |MMCSD_NO_RESPONSE)
#define SDIO_CMD5       (SDIO_CMDIDX5  |MMCSD_R4_RESPONSE)
#define MMCSD_CMD6      (MMCSD_CMDIDX6 |MMCSD_R1_RESPONSE)
#define MMCSD_CMD7S     (MMCSD_CMDIDX7 |MMCSD_R1B_RESPONSE)
#define MMCSD_CMD7D     (MMCSD_CMDIDX7 |MMCSD_NO_RESPONSE)  /* No response when de-selecting card */
#define SD_CMD8         (SD_CMDIDX8    |MMCSD_R7_RESPONSE)
#define MMCSD_CMD9      (MMCSD_CMDIDX9 |MMCSD_R2_RESPONSE)
#define MMCSD_CMD10     (MMCSD_CMDIDX10|MMCSD_R2_RESPONSE)
#define MMC_CMD11       (MMC_CMDIDX11  |MMCSD_R1_RESPONSE)
#define MMCSD_CMD12     (MMCSD_CMDIDX12|MMCSD_R1B_RESPONSE)
#define MMCSD_CMD13     (MMCSD_CMDIDX13|MMCSD_R1_RESPONSE)
#define MMCSD_CMD14     (MMCSD_CMDIDX14|MMCSD_R1_RESPONSE)
#define MMCSD_CMD15     (MMCSD_CMDIDX15|MMCSD_R1_RESPONSE)
#define MMCSD_CMD16     (MMCSD_CMDIDX16|MMCSD_R1_RESPONSE)
#define MMCSD_CMD17     (MMCSD_CMDIDX17|MMCSD_R1_RESPONSE)
#define MMCSD_CMD18     (MMCSD_CMDIDX18|MMCSD_R1_RESPONSE)
#define MMCSD_CMD19     (MMCSD_CMDIDX19|MMCSD_R1_RESPONSE)
#define MMC_CMD23       (MMC_CMDIDX23  |MMCSD_R1_RESPONSE)
#define MMCSD_CMD24     (MMCSD_CMDIDX24|MMCSD_R1_RESPONSE)
#define MMCSD_CMD25     (MMCSD_CMDIDX25|MMCSD_R1_RESPONSE)
#define MMCSD_CMD26     (MMCSD_CMDIDX26|MMCSD_R1_RESPONSE)
#define MMCSD_CMD27     (MMCSD_CMDIDX27|MMCSD_R1_RESPONSE)
#define MMCSD_CMD28     (MMCSD_CMDIDX28|MMCSD_R1B_RESPONSE)
#define MMCSD_CMD29     (MMCSD_CMDIDX29|MMCSD_R1B_RESPONSE)
#define MMCSD_CMD30     (MMCSD_CMDIDX30|MMCSD_R1_RESPONSE)
#define SD_CMD32        (SD_CMDIDX32   |MMCSD_R1_RESPONSE)
#define SD_CMD33        (SD_CMDIDX33   |MMCSD_R1_RESPONSE)
#define MMC_CMD34       (MMC_CMDIDX34  |MMCSD_R1_RESPONSE)
#define MMC_CMD35       (MMC_CMDIDX35  |MMCSD_R1_RESPONSE)
#define MMC_CMD36       (MMC_CMDIDX36  |MMCSD_R1_RESPONSE)
#define MMC_CMD37       (MMC_CMDIDX37  |MMCSD_R1_RESPONSE)
#define MMCSD_CMD38     (MMCSD_CMDIDX38|MMCSD_R1B_RESPONSE)
#define MMC_CMD39       (MMC_CMDIDX39  |MMCSD_R4_RESPONSE)
#define MMC_CMD40       (MMC_CMDIDX40  |MMCSD_R5_RESPONSE)
#define MMCSD_CMD42     (MMCSD_CMDIDX42|MMCSD_R1B_RESPONSE)
#define SD_CMD55        (SD_CMDIDX55   |MMCSD_R1_RESPONSE)
#define MMCSD_CMD56     (MMCSD_CMDIDX56|MMCSD_R1_RESPONSE)

/* SD/SDIO APP commands (must be preceded by CMD55) */

#define SD_ACMD6        (SD_ACMDIDX6   |MMCSD_R1_RESPONSE)
#define SD_ACMD13       (SD_ACMDIDX13  |MMCSD_R1_RESPONSE)
#define SD_ACMD18       (SD_ACMDIDX18  |MMCSD_R1_RESPONSE)
#define SD_ACMD22       (SD_ACMDIDX22  |MMCSD_R1_RESPONSE)
#define SD_ACMD23       (SD_ACMDIDX23  |MMCSD_R1_RESPONSE)
#define SD_ACMD25       (SD_ACMDIDX25  |MMCSD_R1_RESPONSE)
#define SD_ACMD38       (SD_ACMDIDX38  |MMCSD_R1_RESPONSE)
#define SD_ACMD41       (SD_ACMDIDX41  |MMCSD_R3_RESPONSE)
#define SD_ACMD42       (SD_ACMDIDX42  |MMCSD_R1_RESPONSE)
#define SD_ACMD43       (SD_ACMDIDX43  |MMCSD_R1_RESPONSE)
#define SD_ACMD44       (SD_ACMDIDX44  |MMCSD_R1_RESPONSE)
#define SD_ACMD45       (SD_ACMDIDX45  |MMCSD_R1_RESPONSE)
#define SD_ACMD46       (SD_ACMDIDX46  |MMCSD_R1_RESPONSE)
#define SD_ACMD47       (SD_ACMDIDX47  |MMCSD_R1_RESPONSE)
#define SD_ACMD48       (SD_ACMDIDX48  |MMCSD_R1_RESPONSE)
#define SD_ACMD49       (SD_ACMDIDX49  |MMCSD_R1_RESPONSE)
#define SD_ACMD51       (SD_ACMDIDX51  |MMCSD_R1_RESPONSE)
#define SDIO_ACMD52     (SDIO_ACMDIDX52|MMCSD_R5_RESPONSE)
#define SDIO_ACMD53     (SDIO_ACMDIDX53|MMCSD_R5_RESPONSE)

/****************************************************************************
 * Name: SDIO_RESET
 *
 * Description:
 *   Reset the MMC/SD controller.  Undo all setup and initialization.
 *
 * Input Parameters:
 *   dev    - An instance of the MMC/SD device interface
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

#define SDIO_RESET(dev) ((dev)->reset(dev))

/****************************************************************************
 * Name: SDIO_STATUS
 *
 * Description:
 *   Get MMC/SD status.
 *
 * Input Parameters:
 *   dev   - Device-specific state data
 *
 * Returned Value:
 *   Returns a bitset of status values (see SDIO_STATUS_* defines)
 *
 ****************************************************************************/

#define SDIO_STATUS(dev)        ((d)->status(dev))

/* MMC/SD status bits */

#define SDIO_STATUS_PRESENT     0x01 /* Bit 0=1: MMC/SD card present */
#define SDIO_STATUS_WRPROTECTED 0x02 /* Bit 1=1: MMC/SD card write protected */

#define SDIO_PRESENT(dev)       ((SDIO_STATUS(dev) & SDIO_STATUS_PRESENT) != 0)
#define SDIO_WRPROTECTED(dev)   ((SDIO_STATUS(dev) & SDIO_STATUS_WRPROTECTED) != 0)

/****************************************************************************
 * Name: SDIO_WIDEBUS
 *
 * Description:
 *   Called after change in Bus width has been selected (via ACMD6).  Most
 *   controllers will need to perform some special operations to work
 *   correctly in the new bus mode.
 *
 * Input Parameters:
 *   dev  - An instance of the MMC/SD device interface
 *   wide - TRUE: wide bus (4-bit) bus mode enabled
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

#define SDIO_WIDEBUS(dev,wide) ((dev)->widebus(dev,wide))

/****************************************************************************
 * Name: SDIO_CLOCK
 *
 * Description:
 *   Enable/disable MMC/SD clocking
 *
 * Input Parameters:
 *   dev  - An instance of the MMC/SD device interface
 *   rate - Specifies the clocking to use (see enum sdio_clock_e)
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

#define SDIO_CLOCK(dev,rate) ((dev)->clock(dev,rate))

/****************************************************************************
 * Name: SDIO_SETBLOCKLEN
 *
 * Description:
 *   Set the MMC/SD block length and block count
 *
 * Input Parameters:
 *   dev      - An instance of the MMC/SD device interface
 *   blocklen - The block length
 *   nblocks  - The block count
 *
 * Returned Value:
 *   OK on success; negated errno on failure
 *
 ****************************************************************************/

#define SDIO_SETBLOCKLEN(dev,blocklen,nblocks) \
  ((dev)->setblocklen(dev,blocklen,nblocks))

/****************************************************************************
 * Name: SDIO_ATTACH
 *
 * Description:
 *   Attach and prepare interrupts
 *
 * Input Parameters:
 *   dev    - An instance of the MMC/SD device interface
 *
 * Returned Value:
 *   OK on success; A negated errno on failure.
 *
 ****************************************************************************/

#define SDIO_ATTACH(dev) ((dev)->attach(dev))

/****************************************************************************
 * Name: SDIO_SENDCMD
 *
 * Description:
 *   Send the MMC/SD command
 *
 * Input Parameters:
 *   dev  - An instance of the MMC/SD device interface
 *   cmd  - The command to send.  See 32-bit command definitions above.
 *   arg  - 32-bit argument required with some commands
 *   data - A reference to data required with some commands
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

#define SDIO_SENDCMD(dev,cmd,arg,data) ((dev)->sendcmd(dev,cmd,arg,data))

/****************************************************************************
 * Name: SDIO_SENDDATA
 *
 * Description:
 *   Send more MMC/SD data
 *
 * Input Parameters:
 *   dev  - An instance of the MMC/SD device interface
 *   data - Data to be sent
 *
 * Returned Value:
 *   Number of bytes sent on succes; a negated errno on failure
 *
 ****************************************************************************/

#define SDIO_SENDDATA(dev,data) ((dev)->senddata(dev,data))

/****************************************************************************
 * Name: SDIO_RECVRx
 *
 * Description:
 *   Receive response to MMC/SD command
 *
 * Input Parameters:
 *   dev    - An instance of the MMC/SD device interface
 *   buffer - Buffer in which to receive the response
 *
 * Returned Value:
 *   Number of bytes sent on succes; a negated errno on failure
 *
 ****************************************************************************/

#define SDIO_RECVR1(dev,buffer) ((dev)->recvR1(dev,buffer))
#define SDIO_RECVR2(dev,buffer) ((dev)->recvR2(dev,buffer))
#define SDIO_RECVR3(dev,buffer) ((dev)->recvR3(dev,buffer))
#define SDIO_RECVR4(dev,buffer) ((dev)->recvR4(dev,buffer))
#define SDIO_RECVR5(dev,buffer) ((dev)->recvR5(dev,buffer))
#define SDIO_RECVR6(dev,buffer) ((dev)->recvR6(dev,buffer))

/****************************************************************************
 * Name: SDIO_RECVDATA
 *
 * Description:
 *   Receive data from MMC/SD
 *
 * Input Parameters:
 *   dev    - An instance of the MMC/SD device interface
 *   buffer - Buffer in which to receive the data
 *
 * Returned Value:
 *   Number of bytes sent on succes; a negated errno on failure
 *
 ****************************************************************************/

#define SDIO_RECVDATA(dev,buffer) ((dev)->recvdata(dev,buffer))

/****************************************************************************
 * Name: SDIO_EVENTENABLE
 *
 * Description:
 *   Enable/disable notification of a set of MMC/SD events
 *
 * Input Parameters:
 *   dev      - An instance of the MMC/SD device interface
 *   eventset - A bitset of events to enable or disable (see SDIOEVENT_*
 *              definitions
 *   enable   - TRUE: enable event; FALSE: disable events
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

#define SDIO_EVENTENABLE(dev,eventset)  ((dev)->eventenable(dev,eventset,TRUE))
#define SDIO_EVENTDISABLE(dev,eventset) ((dev)->eventenable(dev,eventset,FALSE))
#define SDIO_EVENTDISABLEALL(dev)       ((dev)->eventenable(dev,SDIOEVENT_ALLEVENTS,FALSE))

/****************************************************************************
 * Name: SDIO_EVENTWAIT
 *
 * Description:
 *   Wait for one of the enabled events to occur (or a timeout)
 *
 * Input Parameters:
 *   dev     - An instance of the MMC/SD device interface
 *   timeout - Maximum time in milliseconds to wait.  Zero means no timeout.
 *
 * Returned Value:
 *   Event set containing the event(s) that ended the wait.  If no events the
 *   returned event set is zero, then the wait was terminated by the timeout.
 *
 ****************************************************************************/

#define SDIO_EVENTWAIT(dev,timeout)  ((dev)->eventwait(dev,timeout))

/****************************************************************************
 * Name: SDIO_EVENTS
 *
 * Description:
 *   Return the current event set.  This supports polling for MMC/SD (vs.
 *   waiting).
 *
 * Input Parameters:
 *   dev     - An instance of the MMC/SD device interface
 *
 * Returned Value:
 *   Event set containing the current events (cleared after reading).
 *
 ****************************************************************************/

#define SDIO_EVENTS(dev)  ((dev)->events(dev))

/****************************************************************************
 * Name: SDIO_DMASUPPORTED
 *
 * Description:
 *   Return TRUE if the hardware can support DMA
 *
 * Input Parameters:
 *   dev - An instance of the MMC/SD device interface
 *
 * Returned Value:
 *   TRUE if DMA is supported.
 *
 ****************************************************************************/

#ifdef CONFIG_SDIO_DMA
#  define SDIO_DMASUPPORTED(dev) ((dev)->dmasupported(dev))
#else
#  define SDIO_DMASUPPORTED(dev) (FALSE)
#endif

/****************************************************************************
 * Name: SDIO_COHERENT
 *
 * Description:
 *   If the processor supports a data cache, then this method will make sure
 *   that the contents of the DMA memory and the data cache are coherent in
 *   preparation for the DMA transfer.  For write transfers, this may mean
 *   flushing the data cache, for read transfers this may mean invalidating
 *   the data cache.
 *
 * Input Parameters:
 *   dev   - An instance of the MMC/SD device interface
 *   addr  - The beginning address of the DMA
 *   len   - The length of the DMA
 *   write - TRUE: A write DMA will be performed; FALSE: a read DMA will be
 *           performed.
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

#if defined(CONFIG_SDIO_DMA) && defined(CONFIG_DATA_CACHE)
#  define SDIO_COHERENT(dev,addr,len,write) ((dev)->coherent(dev,addr,len,write))
#else
#  define SDIO_COHERENT(dev,addr,len,write)
#endif

/****************************************************************************
 * Name: SDIO_DMAREADSETUP
 *
 * Description:
 *   Setup to perform a read DMA
 *
 * Input Parameters:
 *   dev    - An instance of the MMC/SD device interface
 *   buffer - The memory to DMA from
 *
 * Returned Value:
 *   OK on succes; a negated errno on failure
 *
 ****************************************************************************/

#ifdef CONFIG_SDIO_DMA
#  define SDIO_DMAREADSETUP(dev,buffer) ((dev)->dmareadsetup(dev,buffer))
#else
#  define SDIO_DMAREADSETUP(dev,buffer) (-ENOSYS)
#endif

/****************************************************************************
 * Name: SDIO_DMAWRITESETUP
 *
 * Description:
 *   Setup to perform a write DMA
 *
 * Input Parameters:
 *   dev    - An instance of the MMC/SD device interface
 *   buffer - The memory to DMA into
 *
 * Returned Value:
 *   OK on succes; a negated errno on failure
 *
 ****************************************************************************/

#ifdef CONFIG_SDIO_DMA
#  define SDIO_DMAWRITESETUP(dev,buffer) ((dev)->dmawritesetup(dev,buffer))
#else
#  define SDIO_DMAWRITESETUP(dev,buffer) (-ENOSYS)
#endif

/****************************************************************************
 * Name: SDIO_DMASTART
 *
 * Description:
 *   Start the DMA
 *
 * Input Parameters:
 *   dev - An instance of the MMC/SD device interface
 *
 * Returned Value:
 *   OK on succes; a negated errno on failure
 *
 ****************************************************************************/

#ifdef CONFIG_SDIO_DMA
#  define SDIO_DMASTART(dev) ((dev)->dmastart(dev))
#else
#  define SDIO_DMASTART(dev) (-ENOSYS)
#endif

/****************************************************************************
 * Name: SDIO_DMASTOP
 *
 * Description:
 *   Stop the DMA
 *
 * Input Parameters:
 *   dev - An instance of the MMC/SD device interface
 *
 * Returned Value:
 *   OK on succes; a negated errno on failure
 *
 ****************************************************************************/

#ifdef CONFIG_SDIO_DMA
#  define SDIO_DMASTOP(dev) ((dev)->dmastop(dev))
#else
#  define SDIO_DMASTOP(dev) (-ENOSYS)
#endif

/****************************************************************************
 * Name: SDIO_DMASTATUS
 *
 * Description:
 *   Returnt the number of bytes remaining in the DMA transfer
 *
 * Input Parameters:
 *   dev       - An instance of the MMC/SD device interface
 *   remaining - A pointer to location in which to return the number of bytes
 *               remaining in the transfer.
 *
 * Returned Value:
 *   OK on succes; a negated errno on failure
 *
 ****************************************************************************/

#ifdef CONFIG_SDIO_DMA
#  define SDIO_DMASTATUS(dev,remaining) ((dev)->dmastatus(dev,remaining))
#else
#  define SDIO_DMASTATUS(dev,remaining) (-ENOSYS)
#endif

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* Various clocking used by the MMC/SD driver */

enum sdio_clock_e
{
  CLOCK_SDIO_DISABLED = 0, /* Clock is disabled */
  CLOCK_MMC_SLOW,           /* MMC initialization clocking */
  CLOCK_SD_SLOW,            /* SD initialization clocking */
  CLOCK_MMC_FAST,           /* MMC normal operation clocking */
  CLOCK_SD_FAST             /* SD normal operation clocking */
};

/* This structure defines the interface between the NuttX MMC/SD
 * driver and the chip- or board-specific MMC/SD interface.  This
 * interface is only used in architectures that support SDIO
 * 1- or 4-bit data busses.  For MMC/SD support this interface is
 * registered with the NuttX MMC/SD driver by calling
 * sdio_slotinitialize().
 */

struct sdio_dev_s
{
  /* See descriptions of each method in the access macros provided
   * above.
   */

  /* Initialization/setup */

  void  (*reset)(FAR struct sdio_dev_s *dev);
  ubyte (*status)(FAR struct sdio_dev_s *dev);
  void  (*widebus)(FAR struct sdio_dev_s *dev, boolean enable);
  void  (*clock)(FAR struct sdio_dev_s *dev, enum sdio_clock_e rate);
  int   (*setblocklen)(FAR struct sdio_dev_s *dev, int blocklen, int nblocks);
  int   (*attach)(FAR struct sdio_dev_s *dev);

  /* Command/Status/Data Transfer */

  void  (*sendcmd)(FAR struct sdio_dev_s *dev, uint32 cmd, uint32 arg, FAR const ubyte *data);
  int   (*senddata)(FAR struct sdio_dev_s *dev, FAR const ubyte *buffer);

  int   (*recvR1)(FAR struct sdio_dev_s *dev, uint16 buffer[3]);
  int   (*recvR2)(FAR struct sdio_dev_s *dev, uint16 buffer[8]);
  int   (*recvR3)(FAR struct sdio_dev_s *dev, uint16 buffer[3]);
  int   (*recvR4)(FAR struct sdio_dev_s *dev, uint16 buffer[3]);
  int   (*recvR5)(FAR struct sdio_dev_s *dev, uint16 buffer[3]);
  int   (*recvR6)(FAR struct sdio_dev_s *dev, uint16 buffer[3]);
  int   (*recvdata)(FAR struct sdio_dev_s *dev, FAR ubyte *buffer);

  /* EVENT handler */

  void  (*eventenable)(FAR struct sdio_dev_s *dev, ubyte eventset, boolean enable);
  ubyte (*eventwait)(FAR struct sdio_dev_s *dev, uint32 timeout);
  ubyte (*events)(FAR struct sdio_dev_s *dev);

  /* DMA */

#ifdef CONFIG_SDIO_DMA
  boolean (*dmasupported)(FAR struct sdio_dev_s *dev);
#ifdef CONFIG_DATA_CACHE
  void  (*coherent)(FAR struct sdio_dev_s *dev, FAR void *addr, size_t len, boolean write);
#endif
  int   (*dmareadsetup)(FAR struct sdio_dev_s *dev, FAR ubyte *buffer);
  int   (*dmawritesetup)(FAR struct sdio_dev_s *dev, FAR const ubyte *buffer);
  int   (*dmaenable)(FAR struct sdio_dev_s *dev);
  int   (*dmastop)(FAR struct sdio_dev_s *dev);
  int   (*dmastatus)(FAR struct sdio_dev_s *dev, size_t *remaining);
#endif
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

#undef EXTERN
#if defined(__cplusplus)
}
#endif
#endif /* __NUTTX_SDIO_H */
