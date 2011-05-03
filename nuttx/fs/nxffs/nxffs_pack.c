/****************************************************************************
 * fs/nxffs/nxffs_pack.c
 *
 *   Copyright (C) 2011 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <spudmonkey@racsa.co.cr>
 *
 * References: Linux/Documentation/filesystems/romfs.txt
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

#include <nuttx/config.h>

#include <string.h>
#include <errno.h>
#include <assert.h>
#include <crc32.h>
#include <debug.h>

#include <nuttx/kmalloc.h>

#include "nxffs.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Public Types
 ****************************************************************************/
/* This structure supports access to one inode data stream */

struct nxffs_packstream_s
{
  struct nxffs_entry_s entry;      /* Describes the inode header */
  off_t                fpos;       /* Current file position */
  off_t                blkoffset;  /* Offset to the current data block */
  uint16_t             blklen;     /* Size of this block */
  uint16_t             blkpos;     /* Position in block corresponding to fpos */
};

/* The structure supports the overall packing operation */

struct nxffs_pack_s
{
  /* These describe the source and destination streams */

  struct nxffs_packstream_s src;
  struct nxffs_packstream_s dest;

  /* These describe the state of the current contents of the (destination)
   * volume->pack buffer.
   */

  FAR uint8_t         *iobuffer;   /* I/O block start position */
  off_t                ioblock;    /* I/O block number */
  off_t                block0;     /* First I/O Block number in the erase block */
  uint16_t             iooffset;   /* I/O block offset */
};

/****************************************************************************
 * Public Variables
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxffs_getblock
 *
 * Description:
 *   Return the I/O block number that includes the provided offset.
 *
 * Input Parameters:
 *   volume - Describes the NXFFS volume
 *   offset - FLASH offset
 *
 * Returned Value:
 *   The I/O block number.
 *
 ****************************************************************************/

static off_t nxffs_getblock(FAR struct nxffs_volume_s *volume, off_t offset)
{
  return offset / volume->geo.blocksize;
}

/****************************************************************************
 * Name: nxffs_getoffset
 *
 * Description:
 *   Given an I/O block number return the block offset corresponding to the
 *   FLASH offset;
 *
 * Input Parameters:
 *   volume - Describes the NXFFS volume
 *   offset - FLASH offset
 *
 * Returned Value:
 *   The I/O block number.
 *
 ****************************************************************************/

static off_t nxffs_getoffset(FAR struct nxffs_volume_s *volume,
                             off_t offset, off_t block)
{
  return offset - block * volume->geo.blocksize;
}

/****************************************************************************
 * Name: nxffs_packtell
 *
 * Description:
 *   Report the current destination position in the pack buffer.
 *
 * Input Parameters:
 *   volume - Describes the NXFFS volume
 *   pack   - The volume packing state structure.
 *
 * Returned Value:
 *   The offset from the beginning of FLASH to the current seek position.
 *
 ****************************************************************************/

static off_t nxffs_packtell(FAR struct nxffs_volume_s *volume,
                            FAR struct nxffs_pack_s *pack)
{
  return pack->ioblock * volume->geo.blocksize + pack->iooffset;
}

/****************************************************************************
 * Name: nxffs_packvalid
 *
 * Description:
 *   Check if the current destination block is valid.
 *
 * Input Parameters:
 *   pack   - The volume packing state structure.
 *
 * Returned Values:
 *   None
 *
 ****************************************************************************/

static inline bool nxffs_packvalid(FAR struct nxffs_pack_s *pack)
{
  FAR struct nxffs_block_s *blkhdr;

  blkhdr = (FAR struct nxffs_block_s *)pack->iobuffer;
  return (memcmp(blkhdr->magic, g_blockmagic, NXFFS_MAGICSIZE) == 0 &&
          blkhdr->state == BLOCK_STATE_GOOD);
}

