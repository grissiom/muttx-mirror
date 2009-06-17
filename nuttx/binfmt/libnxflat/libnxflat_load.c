/****************************************************************************
 * binfmt/libnxflat/libnxflat_load.c
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <sys/types.h>
#include <sys/mman.h>

#include <stdlib.h>
#include <string.h>
#include <nxflat.h>
#include <debug.h>
#include <errno.h>

#include <arpa/inet.h>
#include <nuttx/nxflat.h>

/****************************************************************************
 * Pre-Processor Definitions
 ****************************************************************************/

#ifndef MAX
#define MAX(x,y) ((x) > (y) ? (x) : (y))
#endif

/****************************************************************************
 * Private Constant Data
 ****************************************************************************/

#if defined(CONFIG_DEBUG_VERBOSE) && defined(CONFIG_DEBUG_BINFMT)
static const char g_textsegment[] = "TEXT";
static const char g_datasegment[] = "DATA";
static const char g_bsssegment[]  = "BSS";
static const char g_unksegment[]  = "UNKNOWN";

static const char *g_segment[] =
{
  g_textsegment,
  g_datasegment,
  g_bsssegment,
  g_unksegment
};
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxflat_reloc
 ****************************************************************************/

static void nxflat_reloc(struct nxflat_loadinfo_s *loadinfo, uint32 rl)
{
  union
  {
    uint32 l;
    struct nxflat_reloc_s s;
  } reloc;
  uint32 *ptr;
  uint32 datastart;

  /* Force the long value into a union so that we can strip off some
   * bit-encoded values.
   */

  reloc.l = rl;

  /* We only support relocations in the data sections.
   * Verify that the the relocation address lies in the data
   * section of the file image.
   */

  if (reloc.s.r_offset > loadinfo->data_size)
    {
      bdbg("ERROR: Relocation at 0x%08x invalid -- "
	  "does not lie in the data segment, size=0x%08x\n",
	  reloc.s.r_offset, loadinfo->data_size);
      bdbg("       Relocation not performed!\n");
    }
  else if ((reloc.s.r_offset & 0x00000003) != 0)
    {
      bdbg("ERROR: Relocation at 0x%08x invalid -- "
	  "Improperly aligned\n",
	  reloc.s.r_offset);
    }
  else
    {
      /* Get a reference to the "real" start of data.  It is
       * offset slightly from the beginning of the allocated
       * DSpace to hold information needed by ld.so at run time.
       */

      datastart = loadinfo->dspace + NXFLAT_DATA_OFFSET;

      /* Get a pointer to the value that needs relocation in
       * DSpace.
       */
      
      ptr = (uint32*)(datastart + reloc.s.r_offset);

      bvdbg("Relocation of variable at DATASEG+0x%08x "
	  "(address 0x%p, currently 0x%08x) into segment %s\n",
	  reloc.s.r_offset, ptr, *ptr, g_segment[reloc.s.r_type]);
	
      switch (reloc.s.r_type)
	{
	  /* TEXT is located at an offset of sizeof(struct nxflat_hdr_s) from
	   * the allocated/mapped ISpace region.
	   */

	case NXFLAT_RELOC_TYPE_TEXT:
	  *ptr += loadinfo->ispace + sizeof(struct nxflat_hdr_s);
	  break;

	  /* DATA and BSS are always contiguous regions.  DATA
	   * begins at an offset of NXFLAT_DATA_OFFSET from
	   * the beginning of the allocated data segment.
	   * BSS is positioned after DATA, unrelocated references
	   * to BSS include the data offset.
	   *
	   * In other contexts, is it necessary to add the data_size
	   * to get the BSS offset like:
	   *
	   *   *ptr += datastart + loadinfo->data_size;
	   */

	case NXFLAT_RELOC_TYPE_DATA:
	case NXFLAT_RELOC_TYPE_BSS:
	  *ptr += datastart;
	  break;

	  /* This case happens normally if the symbol is a weak
	   * undefined symbol.  We permit these.
	   */

	case NXFLAT_RELOC_TYPE_NONE:
	  bdbg("NULL relocation!\n");
	  break;

	default:
	  bdbg("ERROR: Unknown relocation type=%d\n", reloc.s.r_type);
	  break;
	}

      bvdbg("Relocation became 0x%08x\n", *ptr);
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxflat_load
 ****************************************************************************/

int nxflat_load(struct nxflat_loadinfo_s *loadinfo)
{
  off_t   doffset;     /* Offset to .data in the NXFLAT file */
  uint32 *reloctab;    /* Address of the relocation table */
  uint32  dreadsize;   /* Total number of bytes of .data to be read */
  uint32  ret;
  int     i;

  /* Calculate the extra space we need to map in.  This region will be the
   * BSS segment and the stack.  It will also be used temporarily to hold
   * relocation information.  So the size of this region will either be the
   * size of the BSS section and the stack OR, it the size of the relocation
   * entries, whichever is larger
   */

  {
    uint32 relocsize;
    uint32 extrasize;

    /* This is the amount of memory that we have to have to hold the
     * relocations.
     */

    relocsize  = loadinfo->reloc_count * sizeof(uint32);

    /* In the file, the relocations should lie at the same offset as BSS.
     * The additional amount that we allocate have to be either (1) the
     * BSS size + the stack size, or (2) the size of the relocation records,
     * whicher is larger.
     */

    extrasize = MAX(loadinfo->bss_size + loadinfo->stack_size, relocsize);

    /* Use this addtional amount to adjust the total size of the dspace
     * region.
     */

    loadinfo->dspace_size =
      NXFLAT_DATA_OFFSET +      /* Memory used by ldso */
      loadinfo->data_size +    /* Initialized data */
      extrasize;                /* bss+stack/relocs */

    /* The number of bytes of data that we have to read from the file is
     * the data size plus the size of the relocation table.
     */

    dreadsize = loadinfo->data_size + relocsize;
  }

  /* We'll need this a few times as well. */

  doffset = loadinfo->ispace_size;

  /* We will make two mmap calls create an address space for the executable.
   * We will attempt to map the file to get the ISpace address space and
   * to allocate RAM to get the DSpace address space.  If the filesystem does
   * not support file mapping, the map() implementation should do the
   * right thing.
   */

  /* The following call will give as a pointer to the mapped file ISpace.
   * This may be in ROM, RAM, Flash, ... We don't really care where the memory
   * resides as long as it is fully initialized and ready to execute.
   */

  loadinfo->ispace = (uint32)mmap(NULL, loadinfo->ispace_size, PROT_READ,
                                  MAP_SHARED|MAP_FILE, loadinfo->filfd, 0);
  if (loadinfo->ispace == (uint32)MAP_FAILED)
    {
      bdbg("Failed to map NXFLAT ISpace: %d\n", errno);
      return -errno;
    }

  bvdbg("Mapped ISpace (%d bytes) at 0x%08x\n",
      loadinfo->ispace_size, loadinfo->ispace);

  /* The following call will give a pointer to the allocated but
   * uninitialized ISpace memory.
   */

  loadinfo->dspace = (uint32)malloc(loadinfo->dspace_size);
  if (loadinfo->dspace == 0)
    {
      bdbg("Failed to allocate DSpace\n");
      ret = -ENOMEM;
      goto errout;
    }

  bvdbg("Allocated DSpace (%d bytes) at %08x\n",
      loadinfo->dspace_size, loadinfo->dspace);

  /* Now, read the data into allocated DSpace at doffset into the
   * allocated DSpace memory.
   */

  ret = nxflat_read(loadinfo, (char*)(loadinfo->dspace + NXFLAT_DATA_OFFSET), dreadsize, doffset);
  if (ret < 0)
    {
      bdbg("Failed to read .data section: %d\n", ret);
      goto errout;
    }
       
  /* Save information about the allocation. */

  loadinfo->alloc_start = loadinfo->dspace;
  loadinfo->alloc_size  = loadinfo->dspace_size;

  bvdbg("TEXT=0x%x Entry point offset=0x%08x, datastart is 0x%08x\n",
      loadinfo->ispace, loadinfo->entry_offset, doffset);

  /* Resolve the address of the relocation table.  In the file, the
   * relocations should lie at the same offset as BSS.  The current
   * value of reloc_start is the offset from the beginning of the file.
   * The following adjustment will convert it to an address in DSpace.
   */

  reloctab = (uint32*)
    (loadinfo->reloc_start     /* File offset to reloc records */
     + loadinfo->dspace        /* + Allocated DSpace memory */
     + NXFLAT_DATA_OFFSET        /* + Offset for ldso usage */
     - loadinfo->ispace_size); /* - File offset to DSpace */

  bvdbg("Relocation table at 0x%p, reloc_count=%d\n",
      reloctab, loadinfo->reloc_count);

  /* Now run through the relocation entries. */

  for (i=0; i < loadinfo->reloc_count; i++)
    {
      nxflat_reloc(loadinfo, htonl(reloctab[i]));
    }

  /* Zero the BSS, BRK and stack areas, trashing the relocations
   * that lived in the corresponding space in the file. */

  memset((void*)(loadinfo->dspace + NXFLAT_DATA_OFFSET + loadinfo->data_size),
	       0,
	       (loadinfo->dspace_size - NXFLAT_DATA_OFFSET -
		loadinfo->data_size));

  return OK;

errout:
  (void)nxflat_unload(loadinfo);
  return ret;
}

