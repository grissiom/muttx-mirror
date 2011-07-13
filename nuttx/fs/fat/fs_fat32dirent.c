/****************************************************************************
 * fs/fat/fs_fat32dirent.c
 *
 *   Copyright (C) 2011 Gregory Nutt. All rights reserved.
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
 * NOTE:  If CONFIG_FAT_LFN is defined, then there may be some legal, patent
 * issues. The following was extracted from the entry "File Allocation Table
 * from Wikipedia, the free encyclopedia:
 *
 * "On December 3, 2003 Microsoft announced it would be offering licenses
 *  for use of its FAT specification and 'associated intellectual property',
 *  at the cost of a US$0.25 royalty per unit sold, with a $250,000 maximum
 *  royalty per license agreement.
 *
 *  o "U.S. Patent 5,745,902 (http://www.google.com/patents?vid=5745902) -
 *     Method and system for accessing a file using file names having
 *     different file name formats. ...
 *  o "U.S. Patent 5,579,517 (http://www.google.com/patents?vid=5579517) -
 *     Common name space for long and short filenames. ...
 *  o "U.S. Patent 5,758,352 (http://www.google.com/patents?vid=5758352) -
 *     Common name space for long and short filenames. ...
 *  o "U.S. Patent 6,286,013 (http://www.google.com/patents?vid=6286013) -
 *     Method and system for providing a common name space for long and
 *     short file names in an operating system. ...
 *
 * "Many technical commentators have concluded that these patents only cover
 *  FAT implementations that include support for long filenames, and that
 *  removable solid state media and consumer devices only using short names
 *  would be unaffected. ..."
 *
 * So you have been forewarned:  Use the long filename at your own risk!
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/types.h>

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <errno.h>
#include <debug.h>

#include <nuttx/fs.h>
#include <nuttx/fat.h>

#include "fs_internal.h"
#include "fs_fat32.h"

/****************************************************************************
 * Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Types
 ****************************************************************************/