/****************************************************************************
 * Name: nxffs_mediacheck
 *
 * Description:
 *   Verify that there is at least one valid block and at least one valid
 *   inode header on the media.  On successful return, the volume packing
 *   structure is initialized and contains the offset to the first valid
 *   inode header is returned.
 *
 * Input Parameters:
 *   volume - The volume to be packed.
 *   pack   - The volume packing state structure.
 *
 * Returned Values:
 *   The offset to the data area on the first valid block.  Zero is return
 *   if there are no valid blocks or if there are no valid inode headers
 *   after the first valid block.
 *
 ****************************************************************************/

static inline off_t nxffs_mediacheck(FAR struct nxffs_volume_s *volume,
                                     FAR struct nxffs_pack_s *pack)
{
  off_t froffset;
  int ret;

  /* Find the FLASH offset to the first valid block */

  volume->ioblock = 0;
  ret = nxffs_validblock(volume, &volume->ioblock);
  if (ret < 0)
   {
     /* No valid blocks?  Return offset zero. */

     return 0;
   }

  /* The offset to the free location to pack is then just after the block
   * header in this block.
   */

  volume->iooffset = SIZEOF_NXFFS_BLOCK_HDR;
  froffset         = nxffs_iotell(volume);

  /* Get the offset to the first valid inode entry after this free offset */

  ret = nxffs_nextentry(volume, froffset, &pack->src.entry);
  if (ret < 0)
    {
      /* No valid entries on the media -- Return offset zero */

     return -ENOSPC;
    }

  /* Okay.. the start block and first entry have been found */

  return froffset;
}

/****************************************************************************
 * Name: nxffs_startpos
 *
 * Description:
 *   Find the position in FLASH memory where we should begin packing.  That
 *   position is the place where there is a gap between the last and the next
 *   valid inode.  On entry, the volume packing structure should be as it
 *   was initialized by nxffx_mediacheck.  on successful return, the volume
 *   packing state structure will be updated to begin the packing operation.
 *
 * Input Parameters:
 *   volume - The volume to be packed
 *   pack   - The volume packing state structure.
 *   offset - location to return the pointer to first valid inode header.
 *
 * Returned Values:
 *   Zero on success; Otherwise, a negated errno value is returned to
 *   indicate the nature of the failure.
 *
 ****************************************************************************/

static inline int nxffs_startpos(FAR struct nxffs_volume_s *volume,
                                 FAR struct nxffs_pack_s *pack,
                                 off_t offset)
{
  struct nxffs_blkentry_s blkentry;
  off_t wasted;
  off_t nbytes;
  int ret;

  /* Loop until we find a gap of unused FLASH large enough to warrant the
   * compression.
   */

  for(;;)
    {
      /* Is there wasted space between the offset where the we could have
       * valid data and the offset to the beginning of the first valid
       * inode header?  NOTE:  The threshold check is not accurate, there
       * may or may not be intervening block headers making the separation
       * seem larger than it is.
       */

      DEBUGASSERT(pack->src.entry.hoffset >= offset);
      wasted = pack->src.entry.hoffset - offset;
      if (wasted > CONFIG_NXFFS_PACKTHRESHOLD)
        {
          /* This is where we must begin packing.  Describe the destination
           * inode header (only non-zero entries need to be initialized).
           */

          pack->dest.entry.hoffset = offset;
          pack->dest.entry.name    = pack->src.entry.name;
          pack->dest.entry.utc     = pack->src.entry.utc;
          pack->dest.entry.datlen  = pack->src.entry.datlen;

          /* The destination entry now "owns" the name string */

          pack->src.entry.name     = NULL;
          return OK;
        }

      /* Free the allocated memory in the entry */

      nxffs_freeentry(&pack->src.entry);

      /* Update the offset to the first byte at the end of the last data
       * block.
       */

      nbytes = 0;
      offset = pack->src.entry.doffset;

      while (nbytes < pack->src.entry.datlen)
        {
          /* Read the next data block header */

          ret = nxffs_nextblock(volume, offset, &blkentry);
          if (ret < 0)
            {
              fdbg("Failed to find next data block: %d\n", -ret);
              return ret;
            }

          /* Get the number of blocks and pointer to where the next
           * data block might lie.
           */

          nbytes += blkentry.datlen;
          offset  = blkentry.hoffset + SIZEOF_NXFFS_DATA_HDR + blkentry.datlen;
        }

      /* Make sure there is space at this location for an inode header */

      nxffs_ioseek(volume, offset);
      if (volume->iooffset + SIZEOF_NXFFS_INODE_HDR > volume->geo.blocksize)
        {
          /* No.. not enough space here. Find the next valid block */

          volume->ioblock++;
          ret = nxffs_validblock(volume, &volume->ioblock);
          if (ret < 0)
            {
               /* No valid blocks? Then there is nothing we can do.  Return
                * the end-of-flash indication.
                */

               return -ENOSPC;
            }

          volume->iooffset = SIZEOF_NXFFS_BLOCK_HDR;
          offset = nxffs_iotell(volume);
        }

      /* Get the offset to the next valid inode entry */

      ret = nxffs_nextentry(volume, offset, &pack->src.entry);
      if (ret < 0)
        {
          /* No more valid inode entries.  Just return an end-of-flash error
           * indication.
           */

          return -ENOSPC;
        }
    }

  /* We won't get here */

  return -ENOSYS;
}

