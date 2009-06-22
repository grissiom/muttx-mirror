/****************************************************************************
 * binfmt/libnxflat/libnxflat_bind.c
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

#include <string.h>
#include <nxflat.h>
#include <errno.h>
#include <assert.h>
#include <debug.h>

#include <arpa/inet.h>
#include <nuttx/nxflat.h>
#include <nuttx/symtab.h>

/****************************************************************************
 * Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxflat_bindrel32i
 *
 * Description:
 *   Perform the NXFLAT_RELOC_TYPE_REL32I binding:
 *
 *   Meaning: Object file contains a 32-bit offset into I-Space at the the offset.
 *   Fixup:   Add mapped I-Space address to the offset.
 *
 * Returned Value:
 *   0 (OK) is returned on success and a negated errno is returned on
 *   failure.
 *
 ****************************************************************************/

static inline int nxflat_bindrel32i(FAR struct nxflat_loadinfo_s *loadinfo,
                                      uint32 offset)
{
  uint32 *addr;

  bvdbg("NXFLAT_RELOC_TYPE_REL32I Offset: %08x I-Space: %p\n",
        offset, loadinfo->ispace);

  if (offset < loadinfo->dsize)
    {
      addr = (uint32*)(offset + loadinfo->dspace->region);
      bvdbg("  Before: %08x\n", *addr);
     *addr += (uint32)(loadinfo->ispace);
      bvdbg("  After: %08x\n", *addr);
      return OK;
    }
  else
    {
      bdbg("Offset: %08 does not lie in D-Space size: %08x\n",
           offset, loadinfo->dsize);
      return -EINVAL;
    }
}

/****************************************************************************
 * Name: nxflat_bindrel32d
 *
 * Description:
 *   Perform the NXFLAT_RELOC_TYPE_REL32D binding:
 *
 *   Meaning: Object file contains a 32-bit offset into D-Space at the the offset.
 *   Fixup:   Add allocated D-Space address to the offset.
 *
 * Returned Value:
 *   0 (OK) is returned on success and a negated errno is returned on
 *   failure.
 *
 ****************************************************************************/

static inline int nxflat_bindrel32d(FAR struct nxflat_loadinfo_s *loadinfo,
                                      uint32 offset)
{
  uint32 *addr;

  bvdbg("NXFLAT_RELOC_TYPE_REL32D Offset: %08x D-Space: %p\n",
        offset, loadinfo->dspace->region);

  if (offset < loadinfo->dsize)
    {
      addr = (uint32*)(offset + loadinfo->dspace->region);
      bvdbg("  Before: %08x\n", *addr);
     *addr += (uint32)(loadinfo->dspace->region);
      bvdbg("  After: %08x\n", *addr);
      return OK;
    }
  else
    {
      bdbg("Offset: %08 does not lie in D-Space size: %08x\n",
           offset, loadinfo->dsize);
      return -EINVAL;
    }
}

/****************************************************************************
 * Name: nxflat_gotrelocs
 *
 * Description:
 *   Bind all of the GOT relocations in the loaded module described by
 *   'loadinfo'
 *
 * Returned Value:
 *   0 (OK) is returned on success and a negated errno is returned on
 *   failure.
 *
 ****************************************************************************/

static inline int nxflat_gotrelocs(FAR struct nxflat_loadinfo_s *loadinfo)
{
  FAR struct nxflat_reloc_s *relocs;
  FAR struct nxflat_reloc_s  reloc;
  FAR struct nxflat_hdr_s   *hdr;
  uint32  offset;
  uint16  nrelocs;
  int     ret;
  int     result;
  int     i;

  /* The NXFLAT header is the first thing at the beginning of the ISpace. */

  hdr = (FAR struct nxflat_hdr_s*)loadinfo->ispace;

  /* From this, we can get the offset to the list of relocation entries */

  offset  = ntohl(hdr->h_relocstart);
  nrelocs = ntohs(hdr->h_reloccount);

  /* The value of the relocation list that we get from the header is a
   * file offset.  We will have to convert this to an offset into the
   * DSpace segment to get the pointer to the beginning of the relocation
   * list.
   */

  DEBUGASSERT(offset >= loadinfo->isize && offset < (loadinfo->isize + loadinfo->dsize));
  relocs = (FAR struct nxflat_reloc_s*)(offset - loadinfo->isize + loadinfo->dspace->region);

  /* Now, traverse the relocation list of imported symbols and attempt to bind
   * each GOT relocation (imported symbols will be handled elsewhere).
   */

  ret = OK; /* Assume success */
  for (i = 0; i < nrelocs; i++)
    {
      /* Handle the relocation by the relocation type */

      reloc = *relocs++;
      result = OK;
      switch (NXFLAT_RELOC_TYPE(reloc.r_info))
        {

        /* NXFLAT_RELOC_TYPE_REL32I  Meaning: Object file contains a 32-bit offset
         *                                    into I-Space at the the offset.
         *                           Fixup:   Add mapped I-Space address to the offset.
         */

        case NXFLAT_RELOC_TYPE_REL32I:
          {
            result = nxflat_bindrel32i(loadinfo, NXFLAT_RELOC_OFFSET(reloc.r_info));
          }
          break;

        /* NXFLAT_RELOC_TYPE_REL32D  Meaning: Object file contains a 32-bit offset
         *                                    into D-Space at the the offset.
         *                           Fixup:   Add allocated D-Space address to the
         *                                    offset.
         */

        case NXFLAT_RELOC_TYPE_REL32D:
          {
            result = nxflat_bindrel32d(loadinfo, NXFLAT_RELOC_OFFSET(reloc.r_info));
          }
          break;

        /* NXFLAT_RELOC_TYPE_ABS32   Meaning: Offset refers to a struct nxflat_import_s
         *                                    describing a function pointer to be
         *                                    imported.
         *                           Fixup:   Provide the absolute function address
         *                                    in the struct nxflat_import_s instance.
         */

        case NXFLAT_RELOC_TYPE_ABS32:
          {
            /* These will be handled together in nxflat_bindimports */
          }
          break;
        }

      /* Check for failures */

      if (result < 0 && ret == OK)
        {
          ret = result;
        }
    }

  return ret;
}