enum fat_case_e
{
  FATCASE_UNKNOWN = 0,
  FATCASE_UPPER,
  FATCASE_LOWER
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Private Variables
 ****************************************************************************/

/****************************************************************************
 * Public Variables
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: fat_parsesfname
 *
 * Desciption:  Convert a user filename into a properly formatted FAT
 *   (short 8.3) filename as it would appear in a directory entry.  Here are
 *    the rules for the 8+3 short file name in the directory:
 *
 *   The first byte:
 *
 *     0xe5 = The directory is free
 *     0x00 = This directory and all following directories are free
 *     0x05 = Really 0xe5
 *     0x20 = May NOT be ' '
 *
 *   Other characters may be any characters except for the following:
 *
 *     0x00-0x1f = (except for 0x00 and 0x05 in the first byte)
 *     0x22      = '"'
 *     0x2a-0x2c = '*', '+', ','
 *     0x2e-0x2f = '.', '/'
 *     0x3a-0x3f = ':', ';', '<', '=', '>', '?'
 *     0x5b-0x5d = '[', '\\', ;]'
 *     0x7c      = '|'
 *
 *   '.' May only occur once within string and only within the first 9
 *   bytes.  The '.' is not save in the directory, but is implicit in
 *   8+3 format.
 *
 *   Lower case characters are not allowed in directory names (without some
 *   poorly documented operations on the NTRes directory byte).  Lower case
 *   codes may represent different characters in other character sets ("DOS
 *   code pages".  The logic below does not, at present, support any other
 *   character sets.
 *
 * Returned value:
 *   OK - The path refers to a valid 8.3 FAT file name and has been properly
 *        converted and stored in dirinfo.
 *   <0 - Otherwise an negated error is returned meaning that the string is
 *        not a valid 8+3 because:
 *
 *        1. Contains characters not in the printable character set,
 *        2. Contains forbidden characters or multiple '.' characters
 *        3. File name or extension is too long.
 *
 *        If CONFIG_FAT_LFN is defined and CONFIG_FAT_LCNAMES is NOT
 *        defined, then:
 *
 *        4a. File name or extension contains lower case characters.
 *
 *        If CONFIG_FAT_LFN is defined and CONFIG_FAT_LCNAMES is defined,
 *        then:
 *
 *        4b. File name or extension is not all the same case.
 *
 ****************************************************************************/

static inline int fat_parsesfname(const char **path,
                                   struct fat_dirinfo_s *dirinfo,
                                   char *terminator)
{
#ifdef CONFIG_FAT_LCNAMES
  unsigned int ntlcenable = FATNTRES_LCNAME | FATNTRES_LCEXT;
  unsigned int ntlcfound  = 0;
#ifdef CONFIG_FAT_LFN
  enum fat_case_e namecase = FATCASE_UNKNOWN;
  enum fat_case_e extcase  = FATCASE_UNKNOWN;
#endif
#endif
  const char *node = *path;
  int endndx;
  uint8_t ch;
  int ndx = 0;

  /* Initialized the name with all spaces */

  memset(dirinfo->fd_name, ' ', DIR_MAXFNAME);
 
  /* Loop until the name is successfully parsed or an error occurs */

  endndx  = 8;
  for (;;)
    {
      /* Get the next byte from the path */

      ch = *node++;

      /* Check if this the last byte in this node of the name */

      if ((ch == '\0' || ch == '/') && ndx != 0 )
        {
          /* Return the accumulated NT flags and the terminating character */

#ifdef CONFIG_FAT_LCNAMES
          dirinfo->fd_ntflags = ntlcfound & ntlcenable;
#endif
          *terminator = ch;
          *path       = node;
          return OK;
        }

      /* Accept only the printable character set.  Note the first byte
       * of the name could be 0x05 meaning that is it 0xe5, but this is
       * not a printable character in this character in either case.
       */

      else if (!isgraph(ch))
        {
          goto errout;
        }

      /* Check for transition from name to extension.  Only one '.' is
       * permitted and it must be within first 9 characters
       */

      else if (ch == '.' && endndx == 8)
        {
          /* Starting the extension */

          ndx    = 8;
          endndx = 11;
          continue;
        }

      /* Reject printable characters forbidden by FAT */

      else if (ch == '"'  ||  (ch >= '*' && ch <= ',') ||
               ch == '.'  ||   ch == '/'               ||
              (ch >= ':'  &&   ch <= '?')              ||
              (ch >= '['  &&   ch <= ']')              ||
              (ch == '|'))
        {
          goto errout;
        }

      /* Check for upper case characters */

#ifdef CONFIG_FAT_LCNAMES
      else if (isupper(ch))
        {
          /* Some or all of the characters in the name or extension
           * are upper case. Force all of the characters to be interpreted
           * as upper case.
           */

          if (endndx == 8)
            {
              /* Is there mixed case in the name? */

#ifdef CONFIG_FAT_LFN
              if (namecase == FATCASE_LOWER)
                {
                  /* Mixed case in the name -- use the long file name */

                  goto errout;
                }

              /* So far, only upper case in the name*/

              namecase = FATCASE_UPPER;
#endif

              /* Clear lower case name bit in mask*/

              ntlcenable &= FATNTRES_LCNAME;
            }
          else
            {
              /* Is there mixed case in the extension? */

#ifdef CONFIG_FAT_LFN
              if (extcase == FATCASE_LOWER)
                {
                  /* Mixed case in the extension -- use the long file name */

                  goto errout;
                }

              /* So far, only upper case in the extension*/

              extcase = FATCASE_UPPER;
#endif

              /* Clear lower case extension in mask */

              ntlcenable &= FATNTRES_LCNAME;
            }
        }
#endif

      /* Check for lower case characters */

      else if (islower(ch))
        {
#if defined(CONFIG_FAT_LFN) && !defined(CONFIG_FAT_LCNAMES)
          /* If lower case characters are present, then a long file
           * name will be constructed.
           */

          goto errout;
#else
          /* Convert the character to upper case */

          ch = toupper(ch);

          /* Some or all of the characters in the name or extension
           * are lower case.  They can be interpreted as lower case if
           * only if all of the characters in the name or extension are
           * lower case.
           */

#ifdef CONFIG_FAT_LCNAMES
          if (endndx == 8)
            {
                /* Is there mixed case in the name? */

#ifdef CONFIG_FAT_LFN
                if (namecase == FATCASE_UPPER)
                  {
                    /* Mixed case in the name -- use the long file name */

                    goto errout;
                  }

                /* So far, only lower case in the name*/

                namecase = FATCASE_LOWER;
#endif

              /* Set lower case name bit */

              ntlcfound |= FATNTRES_LCNAME;
            }
          else
            {
              /* Is there mixed case in the extension? */

#ifdef CONFIG_FAT_LFN
              if (extcase == FATCASE_UPPER)
                {
                  /* Mixed case in the extension -- use the long file name */

                  goto errout;
                }

              /* So far, only lower case in the extension*/

              extcase = FATCASE_LOWER;
#endif

              /* Set lower case extension bit */

              ntlcfound |= FATNTRES_LCNAME;
            }
#endif
#endif /* CONFIG_FAT_LFN && !CONFIG_FAT_LCNAMES */
        }

      /* Check if the file name exceeds the size permitted (without
       * long file name support).
       */

      if (ndx >= endndx)
        {
          goto errout;
        }

      /* Save next character in the accumulated name */

      dirinfo->fd_name[ndx++] = ch;
    }

 errout:
  return -EINVAL;
}

/****************************************************************************
 * Name: fat_parselfname
 *
 * Desciption:  Convert a user filename into a properly formatted FAT
 *   long filename as it would appear in a directory entry.  Here are
 *   the rules for the long file name in the directory:
 *
 *   Valid characters are the same as for short file names EXCEPT:
 *
 *     1. '+', ',', ';', '=', '[', and ']' are accepted in the file name
 *     2. '.' (dot) can occur more than once in a filename. Extension is
 *        the substring after the last dot.
 *
 * Returned value:
 *   OK - The path refers to a valid long file name and has been properly
 *        stored in dirinfo.
 *   <0 - Otherwise an negated error is returned meaning that the string is
 *        not a valid long file name:
 *
 *        1. Contains characters not in the printable character set,
 *        2. Contains forbidden characters
 *        3. File name is too long.
 *
 ****************************************************************************/

#ifdef CONFIG_FAT_LFN
static inline int fat_parselfname(const char **path,
                                   struct fat_dirinfo_s *dirinfo,
                                   char *terminator)
{
  const char *node = *path;
  uint8_t ch;
  int ndx = 0;

  /* Loop until the name is successfully parsed or an error occurs */

  for (;;)
    {
      /* Get the next byte from the path */

      ch = *node++;

      /* Check if this the last byte in this node of the name */

      if ((ch == '\0' || ch == '/') && ndx != 0 )
        {
          /* Null terminate the string */

          dirinfo->fd_lfname[ndx] = '\0';

          /* Return the remaining sub-string and the terminating character. */

          *terminator = ch;
          *path       = node;
          return OK;
        }

      /* Accept only the printable character set */

      else if (!isgraph(ch))
        {
          goto errout;
        }

      /* Reject printable characters forbidden by FAT */

      else if (ch == '"' || ch == '*' || ch == '/' || ch == ':'  ||
               ch == '<' || ch == '>' || ch == '?' || ch == '\\' ||
               ch == '|')
        {
          goto errout;
        }

      /* Check if the file name exceeds the size permitted. */

      if (ndx >= LDIR_MAXFNAME)
        {
          goto errout;
        }

      /* Save next character in the accumulated name */

      dirinfo->fd_lfname[ndx++] = ch;
    }

 errout:
    dirinfo->fd_lfname[0] = '\0';
    return -EINVAL;
}
#endif

/****************************************************************************
 * Name: fat_createalias
 *
 * Desciption:  Given a valid long file name, create a short filename alias.
 *   Here are the rules for creation of the alias:
 *
 *   1. All uppercase
 *   2. All dots except the last deleted
 *   3. First 6 (uppercase) characters used as a base
 *   4. Then ~1.  The number is increased if the file already exists in the
 *      directory. If the number exeeds >10, then character stripped off the
 *       base.
 *   5. The extension is the first 3 uppercase chars of extension.
 *
 * Returned value:
 *   OK - The alias was created correctly.
 *   <0 - Otherwise an negated error is returned.
 *
 ****************************************************************************/

#ifdef CONFIG_FAT_LFN
static inline int fat_createalias(const char **path,
                                  struct fat_dirinfo_s *dirinfo)
{
  uint8_t ch;        /* Current character being processed */
  char   *ext;       /* Pointer to the extension substring */
  char   *ptr;       /* Working pointer */
  int     len;       /* Total length of the long file name */
  int     namechars; /* Number of characters available in long name */
  int     extchars;  /* Number of characters available in long name extension */
  int     endndx;    /* Maximum index into the short name array */
  int     ndx;       /* Index to store next character */

  /* First, let's decide what is name and what is extension */

  len = strlen(dirinfo.fd_lfname);
  ext = strrchr(dirinfo.fd_lfname, '.');
  if (ext)
    {
      ptrdiff_t tmp;

      /* ext points to the final '.'.  The difference in bytes from the
       * beginning of the string is then the name length.
       */

      tmp       = ext - dirinfo.fd_lfname;
      namechars = tmp;

      /* And the rest, exluding the '.' is the extension. */

      extchars  = len - namechars - 1;
      ext++;
    }
  else
    {
      /* No '.' found.  It is all name and no extension. */

      namechars = len;
      extchars  = 0;
    }