/****************************************************************************
 * Name: nxffs_srcsetup
 *
 * Description:
 *   Given a valid src inode, configure the src data stream.
 *
 * Input Parameters:
 *   volume - The volume to be packed
 *   pack   - The volume packing state structure.
 *   offset - FLASH offset to the data block header
 *
 * Returned Values:
 *   Zero on success; Otherwise, a negated errno value is returned to
 *   indicate the nature of the failure.
 *
 ****************************************************************************/

static int nxffs_srcsetup(FAR struct nxffs_volume_s *volume,
                          FAR struct nxffs_pack_s *pack, off_t offset)
{
  int ret;

  /* No, start with the first data block */

  pack->src.blkoffset = offset;
  pack->src.blkpos    = 0;

  /* Seek to the data block header, read and verify the block header */

  ret = nxffs_rdblkhdr(volume, offset, &pack->src.blklen);
  if (ret < 0)
    {
      fdbg("Failed to verify the data block header: %d\n", -ret);
    }

  return ret;
}

/****************************************************************************
 * Name: nxffs_destsetup
 *
 * Description:
 *   Given a valid dest inode, configure the dest data stream.
 *
 * Input Parameters:
 *   volume - The volume to be packed
 *   pack   - The volume packing state structure.
 *
 * Returned Values:
 *   Zero on success; Otherwise, a negated errno value is returned to
 *   indicate the nature of the failure.
 *
 ****************************************************************************/

static int nxffs_destsetup(FAR struct nxffs_volume_s *volume,
                           FAR struct nxffs_pack_s *pack)
{
  size_t mindata;
  int    namlen;

  /* The destination can be in one of three of states:
   *
   * State 1: The inode position was not yet been found.  This condition can
   * only occur on initial entry into nxffs_packblock() when there we no space
   * for the inode header at the end of the previous block.  We must now be
   * at the beginning of a shiny new I/O block, so we should always have
   * space for a new inode header right here.
   */

  if (pack->dest.entry.hoffset == 0)
    {
      DEBUGASSERT(pack->iooffset + SIZEOF_NXFFS_INODE_HDR <= volume->geo.blocksize);
      pack->dest.entry.hoffset = nxffs_packtell(volume, pack);
      pack->iooffset += SIZEOF_NXFFS_INODE_HDR;
    }

  /* State 2: inode position found, inode header not written, inode name
   * position not determined.
   */

  if (pack->dest.entry.noffset == 0)
    {
      /* Find the offset to the string memory.  Will if fit in this block?
       * Note: iooffset has already been incremented to account for the
       * size of the inode header.
       */

      namlen = strlen(pack->dest.entry.name);
      if (pack->iooffset + namlen < volume->geo.blocksize)
        {
          /* No.. that inode name will not fit in this block. Return an
           * indication that we are at the end of the block and try again
           * later.
           */

          return -ENOSPC;
        }

      /* Yes.. reserve space for the inode name (but don't write it yet) */

      pack->dest.entry.noffset = nxffs_packtell(volume, pack);
      pack->iooffset += namlen;
    }

  /* State 3: Inode header not-written, inode name written.  Still need the position
   * of the first data block.
   */

  if (pack->dest.entry.doffset == 0)
    {
      /* Will the data block header plus a minimal amount of data fit in this
       * block? (or the whole file if the file is very small).
       */

      mindata = MIN(NXFFS_MINDATA, pack->dest.entry.datlen);
      if (pack->iooffset + SIZEOF_NXFFS_DATA_HDR + mindata < volume->geo.blocksize)
        {
          /* No.. return an indication that we are at the end of the block
           * and try again later.
           */

          return -ENOSPC;
        }

      /* Yes.. reserve space for the data block header */

      pack->dest.entry.doffset = nxffs_packtell(volume, pack);
      pack->iooffset += SIZEOF_NXFFS_DATA_HDR;
    }

  /* Initialize the output data stream to start with the first data block */

  pack->dest.blkoffset = pack->dest.entry.doffset;
  pack->dest.blkpos    = 0;
  return OK;
}