/****************************************************************************
 * Name: nxflat_bindimports
 *
 * Description:
 *   Bind the imported symbol names in the loaded module described by
 *   'loadinfo' using the exported symbol values provided by 'symtab'
 *
 * Returned Value:
 *   0 (OK) is returned on success and a negated errno is returned on
 *   failure.
 *
 ****************************************************************************/

static inline int nxflat_bindimports(FAR struct nxflat_loadinfo_s *loadinfo,
                                       FAR const struct symtab_s *exports,
                                       int nexports)
{
  FAR struct nxflat_import_s *imports;
  FAR struct nxflat_hdr_s    *hdr;
  FAR const struct symtab_s *symbol;

  char   *symname;
  uint32  offset;
  uint16  nimports;
  int     i;

  /* The NXFLAT header is the first thing at the beginning of the ISpace. */

  hdr = (FAR struct nxflat_hdr_s*)loadinfo->ispace;

  /* From this, we can get the offset to the list of symbols imported by
   * this module and the number of symbols imported by this module.
   */

  offset   = ntohl(hdr->h_importsymbols);
  nimports = ntohs(hdr->h_importcount);

  /* Verify that this module requires imported symbols */

  if (offset != 0 && nimports > 0)
    {
      /* It does.. make sure that exported symbols are provided */

      DEBUGASSERT(symtab && nexports > 0);

      /* If non-zero, the value of the imported symbol list that we get
       * from the header is a file offset.  We will have to convert this
       * to an offset into the DSpace segment to get the pointer to the
       * beginning of the imported symbol list.
       */

      DEBUGASSERT(offset >= loadinfo->isize &&
                  offset < loadinfo->isize + loadinfo->dsize);

      imports = (struct nxflat_import_s*)
	(offset - loadinfo->isize + loadinfo->dspace->region);

      /* Now, traverse the list of imported symbols and attempt to bind
       * each symbol to the value exported by from the exported symbol
       * table.
       */

      for (i = 0; i < nimports; i++)
	{
	  /* Get a pointer to the imported symbol name.  The name itself
	   * lies in the TEXT segment.  But the reference to the name
	   * lies in DATA segment.  Therefore, the name reference should
	   * have been relocated when the module was loaded.
	   */

          offset = imports[i].i_funcname;
          DEBUGASSERT(offset < loadinfo->isize);

	  symname = (char*)(offset + loadinfo->ispace);

	  /* Find the exported symbol value for this this symbol name. */

#ifdef CONFIG_SYMTAB_ORDEREDBYNAME
          symbol = symtab_findorderedbyname(exports, symname, nexports);
#else
          symbol = symtab_findbyname(exports, symname, nexports);
#endif
	  if (!symbol)
	    {
	      bdbg("Exported symbol \"%s\" not found\n", symname);
              return -ENOENT;
	    }

	  /* And put this into the module's import structure. */

	  imports[i].i_funcaddress =  (uint32)symbol->sym_value;

	  bvdbg("Bound imported function '%s' to address %08x\n",
	        symname, imports[i].function_address);
	}
    }

  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxflat_bind
 *
 * Description:
 *   Bind the imported symbol names in the loaded module described by
 *   'loadinfo' using the exported symbol values provided by 'symtab'.
 *   After binding the module, clear the BSS region (which held the relocation
 *   data) in preparation for execution.
 *
 * Returned Value:
 *   0 (OK) is returned on success and a negated errno is returned on
 *   failure.
 *
 ****************************************************************************/

int nxflat_bind(FAR struct nxflat_loadinfo_s *loadinfo,
                FAR const struct symtab_s *exports, int nexports)
{
  /* First bind all GOT relocations (omitting absolute symbol relocations) */

  int ret = nxflat_gotrelocs(loadinfo);
  if (ret == OK)
    {
      /* Then bind the imported symbol, absolute relocations separately.
       * There is no particular reason to do these separately over than
       * traversing the import list directly is simpler than traversing
       * it indirectly through the relocation list.
       */

      ret = nxflat_bindimports(loadinfo, exports, nexports);
      if (ret == OK)
        {
          /* Zero the BSS area, trashing the relocations that lived in space
           * in the file.
           */

          memset((void*)(loadinfo->dspace->region + loadinfo->datasize),
	                  0, loadinfo->bsssize);
        }
    }
  return ret;
}