  /* Alias are always all upper case */

#ifdef CONFIG_FAT_LCNAMES
  dirinfo->fd_ntflags = 0;
#endif

  /* Initialized the short name with all spaces */

  memset(dirinfo->fd_name, ' ', DIR_MAXFNAME);
 
  /* Handle a special case where there is no name.  Windows seems to use
   * the extension plus random stuff then ~1 to pat to 8 bytes.  Some
   * examples:
   *
   *   a.b          -> a.b          No long name
   *   a.,          -> A26BE~1._    Padded name to make unique, _ replaces ,
   *   .b           -> B1DD2~1      Extension used as name
   *   .bbbbbbb     -> BBBBBB~1     Extension used as name
   *   a.bbbbbbb    -> AAD39~1.BBB  Padded name to make unique.
   *   aaa.bbbbbbb  -> AAA~1.BBBB   Not padded, already unique?
   *   ,.bbbbbbb    -> _82AF~1.BBB  _ replaces ,
   *   +[],.bbbbbbb -> ____~1.BBB   _ replaces +[],
   */

  if (namechars < 1)
    {
       /* Use the extension as the name */

       DEBUGASSERT(ext && extchars > 0);
       ptr      = ext;
       ext      = NULL;
       namechar = extchars;
       extchars = 0;
    }
  else
    {
       ptr      = dirinfo.fd_ldname;
    }

  /* Then copy the name and extension, handling upper case conversions and
   * excluding forbidden characters.
   */

  ndx    = 0;  /* Position to write the next name character */
  endndx = 6;  /* Maximum index before we write ~! and switch to the extension */

  for (;;)
    {
      /* Get the next byte from the path.  Break out of the loop if we
       * encounter the end of null-terminated the long file name string.
       */

      ch = *ptr++;
      if (ch == '\0')
        {
          break;
        }

      /* Exclude those few characters included in long file names, but
       * excluded in short file name: '+', ',', ';', '=', '[', ']', and '.'
       */

      if (ch == '+' || ch == ',' || ch == '.' || ch == ';' ||
          ch == '=' || ch == '[' || ch == ']' || ch == '|')
        {
          /* Use the underbar character instead */

          ch = '_';
        }

      /* Handle lower case characters */

      ch = toupper(ch);
      
      /* We now have a valid character to add to the name or extension. */

      dirinfo->fd_name[ndx++] = ch;

      /* Did we just add a character to the name? */

      if (endndx == 6)
        {
          /* Decrement the number of characters available in the name
           * portion of the long name.
           */

          namechars--;

          /* Is it time to add ~1 to the string?  Will will do that if
           * either (1) we have already added the maximum number of
           * characters to the short name, or (2) if there are no further
           * characters available in the name portion of the long name.
           */

          if (namechars < 1 || ndx == 6)
            {
              /* Write the ~1 at the end of the name */

              dirinfo->fd_name[ndx++] = '~';
              dirinfo->fd_name[ndx++] = '1';

              /* Then switch to the extension (if there is one) */

              if (!ext || extchars < 1)
                {
                  return OK;
                }

              ndx    = 8;
              endndx = 11;
              ptr    = ext;
            }
        }

      /* No.. we just added a character to the extension */

      else
        {
          /* Decrement the number of characters available in the name
           * portion of the long name
           */

          extchars--;

          /* Is the extension complete? */

          if (extchars < 1 || ndx == 11)
            {
              return OK;
            }
        }
    }
}
#endif

/****************************************************************************
 * Name: fat_uniquealias
 *
 * Desciption:  Make sure that the short alias for the long file name is
 *   unique.  Modify the alias as necessary to assure uniqueness.
 *
 * Returned value:
 *   OK - The alias is unique.
 *   <0 - Otherwise an negated error is returned.
 *
 ****************************************************************************/

#ifdef CONFIG_FAT_LFN
static inline int fat_uniquealias(const char **path,
                                  struct fat_dirinfo_s *dirinfo)
{
#warning "Missing alias alias uniqueness logic"
  return OK;
}
#endif

/****************************************************************************
 * Name: fat_lfnchecksum
 *
 * Desciption:  Caculate the checksum of .
 *
 * Returned value:
 *   OK - The alias is unique.
 *   <0 - Otherwise an negated error is returned.
 *
 ****************************************************************************/

#ifdef CONFIG_FAT_LFN
static uint8_t fat_lfnchecksum(const uint8_t *sfname)
{
  uint8_t sum = 0;
  int i;

  for (i = DIR_MAXFNAME; i; i--)
    {
      sum = ((sum & 1) << 7) + (sum >> 1) + *sfname++;
	}

  return sum;
}
#endif

/****************************************************************************
 * Name: fat_path2dirname
 *
 * Desciption:  Convert a user filename into a properly formatted FAT
 *   (short 8.3) filename as it would appear in a directory entry.
 *
 ****************************************************************************/

static int fat_path2dirname(const char **path, struct fat_dirinfo_s *dirinfo,
                            char *terminator)
{
#ifdef CONFIG_FAT_LFN
  int ret;

  /* Assume no long file name */

  dirinfo->fd_lfname[0] = '\0';

  /* Then parse the (assumed) 8+3 short file name */

  ret = fat_parsesfname(path, dirinfo, terminator);
  if (ret < 0)
    {
      /* No, the name is not a valid short 8+3 file name. Try parsing
       * the long file name.
       */

      ret = fat_parselfname(path, dirinfo, terminator);
    }

  return ret;
#else
  /* Only short, 8+3 filenames supported */

  return fat_parsesfname(path, dirinfo, terminator);
#endif
}

/****************************************************************************
 * Name: fat_findsfnentry
 *
 * Desciption: Find a short file name directory entry.
 *
 ****************************************************************************/

static inline int fat_findsfnentry(struct fat_mountpt_s *fs,
                                   struct fat_dirinfo_s *dirinfo)
{
  uint16_t diroffset;
  uint8_t *direntry;
  int      ret;

  /* Search, beginning with the current sector, for a directory entry this
   * the match shore name
   */