/****************************************************************************
 * Name: nxffs_wrinodehdr
 *
 * Description:
 *   Write the destination inode header to FLASH.
 *
 * Input Parameters:
 *   volume - The volume to be packed
 *   pack   - The volume packing state structure.
 *
 * Returned Values:
 *   Zero on success; Otherwise, a negated errno value is returned to
 *   indicate the nature of the failure.
 *
 ****************************************************************************/

static int nxffs_wrinodehdr(FAR struct nxffs_volume_s *volume,
                            FAR struct nxffs_pack_s *pack)
{
  FAR struct nxffs_inode_s *inode;
  off_t ioblock;
  off_t namblock;
  uint16_t iooffset;
  uint16_t namoffset;
  uint32_t crc;
  int namlen;
  int ret;

  /* Get positions corresponding to the inode header and inode name positions */

  ioblock   = nxffs_getblock(volume, pack->dest.entry.hoffset);
  iooffset  = nxffs_getoffset(volume, pack->dest.entry.hoffset, ioblock);

  namblock  = nxffs_getblock(volume, pack->dest.entry.noffset);
  namoffset = nxffs_getoffset(volume, pack->dest.entry.noffset, namblock);

  /* The inode header is not written until all of the inode data has been
   * packed into its new location.  As a result, there are three possibilities:
   *
   * 1. The inode header lies in the current, unwritten erase block,
   * 2. The inode header resides in an earlier erase block and has already
   *    been written to FLASH, but the inode name resides within the erase
   *    block and has not been written to FLASH, or
   * 3. The inode header resides in an earlier erase block and has already
   *    been written to FLASH (most likely case for files larger than an
   *    erase block).
   *
   * Case 2 & 3: Does the inode header reside in a block before the beginning
   * of the current erase block?
   */

  if (ioblock < pack->block0)
    {
      /* Does the inode name also reside in a block before the beginning of
       * the current erase block?
       */

      if (namblock < pack->block0)
        {
          /* Yes.. this is case 3:  Both the inode block header and the inode
           * name lie in an earlier erase block that has already been written
           * to FLASH.  In this case, if we are very careful, we can just use
           * the standard routine to write the inode header that is called
           * during the normal file close operation:
           */

          ret = nxffs_wrinode(volume, &pack->dest.entry);
          return ret;
        }
      else
        {
          /* Case 2:  The inode header lies in an earlier erase block that
           * has been written to FLASH but the inode name is in the cache and
           * still unwritten.
           */

#warning "Missing logic"
return -ENOSYS;
        }
    }

  /* Cases 1:  Both the inode header and name are in the unwritten cache memory. */
  /* Initialize the inode header */

  iooffset += (ioblock - pack->block0) * volume->geo.blocksize;
  inode     = (FAR struct nxffs_inode_s *)&volume->pack[iooffset];
  memcpy(inode->magic, g_inodemagic, NXFFS_MAGICSIZE);

  nxffs_wrle32(inode->noffs,  pack->dest.entry.noffset);
  nxffs_wrle32(inode->doffs,  pack->dest.entry.doffset);
  nxffs_wrle32(inode->utc,    pack->dest.entry.utc);
  nxffs_wrle32(inode->crc,    0);
  nxffs_wrle32(inode->datlen, pack->dest.entry.datlen);

  /* Get the length of the inode name */

  namlen = strlen(pack->dest.entry.name);
  DEBUGASSERT(namlen < CONFIG_NXFFS_MAXNAMLEN);

  inode->state  = CONFIG_NXFFS_ERASEDSTATE;
  inode->namlen = namlen;

  /* Calculate the CRC */

  crc = crc32((FAR const uint8_t *)inode, SIZEOF_NXFFS_INODE_HDR);
  crc = crc32part((FAR const uint8_t *)pack->dest.entry.name, namlen, crc);

  /* Finish the inode header */

  inode->state = INODE_STATE_FILE;
  nxffs_wrle32(inode->crc, crc);

  /* Write the inode name */

  namoffset += (namblock - pack->block0) * volume->geo.blocksize;
  memcpy(&volume->pack[namoffset], pack->dest.entry.name, namlen);

  /* Reset the dest inode information */

  nxffs_freeentry(&pack->dest.entry);
  memset(&pack->dest, 0, sizeof(struct nxffs_packstream_s));
  return OK;
}

/****************************************************************************
 * Name: nxffs_wrdatthdr
 *
 * Description:
 *   Write the destination data block header to FLASH.
 *
 * Input Parameters:
 *   volume - The volume to be packed
 *   pack   - The volume packing state structure.
 *
 * Returned Values:
 *   Zero on success; Otherwise, a negated errno value is returned to
 *   indicate the nature of the failure.
 *
 ****************************************************************************/

static void nxffs_wrdathdr(FAR struct nxffs_volume_s *volume,
                           FAR struct nxffs_pack_s *pack)
{
  FAR struct nxffs_data_s *dathdr;
  off_t    ioblock;
  uint16_t iooffset;
  uint32_t crc;

  /* Get the offset in the block corresponding to the location of the inode
   * header.  NOTE:  This must lie in the same block as we currently have
   * buffered.
   */

  ioblock  = nxffs_getblock(volume, pack->dest.entry.hoffset);
  iooffset = nxffs_getoffset(volume, pack->dest.entry.hoffset, ioblock);
  DEBUGASSERT(ioblock == pack->ioblock);

  /* Write the data block header to memory */

  dathdr = (FAR struct nxffs_data_s *)&pack->iobuffer[iooffset];
  memcpy(dathdr->magic, g_datamagic, NXFFS_MAGICSIZE);
  nxffs_wrle32(dathdr->crc, 0);
  nxffs_wrle16(dathdr->datlen, pack->dest.blklen);

  /* Update the entire data block CRC (including the header) */

  crc = crc32(&volume->cache[volume->iooffset], pack->dest.blklen + SIZEOF_NXFFS_DATA_HDR);
  nxffs_wrle32(dathdr->crc, crc);

  /* Setup state to allocate the next data block */

  pack->dest.blkoffset = 0;
  pack->dest.blklen    = 0;
  pack->dest.blkpos    = 0;
}

/****************************************************************************
 * Name: nxffs_packblock
 *
 * Description:
 *   Resume packing from the source stream into the newly identified
 *   destination block.
 *
 * Input Parameters:
 *   volume - The volume to be packed
 *   pack   - The volume packing state structure.
 *
 * Returned Values:
 *   Zero on success; Otherwise, a negated errno value is returned to
 *   indicate the nature of the failure.
 *
 ****************************************************************************/