  for (;;)
    {
      /* Read the next sector into memory */

      ret = fat_fscacheread(fs, dirinfo->dir.fd_currsector);
      if (ret < 0)
        {
          return ret;
        }

      /* Get a pointer to the directory entry */

      diroffset = DIRSEC_BYTENDX(fs, dirinfo->dir.fd_index);
      direntry  = &fs->fs_buffer[diroffset];

      /* Check if we are at the end of the directory */

      if (direntry[DIR_NAME] == DIR0_ALLEMPTY)
        {
          return -ENOENT;
        }

      /* Check if we have found the directory entry that we are looking for */

      if (direntry[DIR_NAME] != DIR0_EMPTY &&
          !(DIR_GETATTRIBUTES(direntry) & FATATTR_VOLUMEID) &&
          !memcmp(&direntry[DIR_NAME], dirinfo->fd_name, DIR_MAXFNAME) )
        {
          /* Yes.. Return success */

          dirinfo->fd_seq.ds_sector  = fs->fs_currentsector;
          dirinfo->fd_seq.ds_offset  = diroffset;
#ifdef CONFIG_FAT_LFN
          dirinfo->fd_seq.ds_cluster = dirinfo->dir.fd_currcluster;
#endif

          /* Position the last long file name directory entry at the same
           * position.
           */

#ifdef CONFIG_FAT_LFN
          dirinfo->fd_seq.ds_lfnsector  = dirinfo->fd_seq.ds_sector;
          dirinfo->fd_seq.ds_lfnoffset  = dirinfo->fd_seq.ds_offset;
          dirinfo->fd_seq.ds_lfncluster = dirinfo->fd_seq.ds_cluster;
#endif
         return OK;
        }

      /* No... get the next directory index and try again */

      if (fat_nextdirentry(fs, &dirinfo->dir) != OK)
        {
          return -ENOENT;
        }
    }
}

/****************************************************************************
 * Name: fat_cmplfnchunk
 *
 * Desciption:  There are 13 characters per LFN entry, broken up into three
 *   chunks for characts 1-5, 6-11, and 12-13.  This function will perform
 *   the comparison of a single chunk.
 *
 ****************************************************************************/

#ifdef CONFIG_FAT_LFN
static bool fat_cmplfnchunk(wchar_t *chunk, uint8_t *substr, int nchunk)
{
  uint8_t *ptr;
  int i;

  /* Check bytes 1-nchunk */

  for (i = 0; i < nchunk; i++)
    {
      wchar_t wch;

      /* If we encounter the NUL terminator in the name string, then it is
       * a match -- return TRUE.
       */

      if (*substr == 0)
        {
          return true;
        }

      /* Get the next unicode character from the chunk.  We only handle ASCII.
       * For ASCII, the upper byte should be zero and the lower should match
       * the ASCII code.
       */

      wch = fat_getuint16(chunk);
      if (wch & 0xff != (wchar_t)*substr)
        {
          return false;
        }

      /* Yes.. the characters match.  Try the next */

      chunk++;
      substr++;
    }

  /* All of the characters in the chunk match.. Return success */

  return true;
}
#endif

/****************************************************************************
 * Name: fat_cmplfname
 *
 * Desciption: Given an LFN directory entry, compare a substring of the name
 *   to a portion in the directory entry.
 *
 ****************************************************************************/

#ifdef CONFIG_FAT_LFN
static bool fat_cmplfname(uint8_t *direntry, uint8_t *substr)
{
  uint8_t *chunk;
  int i;

  /* Check bytes 1-5 */

  chunk = LDIR_PTRWCHAR1_5(direntry);
  if (fat_cmplfnchunk(chunk, substr, 5))
    {
      /* Check bytes 6-11 */

      chunk = LDIR_PTRWCHAR6_11(direntry);
      if (fat_cmplfnchunk(chunk, &substr[5], 6))
        {
          /* Check bytes 6-11 */

          chunk = efine LDIR_WCHAR12_13;
          return fat_cmplfnchunk(chunk, &substr[11], 2);
        }
    }

  return false;
}
#endif

/****************************************************************************
 * Name: fat_findlfnentry
 *
 * Desciption: Find a sequence of long file name directory entries.
 *
 ****************************************************************************/

#ifdef CONFIG_FAT_LFN
static inline int fat_findlfnentry(struct fat_mountpt_s *fs,
                                   struct fat_dirinfo_s *dirinfo)
{
  uint16_t diroffset;
  uint8_t *direntry;
  uint8_t  lastseq;
  uint8_t  seqno;
  uint8_t  nfullentries;
  uint8_t  nentries;
  uint8_t  remainder;
  uint8_t  cksum;
  int      offset;
  int      namelen;
  int      ret;

  /* Get the length of the long file name and make sure that it does
   * not exceed the maximum.
   */

  namelen = strlen(dirinfo.fd_lfname);
  if (namelen > LDIR_MAXFNAME)
    {
      return -EINVAL;
    }

  /* How many LFN directory entries are we expecting? */

  nfullentries = namelen / LDIR_MAXLFNCHARS;
  remainder    = namelen - nfullentries * LDIR_MAXLFNCHARS;
  nentries     = nfullentries;
  if (remainder > 0)
    {
      nentries++;
    }
  DEBUGASSERT(nentries > 0 && nentries <= LDIR_MAXLFNS);

  /* This is the first sequency number we are looking for, the sequence
   * number of the last LFN entry (remember that they appear in reverse
   * order.. from last to first).
   */

  lastseq = LDIR_SEQ | nentries;
  seqno   = lastseq;

  /* Search, beginning with the current sector, for a directory entry this
   * the match shore name
   */

  for (;;)
    {
      /* Read the next sector into memory */

      ret = fat_fscacheread(fs, dirinfo->dir.fd_currsector);
      if (ret < 0)
        {
          return ret;
        }

      /* Get a pointer to the directory entry */

      diroffset = DIRSEC_BYTENDX(fs, dirinfo->dir.fd_index);
      direntry  = &fs->fs_buffer[diroffset];

      /* Check if we are at the end of the directory */

      if (direntry[DIR_NAME] == DIR0_ALLEMPTY)
        {
          return -ENOENT;
        }

      /* Is this an LFN entry?  Does it have the sequence number we are
       * looking for?
       */

      if (LDIR_GETATTRIBUTES(direntry) != LDDIR_LFNATTR) ||
          LDIR_GETSEQ(direntry) != seqno);
        {
          /* No, restart the search at the next entry */

          seqno = lastseq;
          goto nextentry;
        }

      /* Yes.. If this is not the "last" LFN entry, then the checksum must
       * also be the same.
       */

      if (seqno == lastseq)
        {
          /* Just save the checksum for subsequent checks */

          cksum = LDIR_GETCHECKSUM(direntry);
        }

      /* Not the first entry in the sequence.  Does the checksum match the
       * previous sequences?
       */

      else if (cksum != LDIR_GETCHECKSUM(direntry))
        {
          /* No, restart the search at the next entry */

          seqno = lastseq;
          goto nextentry;
        }