static inline int nxffs_packblock(FAR struct nxffs_volume_s *volume,
                                  FAR struct nxffs_pack_s *pack)
{
  off_t offset;
  int ret;

  /* Are we currently processing a block from the source stream? */

  if (pack->src.blkoffset)
    {
      /* No.. setup the source stream */

      ret = nxffs_srcsetup(volume, pack, pack->src.entry.doffset);
      if (ret < 0)
        {
          fdbg("Failed to configure the src stream: %d\n", -ret);
          return ret;
        }
    }

  /* We enter here on a new block every time, so we always have to setup
   * the dest data stream.
   */

  ret = nxffs_destsetup(volume, pack);
  if (ret < 0)
    {
      fdbg("Failed to configure the dest stream: %d\n", -ret);
      return ret;
    }

  /* Loop, transferring data from the source block to the destination pack
   * buffer until either (1) the source stream is exhausted, (2) the destination
   * block is full, or (3) an error occurs.
   */

  for (;;)
    {
       /* Determine how much data is available in the dest pack buffer */

       uint16_t destlen = volume->geo.blocksize - pack->dest.blkpos;

       /* Dermined how much data is available in the src data block */

       uint16_t srclen = pack->src.blklen - pack->src.blkpos;

       /* Transfer the smaller of the two amounts data */

       uint16_t xfrlen = MIN(srclen, destlen);
       nxffs_ioseek(volume, pack->src.blkoffset + pack->src.blkpos);
       memcpy(&pack->iobuffer[pack->iooffset], &volume->cache[volume->iooffset], xfrlen);

       /* Now, either the (1) src block has been fully transferred, (2) all
        * of the source data has been transferred, of (3) the the destination
        * block is full, .. or all three.
        *
        * Check if all of the bytes in the source inode have been transferred.
        */

       pack->src.fpos   += xfrlen;
       pack->src.blkpos += xfrlen;

       if (pack->src.fpos >= pack->src.entry.datlen)
         {
           /* Write the final destination data block header and inode
            * headers.
            */

           nxffs_wrdathdr(volume, pack);
           nxffs_wrinodehdr(volume,pack);

           /* Find the next valid source inode */

           offset = pack->src.blkoffset + pack->src.blklen;
           ret = nxffs_nextentry(volume, offset, &pack->src.entry);
           if (ret < 0)
             {
               /* No more valid inode entries.  Just return an end-of-flash error
                * indication.
                */

               return -ENOSPC;
             }

           /* Setup the new source stream */

           ret = nxffs_srcsetup(volume, pack, pack->src.entry.doffset);
           if (ret < 0)
             {
               return ret;
             }

           /* Setup the dest stream */

           pack->dest.entry.name    = pack->src.entry.name;
           pack->dest.entry.utc     = pack->src.entry.utc;
           pack->dest.entry.datlen  = pack->src.entry.datlen;
           pack->src.entry.name     = NULL;

           /* Is there sufficient space at the end of the I/O block to hold
            * the inode header?
            */

           if (pack->iooffset + SIZEOF_NXFFS_INODE_HDR > volume->geo.blocksize)
             {
               /* No, just return success... we will handle this condition when
                * this function is called on the next I/O block.
                */

               return OK;
             }
 
           pack->dest.entry.hoffset = nxffs_packtell(volume, pack);
           ret = nxffs_destsetup(volume, pack);
           if (ret < 0)
             {
               return ret;
             }
         }

      /* Not at the end of the source data stream.  Check if we are at the
       * end of the current data block.
       */

      else if (pack->src.blkpos >= pack->src.blklen)
        {
          struct nxffs_blkentry_s blkentry;

          /* Yes.. find the next data block in the source input stream. */

          ret = nxffs_nextblock(volume, offset, &blkentry);
          if (ret < 0)
            {
              fdbg("Failed to find next data block: %d\n", -ret);
              return ret;
            }

          /* Set up the source stream */

          pack->src.blkoffset = blkentry.hoffset;
          pack->src.blklen    = blkentry.datlen;
          pack->src.blkpos    = 0;
        }

     /* Check if the destination block is full */

     pack->dest.fpos   += xfrlen;
     pack->dest.blkpos += xfrlen;
     pack->iooffset    += xfrlen;

     if (pack->iooffset >= volume->geo.blocksize)
       {
         /* Yes.. Write the destination data block header and return success */

         nxffs_wrdathdr(volume, pack);
         return OK;
       }
    }

  return -ENOSYS;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxffs_pack
 *
 * Description:
 *   Pack and re-write the filesystem in order to free up memory at the end
 *   of FLASH.
 *
 * Input Parameters:
 *   volume - The volume to be packed.
 *
 * Returned Values:
 *   Zero on success; Otherwise, a negated errno value is returned to
 *   indicate the nature of the failure.
 *
 ****************************************************************************/

int nxffs_pack(FAR struct nxffs_volume_s *volume)
{
  struct nxffs_pack_s pack;
  off_t iooffset;
  off_t eblock;
  off_t block;
  int i;
  int ret;

  /* Get the offset to the first valid inode entry */

  iooffset = nxffs_mediacheck(volume, &pack);
  if (iooffset == 0)
    {
      /* Offset zero is only returned if no valid blocks were found on the
       * FLASH media or if there are no valid inode entries on the FLASH after
       * the first valid block.  In this case, the media needs to be re-
       * formatted.
       */

      return nxffs_reformat(volume);
    }

  /* There is a valid format and valid inodes on the media.. setup up to
   * begin the packing operation.
   */

  ret = nxffs_startpos(volume, &pack, iooffset);
  if (ret < 0)
    {
      /* This is a normal situation if the volume is full */

      if (ret == -ENOSPC)
        {
          return OK;
        }
      else
        {
          fvdbg("Failed to find a packing position: %d\n", -ret);
          return ret;
        }
    }

  /* Otherwise, begin pack at this src/dest block combination.  Initialize
   * ioblock and iooffset with the position of the first inode header.
   */

  pack.ioblock  = nxffs_getblock(volume, pack.dest.entry.hoffset);
  pack.iooffset = nxffs_getoffset(volume, pack.dest.entry.hoffset, pack.ioblock);

  /* Reserve space for the inode header.  Note we are guaranteed by
   * nxffs_startpos() that the inode header will fit at hoffset.
   */

  pack.iooffset += SIZEOF_NXFFS_INODE_HDR;

  /* Then pack all erase blocks starting with the erase block that contains
   * the ioblock and through the final erase block on the FLASH.
   */

  for (eblock = pack.ioblock / volume->blkper;
       eblock < volume->geo.neraseblocks;
       eblock++)
    {
      /* Read the erase block into the pack buffer. */

      pack.block0 = eblock * volume->blkper;
      ret = MTD_BREAD(volume->mtd, pack.block0, volume->blkper, volume->pack);
      if (ret < 0)
        {
          fdbg("Failed to read erase block %d: %d\n", eblock, -ret);
          goto errout_with_pack;
        }

      /* Pack each I/O block */

      for (i = 0, block = pack.block0, pack.iobuffer = volume->pack;
           i < volume->blkper;
           i++, block++, pack.iobuffer += volume->geo.blocksize)
        {
           /* The first time here, the ioblock may point to an offset into
            * the erase block.  We just need to skip over those cases.
            */

           if (block >= pack.ioblock)
              {
                /* Set the I/O position.  Note on the first time we get
                 * pack.iooffset will hold the offset in the first I/O block
                 * to the first inode header.
                 */

                pack.ioblock = block;

                /* Check if this is a valid block (it will be valid for the
                 * first block.
                 */

                if (nxffs_packvalid(&pack))
                  {
                     /* Yes.. pack data into this block */

                     ret = nxffs_packblock(volume, &pack);
                     if (ret < 0)
                       {
                         fdbg("Failed to pack into block %d: %d\n",
                              block, ret);
                         goto errout_with_pack;
                       }
                   }

                 /* Next time we get here, pack.iooffset will point to the
                  * first byte after the block header.
                  */

                 pack.iooffset = SIZEOF_NXFFS_BLOCK_HDR;
              }
         }

      /* Write the packed I/O block to FLASH */

      ret = MTD_BWRITE(volume->mtd, pack.block0, volume->blkper, volume->pack);
      if (ret < 0)
        {
          fdbg("Failed to write erase block %d: %d\n", eblock, -ret);
          goto errout_with_pack;
        }
    }

errout_with_pack:
  nxffs_freeentry(&pack.src.entry);
  nxffs_freeentry(&pack.dest.entry);
  return ret;
}