      /* Check if the name substring in this LFN matches the corresponding
       * substring of the name we are looking for.
       */

      offset = ((seqno & LDIR0_SEQ_MASK) - 1) * LDIR_MAXLFNCHARS;
      if (fat_cmplfname(direntry, &dirinfo.fd_lfname[offset]))
        {
          /* Yes.. it matches.  Check the sequence number.  Is this the
           * "last" LFN entry (i.e., the one that appears first)?
           */

          if (seqno == lastseq)
            {
              /* Yes.. Save information about this LFN entry position */

              dirinfo->fd_seq.ds_lfnsector  = fs->fs_currentsector;
              dirinfo->fd_seq.ds_lfnoffset  = diroffset;
              dirinfo->fd_seq.ds_lfncluster = dirinfo->dir.fd_currcluster;
              seqno &= LDIR0_SEQ_MASK;
            }

          /* Is this the first sequence number (i.e., the LFN entry that
           * will appear last)?
           */

          if (seqno == 1)
            {
              /* We have found all of the LFN entries.  The next directory
               * entry should be the one containing the short file name
               * alias and all of the meat about the file or directory.
               */

              if (fat_nextdirentry(fs, &dirinfo->dir) != OK)
                {
                  return -ENOENT;
                }

             /* Verify the checksum */

             if (fat_lfnchecksum(dirinfo->fd_name) == cksum)
               {
                 /* Success! Save the position of the directory entry and
                  * return success.
                  */

                 dirinfo->fd_seq.ds_sector  = fs->fs_currentsector;
                 dirinfo->fd_seq.ds_offset  = diroffset;
                 dirinfo->fd_seq.ds_cluster = dirinfo->dir.fd_currcluster;
                 return OK;
                }

              /* Bad news.. reset and continue with this entry (which is
               * probably not an LFN entry unless the file systen is
               * seriously corrupted.
               */

              seqno = lastseq;
              continue;
            }

          /* No.. there are more LFN entries to go.  Decrement the sequence
           * number and check the next directory entry.
           */

          seqno--;
        }
      else
        {
          /* No.. the names do not match.  Restart the search at the next
           * entry.
           */

          seqno = lastseq;
        }

      /* Continue at the next directory entry */
 
next_entry:
      if (fat_nextdirentry(fs, &dirinfo->dir) != OK)
        {
          return -ENOENT;
        }
    }
}
#endif

/****************************************************************************
 * Name: fat_allocatesfnentry
 *
 * Desciption: Find a free directory entry for a short file name entry.
 *
 ****************************************************************************/

static inline int fat_allocatesfnentry(struct fat_mountpt_s *fs,
                                       struct fat_dirinfo_s *dirinfo)
{
  uint16_t diroffset;
  uint8_t *direntry;
  uint8_t  ch;
  int      ret;

  for (;;)
    {
      /* Read the directory sector into fs_buffer */

      ret = fat_fscacheread(fs, dirinfo->dir.fd_currsector);
      if (ret < 0)
        {
          /* Make sure that the return value is NOT -ENOSPC */

          return -EIO;
        }

      /* Get a pointer to the entry at fd_index */

      diroffset = (dirinfo->dir.fd_index & DIRSEC_NDXMASK(fs)) * DIR_SIZE;
      direntry  = &fs->fs_buffer[diroffset];

      /* Check if this directory entry is empty */

      ch = direntry[DIR_NAME];
      if (ch == DIR0_ALLEMPTY || ch == DIR0_EMPTY)
        {
          /* It is empty -- we have found a directory entry */

          dirinfo->fd_seq.ds_sector  = fs->fs_currentsector;
          dirinfo->fd_seq.ds_offset  = diroffset;
#ifdef CONFIG_FAT_LFN
          dirinfo->fd_seq.ds_cluster = dirinfo->dir.fd_currcluster;
#endif

          /* Set the "last" long file name offset to the same entry */

#ifdef CONFIG_FAT_LFN
          dirinfo->fd_seq.ds_lfnsector  = dirinfo->fd_seq.ds_sector;
          dirinfo->fd_seq.ds_lfnoffset  = dirinfo->fd_seq.ds_offset;
          dirinfo->fd_seq.ds_lfncluster = dirinfo->fd_seq.ds_cluster;
#endif
          return OK;
        }

      /* It is not empty try the next one */

      ret = fat_nextdirentry(fs, &dirinfo->dir);
      if (ret < 0)
        {
          /* This will return -ENOSPC if we have examined all of the
           * directory entries without finding a free entry.
           */

          return ret;
        }
    }
}

/****************************************************************************
 * Name: fat_allocatelfnentry
 *
 * Desciption: Find a sequence of free directory entries for a several long
 *   and one short file name entry.
 *
 ****************************************************************************/

#ifdef CONFIG_FAT_LFN
static inline int fat_allocatelfnentry(struct fat_mountpt_s *fs,
                                       struct fat_dirinfo_s *dirinfo)
{
  uint16_t diroffset;
  uint8_t *direntry;
  uint8_t  nentries;
  uint8_t  remainder;
  uint8_t  needed;
  int      namelen;
  int      ret;

  /* Get the length of the long file name and make sure that it does
   * not exceed the maximum.
   */

  namelen = strlen(dirinfo->fd_lfname);
  if (namelen > LDIR_MAXFNAME)
    {
      return -EINVAL;
    }

  /* How many LFN directory entries are we expecting? */

  nentries   = namelen / LDIR_MAXLFNCHARS;
  remainder  = namelen - nfullentries * LDIR_MAXLFNCHARS;
  if (remainder > 0)
    {
      nentries++;
    }
  DEBUGASSERT(nentries > 0 && nentries <= LDIR_MAXLFNS);

  /* Plus another for short file name entry that follows the sequence of LFN
   * entries.
   */

  nentries++;

  /* Now, search the directory looking for a sequence for free entries that
   * long.
   */

  needed = nentries;
  for (;;)
    {
      /* Read the directory sector into fs_buffer */

      ret = fat_fscacheread(fs, dirinfo->dir.fd_currsector);
      if (ret < 0)
        {
          /* Make sure that the return value is NOT -ENOSPC */

          return -EIO;
        }

      /* Get a pointer to the entry at fd_index */

      diroffset = (dirinfo->dir.fd_index & DIRSEC_NDXMASK(fs)) * DIR_SIZE;
      direntry  = &fs->fs_buffer[diroffset];

      /* Check if this directory entry is empty */

      ch = direntry[DIR_NAME];
      if (ch == DIR0_ALLEMPTY || ch == DIR0_EMPTY)
        {
          /* It is empty -- we have found a directory entry.  Is this the
           * "last" LFN entry (i.e., the one that occurs first)?
           */

          if (needed == nentries)
            {
              /* Yes.. remember the position of this entry */

              dirinfo->fd_seq.ds_lfnsector  = fs->fs_currentsector;
              dirinfo->fd_seq.ds_lfnoffset  = diroffset;
              dirinfo->fd_seq.ds_lfncluster = dirinfo->dir.fd_currcluster;
              }

          /* Is this last entry we need (i.e., the entry for the short
           * file name entry)?
           */
           
          if (needed <= 1)
            {
              /* Yes.. remember the position of this entry and return
               * success.
               */

              dirinfo->fd_seq.ds_sector  = fs->fs_currentsector;
              dirinfo->fd_seq.ds_offset  = diroffset;
              dirinfo->fd_seq.ds_cluster = dirinfo->dir.fd_currcluster;
              return OK;
            }

          /* Otherwise, just decrement the number of directory entries
           * needed and continue looking.
           */

          needed--;
        }

      /* The directory entry is not available */

      else
        {
          /* Reset the search and continue looking */

          needed = nentries;
        }

      /* Try the next directory entry */

      ret = fat_nextdirentry(fs, &dirinfo->dir);
      if (ret < 0)
        {
          /* This will return -ENOSPC if we have examined all of the
           * directory entries without finding a free entry.
           */

          return ret;
        }
    }
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: fat_finddirentry
 *
 * Desciption: Given a path to something that may or may not be in the file
 *   system, return the directory entry of the item.
 *
 ****************************************************************************/

int fat_finddirentry(struct fat_mountpt_s *fs, struct fat_dirinfo_s *dirinfo,
                     const char *path)
{
  off_t    cluster;
  uint8_t *direntry;
  char     terminator;
  int      ret;

  /* Initialize to traverse the chain.  Set it to the cluster of the root
   * directory
   */

  cluster = fs->fs_rootbase;
  if (fs->fs_type == FSTYPE_FAT32)
    {
      /* For FAT32, the root directory is variable sized and is a cluster
       * chain like any other directory.  fs_rootbase holds the first
       * cluster of the root directory.
       */

      dirinfo->dir.fd_startcluster = cluster;
      dirinfo->dir.fd_currcluster  = cluster;
      dirinfo->dir.fd_currsector   = fat_cluster2sector(fs, cluster);
    }
  else
    {
      /* For FAT12/16, the first sector of the root directory is a sector
       * relative to the first sector of the fat volume.
       */

      dirinfo->dir.fd_startcluster = 0;
      dirinfo->dir.fd_currcluster  = 0;
      dirinfo->dir.fd_currsector   = cluster;
    }

  /* fd_index is the index into the current directory table */

  dirinfo->dir.fd_index = 0;

  /* If no path was provided, then the root directory must be exactly what
   * the caller is looking for.
   */

  if (*path == '\0')
    {
      dirinfo->fd_root = true;
      return OK;
    }

  /* Otherwise, loop until the path is found */

  for (;;)
    {
      /* Convert the next the path segment name into the kind of name that
       * we would see in the directory entry.
       */

      ret = fat_path2dirname(&path, dirinfo, &terminator);
      if (ret < 0)
        {
          /* ERROR:  The filename contains invalid characters or is
           * too long.
           */

          return ret;
        }

      /* Is this a path segment a long or a short file.  Was a long file
       * name parsed?
       */

#ifdef CONFIG_FAT_LFN
      if (dirinfo.fd_lfname[0] != '\0')
        {
          /* Yes.. Search for the sequence of long file name directory
           * entries.
           */
 
          ret = fat_findlfnentry(fs, dirinfo);
        }

      /* No.. Search for the single short file name directory entry */

      else
#endif
        {
          ret = fat_findsfnentry(fs, dirinfo);
        }

      /* Did we find the directory entries? */

      if (ret < 0)
        {
          return ret;
        }

      /* If the terminator character in the path was the end of the string
       * then we have successfully found the directory entry that describes
       * the path.
       */

      if (!terminator)
        {
          /* Return success meaning that the description the matching
           * directory entry is in dirinfo.
           */

          return OK;
        }

      /* No.. then we have found one of the intermediate directories on
       * the way to the final path target.  In this case, make sure
       * the thing that we found is, indeed, a directory.
       */

      direntry = &fs->fs_buffer[dirinfo->fd_seq.ds_offset];
      if (!(DIR_GETATTRIBUTES(direntry) & FATATTR_DIRECTORY))
        {
          /* Ooops.. we found something else */

          return -ENOTDIR;
        }

      /* Get the cluster number of this directory */

      cluster =
          ((uint32_t)DIR_GETFSTCLUSTHI(direntry) << 16) |
          DIR_GETFSTCLUSTLO(direntry);

      /* The restart scanning at the new directory */

      dirinfo->dir.fd_currcluster = dirinfo->dir.fd_startcluster = cluster;
      dirinfo->dir.fd_currsector  = fat_cluster2sector(fs, cluster);
      dirinfo->dir.fd_index       = 2;
    }
}

/****************************************************************************
 * Name: fat_allocatedirentry
 *
 * Desciption: Find a free directory entry
 *
 ****************************************************************************/

int fat_allocatedirentry(struct fat_mountpt_s *fs, struct fat_dirinfo_s *dirinfo)
{
  int32_t  cluster;
  off_t    sector;
  int      ret;
  int      i;

  /* Re-initialize directory object */

  cluster = dirinfo->dir.fd_startcluster;

  /* Loop until we successfully allocate the sequence of directory entries
   * or until to fail to extend the directory cluster chain.
   */

  for (;;)
    {
      /* Can this cluster chain be extended */

      if (cluster)
        {
         /* Cluster chain can be extended */

          dirinfo->dir.fd_currcluster = cluster;
          dirinfo->dir.fd_currsector  = fat_cluster2sector(fs, cluster);
        }
      else
        {
          /* Fixed size FAT12/16 root directory is at fixxed offset/size */

          dirinfo->dir.fd_currsector = fs->fs_rootbase;
        }
    dirinfo->dir.fd_index = 0;

      /* Is this a path segment a long or a short file.  Was a long file
       * name parsed?
       */

#ifdef CONFIG_FAT_LFN
      if (dirinfo.fd_lfname[0] != '\0')
        {
          /* Yes.. Allocate for the sequence of long file name directory
           * entries plus a short file name directory entry.
           */
 
          ret = fat_allocatelfnentry(fs, dirinfo);
        }

      /* No.. Allocate only a short file name directory entry */

      else
#endif
        {
          ret = fat_allocatesfnentry(fs, dirinfo);
        }

      /* Did we successfully allocate the directory entries?  If the error
       * value is -ENOSPC, then we can try to extend the directory cluster
       * (we can't handle other return values)
       */

      if (ret == OK || ret != -ENOSPC)
        {
          return ret;
        }

      /* If we get here, then we have reached the end of the directory table
       * in this sector without finding a free directory entry.
       *
       * It this is a fixed size dirctory entry, then this is an error.
       * Otherwise, we can try to extend the directory cluster chain to
       * make space for the new directory entry.
       */

      if (!cluster)
        {
          /* The size is fixed */

          return -ENOSPC;
        }

      /* Try to extend the cluster chain for this directory */

      cluster = fat_extendchain(fs, dirinfo->dir.fd_currcluster);
      if (cluster < 0)
        {
          return cluster;
        }

     /* Flush out any cached data in fs_buffer.. we are going to use
      * it to initialize the new directory cluster.
      */

      ret = fat_fscacheflush(fs);
      if (ret < 0)
        {
          return ret;
        }

      /* Clear all sectors comprising the new directory cluster */

      fs->fs_currentsector = fat_cluster2sector(fs, cluster);
      memset(fs->fs_buffer, 0, fs->fs_hwsectorsize);

      sector = sector;
      for (i = fs->fs_fatsecperclus; i; i--)
        {
          ret = fat_hwwrite(fs, fs->fs_buffer, sector, 1);
          if ( ret < 0)
            {
              return ret;
            }
          sector++;
        }
    }
}

/****************************************************************************
 * Name: fat_freedirentry
 *
 * Desciption:  Free the directory entry.
 *
 ****************************************************************************/

int fat_freedirentry(struct fat_mountpt_s *fs, struct fat_dirseq_s *seq)
{
#ifdef CONFIG_FAT_LFN
  uint16_t diroffset;
  uint8_t *direntry;
  int      ret;

  /* Set it to the cluster containing the "last" LFN entry (that appears
   * first on the media).
   */

  dirinfo->dir.fd_startcluster = dirinfo->fd_seq.fd_lfncluster;
  dirinfo->dir.fd_currcluster  = dirinfo->fd_seq.fd_lfnsector;
  dirinfo->dir.fd_index        = dirinfo->fd_seq.fd_lfnoffset / DIR_SIZE;

  /* Free all of the directory entries used for the sequence of long file name
   * and for the single short file name entry.
   */

  for (;;)
    {
      /* Read the directory sector into the sector cache */

      ret = fat_fscacheread(fs, dirinfo->dir.fd_currsector);
      if (ret < 0)
        {
          return ret;
        }

      /* Get a pointer to the directory entry */

      diroffset = (dirinfo->dir.fd_index & DIRSEC_NDXMASK(fs)) * DIR_SIZE;
      direntry  = &fs->fs_buffer[diroffset];

      /* Then mark the entry as deleted */

      direntry[DIR_NAME] = DIR0_EMPTY;
      fs->fs_dirty       = true;

      /* Did we just free the single short file name entry? */

      if (dirinfo->dir.fd_currsector == dirinfo->fd_seq.fd_sector &&
          diroffset == dirinfo->fd_seq.fd_sector)
        {
          /* Yes.. then we are finished. flush anything remaining in the
           * cache and return, probably successfully.
           */

          return fat_fscacheflush(fs);
        }

      /* There are moe entries to go.. Try the next directory entry */

      ret = fat_nextdirentry(fs, &dirinfo->dir);
      if (ret < 0)
        {
          return ret;
        }
    }

#else
  uint8_t *direntry;
  int      ret;

  /* Free the single short file name entry.
   *
   * Make sure that the sector containing the directory entry is in the
   * cache.
   */

  ret = fat_fscacheread(fs, seq->ds_sector);
  if (ret == OK)
    {
      /* Then mark the entry as deleted */

      direntry           = &fs->fs_buffer[seq->ds_offset];
      direntry[DIR_NAME] = DIR0_EMPTY;
      fs->fs_dirty       = true;
    }

  return ret;
#endif
}

/****************************************************************************
 * Name: fat_dirname2path
 *
 * Desciption:  Convert a filename in a raw directory entry into a user
 *    filename.  This is essentially the inverse operation of that performed
 *    by fat_path2dirname.  See that function for more details.
 *
 ****************************************************************************/

int fat_dirname2path(char *path, uint8_t *direntry)
{
#ifdef CONFIG_FAT_LCNAMES
    uint8_t ntflags;
#endif
    int  ch;
    int  ndx;

    /* Check if we will be doing upper to lower case conversions */

#ifdef CONFIG_FAT_LCNAMES
    ntflags = DIR_GETNTRES(direntry);
#endif

    /* Get the 8-byte filename */

    for (ndx = 0; ndx < 8; ndx++)
      {
        /* Get the next filename character from the directory entry */

        ch = direntry[ndx];

        /* Any space (or ndx==8) terminates the filename */

        if (ch == ' ')
          {
            break;
          }

        /* In this version, we never write 0xe5 in the directoryfilenames
         * (because we do not handle any character sets where 0xe5 is valid
         * in a filaname), but we could encounted this in a filesystem
         * written by some other system
         */

        if (ndx == 0 && ch == DIR0_E5)
          {
            ch = 0xe5;
          }

        /* Check if we should perform upper to lower case conversion
         * of the (whole) filename.
         */

#ifdef CONFIG_FAT_LCNAMES
        if (ntflags & FATNTRES_LCNAME && isupper(ch))
          {
            ch = tolower(ch);
          }
#endif
        /* Copy the next character into the filename */

        *path++ = ch;
      }

    /* Check if there is an extension */

    if (direntry[8] != ' ')
      {
        /* Yes, output the dot before the extension */

        *path++ = '.';

        /* Then output the (up to) 3 character extension */

        for (ndx = 8; ndx < 11; ndx++)
          {
            /* Get the next extensions character from the directory entry */

            ch = direntry[DIR_NAME + ndx];

            /* Any space (or ndx==11) terminates the extension */

            if (ch == ' ')
              {
                break;
              }

            /* Check if we should perform upper to lower case conversion
             * of the (whole) filename.
             */

#ifdef CONFIG_FAT_LCNAMES
            if (ntflags & FATNTRES_LCEXT && isupper(ch))
              {
                ch = tolower(ch);
              }
#endif
        /* Copy the next character into the filename */

            *path++ = ch;
          }
      }

    /* Put a null terminator at the end of the filename */

    *path = '\0';
    return OK;
}

/****************************************************************************
 * Name: fat_dirnamewrite
 *
 * Desciption: Write the (possibly long) directory entry name.
 *
 * Assumption:  The directory sector is in the cache.
 *
 ****************************************************************************/

int fat_dirnamewrite(struct fat_mountpt_s *fs, struct fat_dirinfo_s *dirinfo)
{
  uint8_t *direntry = &fs->fs_buffer[dirinfo->fd_seq.ds_offset];
 
  memcpy(&direntry[DIR_NAME], dirinfo->fd_name, DIR_MAXFNAME);
#ifdef CONFIG_FLAT_LCNAMES
  DIR_PUTNTRES(direntry, dirinfo->fd_ntflags);
#else
  DIR_PUTNTRES(direntry, 0);
#endif
  fs->fs_dirty = true;
  return OK;
}

/****************************************************************************
 * Name: fat_dirwrite
 *
 * Desciption: Write a directory entry, possibly with a long file name
 *
 * Assumption:  The directory sector is in the cache.  The caller will write
 *   sector information.
 *
 ****************************************************************************/

int fat_dirwrite(struct fat_mountpt_s *fs, struct fat_dirinfo_s *dirinfo,
                 uint8_t attributes, uint32_t fattime)
{
  uint8_t *direntry;

  /* Initialize the 32-byte directory entry */

  direntry = &fs->fs_buffer[dirinfo->fd_seq.ds_offset];
  memset(direntry, 0, DIR_SIZE);

  /* Directory name info */

  (void)fat_dirnamewrite(fs, dirinfo);

  /* Set the attribute attribute, write time, creation time */

  DIR_PUTATTRIBUTES(direntry, attributes);

  /* Set the time information */

  DIR_PUTWRTTIME(direntry, fattime & 0xffff);
  DIR_PUTCRTIME(direntry, fattime & 0xffff);
  DIR_PUTWRTDATE(direntry, fattime >> 16);
  DIR_PUTCRDATE(direntry, fattime >> 16);

  fs->fs_dirty = true;
  return OK;
}

/****************************************************************************
 * Name: fat_dircreate
 *
 * Desciption: Create a directory entry for a new file
 *
 ****************************************************************************/

int fat_dircreate(struct fat_mountpt_s *fs, struct fat_dirinfo_s *dirinfo)
{
  uint32_t fattime;
  int ret;

  /* Allocate a directory entry entry */

  ret = fat_allocatedirentry(fs, dirinfo);
  if (ret != OK)
    {
      /* Failed to set up directory entry */

      return ret;
    }

  /* Write the directory entry with the current time and the ARCHIVE attribute */

  fattime = fat_systime2fattime();
  return fat_dirwrite(fs, dirinfo, FATATTR_ARCHIVE, fattime);
}

/****************************************************************************
 * Name: fat_remove
 *
 * Desciption: Remove a directory or file from the file system.  This
 *   implements both rmdir() and unlink().
 *
 ****************************************************************************/

int fat_remove(struct fat_mountpt_s *fs, const char *relpath, bool directory)
{
  struct fat_dirinfo_s dirinfo;
  uint32_t             dircluster;
  uint8_t             *direntry;
  int                  ret;

  /* Find the directory entry referring to the entry to be deleted */

  ret = fat_finddirentry(fs, &dirinfo, relpath);
  if (ret != OK)
    {
      /* No such path */

      return -ENOENT;
    }

  /* Check if this is a FAT12/16 root directory */

  if (dirinfo.fd_root)
    {
      /* The root directory cannot be removed */

      return -EPERM;
    }

  /* The object has to have write access to be deleted */

  direntry = &fs->fs_buffer[dirinfo.fd_seq.ds_offset];
  if ((DIR_GETATTRIBUTES(direntry) & FATATTR_READONLY) != 0)
    {
      /* It is a read-only entry */

      return -EACCES;
    }

  /* Get the directory sector and cluster containing the
   * entry to be deleted
   */

  dircluster =
      ((uint32_t)DIR_GETFSTCLUSTHI(direntry) << 16) |
      DIR_GETFSTCLUSTLO(direntry);

  /* Is this entry a directory? */

  if (DIR_GETATTRIBUTES(direntry) & FATATTR_DIRECTORY)
    {
      /* It is a sub-directory. Check if we are be asked to remove
       * a directory or a file.
       */

      if (!directory)
        {
          /* We are asked to delete a file */

          return -EISDIR;
        }

      /* We are asked to delete a directory. Check if this
       * sub-directory is empty
       */

      dirinfo.dir.fd_currcluster = dircluster;
      dirinfo.dir.fd_currsector  = fat_cluster2sector(fs, dircluster);
      dirinfo.dir.fd_index       = 2;

      /* Loop until either (1) an entry is found in the directory
       * (error), (2) the directory is found to be empty, or (3) some
       * error occurs.
       */

      for (;;)
        {
          unsigned int subdirindex;
          uint8_t     *subdirentry;

          /* Make sure that the sector containing the of the
           * subdirectory sector is in the cache
           */

          ret = fat_fscacheread(fs, dirinfo.dir.fd_currsector);
          if (ret < 0)
            {
              return ret;
            }

          /* Get a reference to the next entry in the directory */

          subdirindex = (dirinfo.dir.fd_index & DIRSEC_NDXMASK(fs)) * DIR_SIZE;
          subdirentry = &fs->fs_buffer[subdirindex];

          /* Is this the last entry in the direcory? */

          if (subdirentry[DIR_NAME] == DIR0_ALLEMPTY)
            {
              /* Yes then the directory is empty.  Break out of the
               * loop and delete the directory.
               */

              break;
            }

          /* Check if the next entry refers to a file or directory */

          if (subdirentry[DIR_NAME] != DIR0_EMPTY &&
              !(DIR_GETATTRIBUTES(subdirentry) & FATATTR_VOLUMEID))
            {
              /* The directory is not empty */

              return -ENOTEMPTY;
            }

          /* Get the next directory entry */

          ret = fat_nextdirentry(fs, &dirinfo.dir);
          if (ret < 0)
            {
              return ret;
            }
        }
    }
  else
    {
      /* It is a file. Check if we are be asked to remove a directory
       * or a file.
       */

      if (directory)
        {
          /* We are asked to remove a directory */

          return -ENOTDIR;
        }
    }

  /* Mark the directory entry 'deleted' */

  ret = fat_freedirentry(fs, &dirinfo.fd_seq);
  if (ret < 0)
    {
      return ret;
    }

  /* And remove the cluster chain making up the subdirectory */

  ret = fat_removechain(fs, dircluster);
  if (ret < 0)
    {
      return ret;
    }

  /* Update the FSINFO sector (FAT32) */

  ret = fat_updatefsinfo(fs);
  if (ret < 0)
    {
      return ret;
    }

  return OK;
}
