/****************************************************************************
 * lib_libvsprintf.c
 *
 *   Copyright (C) 2007, 2008 Gregory Nutt. All rights reserved.
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
 * Compilation Switches
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/compiler.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>

#include "lib_internal.h"

/****************************************************************************
 * Definitions
 ****************************************************************************/

enum
{
  FMT_RJUST = 0, /* Default */
  FMT_LJUST,
  FMT_RJUST0,
  FMT_CENTER
};

#define FLAG_SHOWPLUS            0x01
#define FLAG_ALTFORM             0x02
#define FLAG_HASDOT              0x04
#define FLAG_HASASTERISKWIDTH    0x08
#define FLAG_HASASTERISKTRUNC    0x10
#define FLAG_LONGPRECISION       0x20
#define FLAG_LONGLONGPRECISION   0x40
#define FLAG_NEGATE              0x80

#define SET_SHOWPLUS(f)          do (f) |= FLAG_SHOWPLUS; while (0)
#define SET_ALTFORM(f)           do (f) |= FLAG_ALTFORM; while (0)
#define SET_HASDOT(f)            do (f) |= FLAG_HASDOT; while (0)
#define SET_HASASTERISKWIDTH(f)  do (f) |= FLAG_HASASTERISKWIDTH; while (0)
#define SET_HASASTERISKTRUNC(f)  do (f) |= FLAG_HASASTERISKTRUNC; while (0)
#define SET_LONGPRECISION(f)     do (f) |= FLAG_LONGPRECISION; while (0)
#define SET_LONGLONGPRECISION(f) do (f) |= FLAG_LONGLONGPRECISION; while (0)
#define SET_NEGATE(f)            do (f) |= FLAG_NEGATE; while (0)

#define CLR_SHOWPLUS(f)          do (f) &= ~FLAG_SHOWPLUS; while (0)
#define CLR_ALTFORM(f)           do (f) &= ~FLAG_ALTFORM; while (0)
#define CLR_HASDOT(f)            do (f) &= ~FLAG_HASDOT; while (0)
#define CLR_HASASTERISKWIDTH(f)  do (f) &= ~FLAG_HASASTERISKWIDTH; while (0)
#define CLR_HASASTERISKTRUNC(f)  do (f) &= ~FLAG_HASASTERISKTRUNC; while (0)
#define CLR_LONGPRECISION(f)     do (f) &= ~FLAG_LONGPRECISION; while (0)
#define CLR_LONGLONGPRECISION(f) do (f) &= ~FLAG_LONGLONGPRECISION; while (0)
#define CLR_NEGATE(f)            do (f) &= ~FLAG_NEGATE; while (0)
#define CLR_SIGNED(f)            do (f) &= ~(FLAG_SHOWPLUS|FLAG_NEGATE); while (0)

#define IS_SHOWPLUS(f)           (((f) & FLAG_SHOWPLUS) != 0)
#define IS_ALTFORM(f)            (((f) & FLAG_ALTFORM) != 0)
#define IS_HASDOT(f)             (((f) & FLAG_HASDOT) != 0)
#define IS_HASASTERISKWIDTH(f)   (((f) & FLAG_HASASTERISKWIDTH) != 0)
#define IS_HASASTERISKTRUNC(f)   (((f) & FLAG_HASASTERISKTRUNC) != 0)
#define IS_LONGPRECISION(f)      (((f) & FLAG_LONGPRECISION) != 0)
#define IS_LONGLONGPRECISION(f)  (((f) & FLAG_LONGLONGPRECISION) != 0)
#define IS_NEGATE(f)             (((f) & FLAG_NEGATE) != 0)
#define IS_SIGNED(f)             (((f) & (FLAG_SHOWPLUS|FLAG_NEGATE)) != 0)

/****************************************************************************
 * Private Type Declarations
 ****************************************************************************/

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/* Pointer to ASCII conversion */

#ifdef CONFIG_PTR_IS_NOT_INT
static void ptohex(FAR struct lib_stream_s *obj, ubyte flags, FAR void *p);
#ifndef CONFIG_NOPRINTF_FIELDWIDTH
static int  getsizesize(ubyte fmt, ubyte flags, FAR void *p)
#endif /* CONFIG_NOPRINTF_FIELDWIDTH */
#endif /* CONFIG_PTR_IS_NOT_INT */

/* Unsigned int to ASCII conversion */

static void utodec(FAR struct lib_stream_s *obj, unsigned int n);
static void utohex(FAR struct lib_stream_s *obj, unsigned int n, ubyte a);
static void utooct(FAR struct lib_stream_s *obj, unsigned int n);
static void utobin(FAR struct lib_stream_s *obj, unsigned int n);
static void utoascii(FAR struct lib_stream_s *obj, ubyte fmt,
                     ubyte flags, unsigned int lln);

#ifndef CONFIG_NOPRINTF_FIELDWIDTH
static void fixup(ubyte fmt, FAR ubyte *flags, int *n);
static int  getusize(ubyte fmt, ubyte flags, unsigned int lln);
#endif

/* Unsigned long int to ASCII conversion */

#ifdef CONFIG_LONG_IS_NOT_INT
static void lutodec(FAR struct lib_stream_s *obj, unsigned long ln);
static void lutohex(FAR struct lib_stream_s *obj, unsigned long ln, ubyte a);
static void lutooct(FAR struct lib_stream_s *obj, unsigned long ln);
static void lutobin(FAR struct lib_stream_s *obj, unsigned long ln);
static void lutoascii(FAR struct lib_stream_s *obj, ubyte fmt,
                      ubyte flags, unsigned long ln);
#ifndef CONFIG_NOPRINTF_FIELDWIDTH
static void lfixup(ubyte fmt, FAR ubyte *flags, long *ln);
static int  getlusize(ubyte fmt, FAR ubyte flags, unsigned long ln);
#endif
#endif

/* Unsigned long long int to ASCII conversions */

#ifdef CONFIG_HAVE_LONG_LONG
static void llutodec(FAR struct lib_stream_s *obj, unsigned long long lln);
static void llutohex(FAR struct lib_stream_s *obj, unsigned long long lln, ubyte a);
static void llutooct(FAR struct lib_stream_s *obj, unsigned long long lln);
static void llutobin(FAR struct lib_stream_s *obj, unsigned long long lln);
static void llutoascii(FAR struct lib_stream_s *obj, ubyte fmt,
                       ubyte flags, unsigned long long lln);
#ifndef CONFIG_NOPRINTF_FIELDWIDTH
static void llfixup(ubyte fmt, FAR ubyte *flags, FAR long long *lln);
static int  getllusize(ubyte fmt, FAR ubyte flags, FAR unsigned long long lln);
#endif
#endif

#ifndef CONFIG_NOPRINTF_FIELDWIDTH
static void prejustify(FAR struct lib_stream_s *obj, ubyte fmt,
                       ubyte flags, int fieldwidth, int numwidth);
static void postjustify(FAR struct lib_stream_s *obj, ubyte fmt,
                        ubyte flags, int fieldwidth, int numwidth);
#endif

/****************************************************************************
 * Global Constant Data
 ****************************************************************************/

/****************************************************************************
 * Global Variables
 ****************************************************************************/

/****************************************************************************
 * Private Constant Data
 ****************************************************************************/

static const char g_nullstring[] = "(null)";

/****************************************************************************
 * Private Variables
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ptohex
 ****************************************************************************/

#ifdef CONFIG_PTR_IS_NOT_INT
static void ptohex(FAR struct lib_stream_s *obj, ubyte flags, FAR void *p)
{
  union
  {
    uint32    dw;
    FAR void *p;
  } u;
  ubyte bits;

  /* Check for alternate form */

  if (IS_ALTFORM(flags))
    {
      /* Prefix the number with "0x" */

      obj->put(obj, '0');
      obj->put(obj, 'x');
    }

  u.dw = 0;
  u.p  = p;

  for (bits = 8*sizeof(void *); bits > 0; bits -= 4)
    {
      ubyte nibble = (ubyte)((u.dw >> (bits - 4)) & 0xf);
      if (nibble < 10)
        {
          obj->put(obj, nibble + '0');
        }
      else
        {
          obj->put(obj, nibble + 'a' - 10);
        }
    }
}

/****************************************************************************
 * Name: getpsize
 ****************************************************************************/

#ifndef CONFIG_NOPRINTF_FIELDWIDTH
static int getpsize(ubyte flags, FAR void *p)
{
  struct lib_stream_s nullstream;
  lib_nullstream(&nullstream);

  ptohex(&nullstream, flags, p);
  return nullstream.nput;
}

#endif /* CONFIG_NOPRINTF_FIELDWIDTH */
#endif /* CONFIG_PTR_IS_NOT_INT */

/****************************************************************************
 * Name: utodec
 ****************************************************************************/

static void utodec(FAR struct lib_stream_s *obj, unsigned int n)
{
  unsigned int remainder = n % 10;
  unsigned int dividend  = n / 10;

  if (dividend)
    {
      utodec(obj, dividend);
    }

  obj->put(obj, (remainder + (unsigned int)'0'));
}

/****************************************************************************
 * Name: utohex
 ****************************************************************************/

static void utohex(FAR struct lib_stream_s *obj, unsigned int n, ubyte a)
{
  boolean nonzero = FALSE;
  ubyte bits;

  for (bits = 8*sizeof(unsigned int); bits > 0; bits -= 4)
    {
      ubyte nibble = (ubyte)((n >> (bits - 4)) & 0xf);
      if (nibble || nonzero)
        {
          nonzero = TRUE;

          if (nibble < 10)
            {
              obj->put(obj, nibble + '0');
            }
          else
            {
              obj->put(obj, nibble + a - 10);
            }
        }
    }

  if (!nonzero)
    {
      obj->put(obj, '0');
    }
}

/****************************************************************************
 * Name: utooct
 ****************************************************************************/

static void utooct(FAR struct lib_stream_s *obj, unsigned int n)
{
  unsigned int remainder = n & 0x7;
  unsigned int dividend = n >> 3;

  if (dividend)
    {
      utooct(obj, dividend);
    }

  obj->put(obj, (remainder + (unsigned int)'0'));
}

/****************************************************************************
 * Name: utobin
 ****************************************************************************/

static void utobin(FAR struct lib_stream_s *obj, unsigned int n)
{
  unsigned int remainder = n & 1;
  unsigned int dividend = n >> 1;

  if (dividend)
    {
      utobin(obj, dividend);
    }

  obj->put(obj, (remainder + (unsigned int)'0'));
}

/****************************************************************************
 * Name: lutoascii
 ****************************************************************************/

static void utoascii(FAR struct lib_stream_s *obj, ubyte fmt, ubyte flags, unsigned int n)
{
  /* Perform the integer conversion according to the format specifier */

  switch (fmt)
    {
      case 'd':
      case 'i':
        /* Signed base 10 */
        {
#ifdef CONFIG_NOPRINTF_FIELDWIDTH
          if ((int)n < 0)
            {
              obj->put(obj, '-');
              n = (unsigned int)(-(int)n);
            }
          else if (IS_SHOWPLUS(flags))
            {
              obj->put(obj, '+');
            }
#endif
          /* Convert the unsigned value to a string. */

          utodec(obj, n);
        }
        break;

      case 'u':
        /* Unigned base 10 */
        {
#ifdef CONFIG_NOPRINTF_FIELDWIDTH
          if (IS_SHOWPLUS(flags))
            {
              obj->put(obj, '+');
            }
#endif
          /* Convert the unsigned value to a string. */

          utodec(obj, n);
        }
        break;

#ifndef CONFIG_PTR_IS_NOT_INT
      case 'p':
#endif
      case 'x':
      case 'X':
        /* Hexadecimal */
        {
          /* Check for alternate form */

          if (IS_ALTFORM(flags))
            {
              /* Prefix the number with "0x" */

              obj->put(obj, '0');
              obj->put(obj, 'x');
            }

          /* Convert the unsigned value to a string. */

          if (fmt == 'X')
            {
              utohex(obj, n, 'A');
            }
          else
            {
              utohex(obj, n, 'a');
            }
        }
        break;

      case 'o':
        /* Octal */
         {
           /* Check for alternate form */

           if (IS_ALTFORM(flags))
             {
               /* Prefix the number with '0' */

               obj->put(obj, '0');
             }

           /* Convert the unsigned value to a string. */

           utooct(obj, n);
         }
         break;

      case 'b':
        /* Binary */
        {
          /* Convert the unsigned value to a string. */

          utobin(obj, n);
        }
        break;

#ifdef CONFIG_PTR_IS_NOT_INT
      case 'p':
#endif
      default:
        break;
    }
}

/****************************************************************************
 * Name: fixup
 ****************************************************************************/

#ifndef CONFIG_NOPRINTF_FIELDWIDTH
static void fixup(ubyte fmt, FAR ubyte *flags, FAR int *n)
{
  /* Perform the integer conversion according to the format specifier */

  switch (fmt)
    {
      case 'd':
      case 'i':
        /* Signed base 10 */

        if (*n < 0)
          {
            SET_NEGATE(*flags);
            CLR_SHOWPLUS(*flags);
            *n    = -*n;
          }
        break;

      case 'u':
        /* Unsigned base 10 */
        break;

      case 'p':
      case 'x':
      case 'X':
        /* Hexadecimal */
      case 'o':
        /* Octal */
      case 'b':
        /* Binary */
        CLR_SIGNED(*flags);
        break;

      default:
        break;
    }
}

/****************************************************************************
 * Name: getusize
 ****************************************************************************/

static int getusize(ubyte fmt, ubyte flags, unsigned int n)
{
  struct lib_stream_s nullstream;
  lib_nullstream(&nullstream);

  utoascii(&nullstream, fmt, flags, n);
  return nullstream.nput;
}
#endif /* CONFIG_NOPRINTF_FIELDWIDTH */

#ifdef CONFIG_LONG_IS_NOT_INT
/****************************************************************************
 * Name: lutodec
 ****************************************************************************/

static void lutodec(FAR struct lib_stream_s *obj, unsigned long n)
{
  unsigned int  remainder = n % 10;
  unsigned long dividend  = n / 10;

  if (dividend)
    {
      lutodec(obj, dividend);
    }

  obj->put(obj, (remainder + (unsigned int)'0'));
}

/****************************************************************************
 * Name: lutohex
 ****************************************************************************/

static void lutohex(FAR struct lib_stream_s *obj, unsigned long n, ubyte a)
{
  boolean nonzero = FALSE;
  ubyte bits;

  for (bits = 8*sizeof(unsigned long); bits > 0; bits -= 4)
    {
      ubyte nibble = (ubyte)((n >> (bits - 4)) & 0xf);
      if (nibble || nonzero)
        {
          nonzero = TRUE;

          if (nibble < 10)
            {
              obj->put(obj, nibble + '0');
            }
          else
            {
              obj->put(obj, nibble + a - 10);
            }
        }
    }

  if (!nonzero)
    {
      obj->put(obj, '0');
    }
}

/****************************************************************************
 * Name: lutooct
 ****************************************************************************/

static void lutooct(FAR struct lib_stream_s *obj, unsigned long n)
{
  unsigned int  remainder = n & 0x7;
  unsigned long dividend  = n >> 3;

  if (dividend)
    {
      lutooct(obj, dividend);
    }

  obj->put(obj, (remainder + (unsigned int)'0'));
}

/****************************************************************************
 * Name: lutobin
 ****************************************************************************/

static void lutobin(FAR struct lib_stream_s *obj, unsigned long n)
{
  unsigned int  remainder = n & 1;
  unsigned long dividend  = n >> 1;

  if (dividend)
    {
      lutobin(obj, dividend);
    }

  obj->put(obj, (remainder + (unsigned int)'0'));
}

/****************************************************************************
 * Name: lutoascii
 ****************************************************************************/

static void lutoascii(FAR struct lib_stream_s *obj, ubyte fmt, ubyte flags, unsigned long ln)
{
  /* Perform the integer conversion according to the format specifier */

  switch (fmt)
    {
      case 'd':
      case 'i':
        /* Signed base 10 */
        {
#ifdef CONFIG_NOPRINTF_FIELDWIDTH
          if ((long)ln < 0)
            {
              obj->put(obj, '-');
              ln    = (unsigned long)(-(long)ln);
            }
          else if (IS_SHOWPLUS(flags))
            {
              obj->put(obj, '+');
            }
#endif
          /* Convert the unsigned value to a string. */

          lutodec(obj, ln);
        }
        break;

      case 'u':
        /* Unigned base 10 */
        {
#ifdef CONFIG_NOPRINTF_FIELDWIDTH
          if (IS_SHOWPLUS(flags))
            {
              obj->put(obj, '+');
            }
#endif
          /* Convert the unsigned value to a string. */

          lutodec(obj, ln);
        }
        break;

      case 'x':
      case 'X':
        /* Hexadecimal */
        {
          /* Check for alternate form */

          if (IS_ALTFORM(flags))
            {
              /* Prefix the number with "0x" */

              obj->put(obj, '0');
              obj->put(obj, 'x');
            }

          /* Convert the unsigned value to a string. */

          if (fmt == 'X')
            {
              lutohex(obj, ln, 'A');
            }
          else
            {
              lutohex(obj, ln, 'a');
            }
        }
        break;

      case 'o':
        /* Octal */
         {
           /* Check for alternate form */

           if (IS_ALTFORM(flags))
             {
               /* Prefix the number with '0' */

               obj->put(obj, '0');
             }

           /* Convert the unsigned value to a string. */

           lutooct(obj, ln);
         }
         break;

      case 'b':
        /* Binary */
        {
          /* Convert the unsigned value to a string. */

          lutobin(obj, ln);
        }
        break;

      case 'p':
      default:
        break;
    }
}

/****************************************************************************
 * Name: lfixup
 ****************************************************************************/

#ifndef CONFIG_NOPRINTF_FIELDWIDTH
static void lfixup(ubyte fmt, FAR ubyte *flags, FAR long *ln)
{
  /* Perform the integer conversion according to the format specifier */

  switch (fmt)
    {
      case 'd':
      case 'i':
        /* Signed base 10 */

        if (*ln < 0)
          {
            SET_NEGATE(*flags);
            CLR_SHOWPLUS(*flags);
            *ln    = -*ln;
          }
        break;

      case 'u':
        /* Unsigned base 10 */
        break;

      case 'p':
      case 'x':
      case 'X':
        /* Hexadecimal */
      case 'o':
        /* Octal */
      case 'b':
        /* Binary */
        CLR_SIGNED(*flags);
        break;

      default:
        break;
    }
}

/****************************************************************************
 * Name: getlusize
 ****************************************************************************/

static int getlusize(ubyte fmt, ubyte flags, unsigned long ln)
{
  struct lib_stream_s nullstream;
  lib_nullstream(&nullstream);

  lutoascii(&nullstream, fmt, flags, ln);
  return nullstream.nput;
}

#endif /* CONFIG_NOPRINTF_FIELDWIDTH */
#endif /* CONFIG_LONG_IS_NOT_INT */

#ifdef CONFIG_HAVE_LONG_LONG
/****************************************************************************
 * Name: llutodec
 ****************************************************************************/

static void llutodec(FAR struct lib_stream_s *obj, unsigned long long n)
{
  unsigned int remainder = n % 10;
  unsigned long long dividend = n / 10;

  if (dividend)
    {
      llutodec(obj, dividend);
    }

  obj->put(obj, (remainder + (unsigned int)'0'));
}

/****************************************************************************
 * Name: llutohex
 ****************************************************************************/

static void llutohex(FAR struct lib_stream_s *obj, unsigned long long n, ubyte a)
{
  boolean nonzero = FALSE;
  ubyte bits;

  for (bits = 8*sizeof(unsigned long long); bits > 0; bits -= 4)
    {
      ubyte nibble = (ubyte)((n >> (bits - 4)) & 0xf);
      if (nibble || nonzero)
        {
          nonzero = TRUE;

          if (nibble < 10)
            {
              obj->put(obj, (nibble + '0'));
            }
          else
            {
              obj->put(obj, (nibble + a - 10));
            }
        }
    }

  if (!nonzero)
    {
      obj->put(obj, '0');
    }
}

/****************************************************************************
 * Name: llutooct
 ****************************************************************************/

static void llutooct(FAR struct lib_stream_s *obj, unsigned long long n)
{
  unsigned int remainder = n & 0x7;
  unsigned long long dividend = n >> 3;

  if (dividend)
    {
      llutooct(obj, dividend);
    }

  obj->put(obj, (remainder + (unsigned int)'0'));
}

/****************************************************************************
 * Name: llutobin
 ****************************************************************************/

static void llutobin(FAR struct lib_stream_s *obj, unsigned long long n)
{
  unsigned int remainder = n & 1;
  unsigned long long dividend = n >> 1;

  if (dividend)
    {
      llutobin(obj, dividend);
    }

  obj->put(obj, (remainder + (unsigned int)'0'));
}

/****************************************************************************
 * Name: llutoascii
 ****************************************************************************/

static void llutoascii(FAR struct lib_stream_s *obj, ubyte fmt, ubyte flags, unsigned long long lln)
{
  /* Perform the integer conversion according to the format specifier */

  switch (fmt)
    {
      case 'd':
      case 'i':
        /* Signed base 10 */
        {
#ifdef CONFIG_NOPRINTF_FIELDWIDTH
          if ((long long)lln < 0)
            {
              obj->put(obj, '-');
              lln    = (unsigned long long)(-(long long)lln);
            }
          else if (IS_SHOWPLUS(flags))
            {
              obj->put(obj, '+');
            }
#endif
          /* Convert the unsigned value to a string. */

          llutodec(obj, (unsigned long long)lln);
        }
        break;

      case 'u':
        /* Unigned base 10 */
        {
#ifdef CONFIG_NOPRINTF_FIELDWIDTH
          if (IS_SHOWPLUS(flags))
            {
              obj->put(obj, '+');
            }
#endif
          /* Convert the unsigned value to a string. */

          llutodec(obj, (unsigned long long)lln);
        }
        break;

      case 'x':
      case 'X':
        /* Hexadecimal */
        {
          /* Check for alternate form */

          if (IS_ALTFORM(flags))
            {
              /* Prefix the number with "0x" */

              obj->put(obj, '0');
              obj->put(obj, 'x');
            }

          /* Convert the unsigned value to a string. */

          if (fmt == 'X')
            {
              llutohex(obj, (unsigned long long)lln, 'A');
            }
          else
            {
              llutohex(obj, (unsigned long long)lln, 'a');
            }
        }
        break;

      case 'o':
        /* Octal */
         {
           /* Check for alternate form */

           if (IS_ALTFORM(flags))
             {
               /* Prefix the number with '0' */

               obj->put(obj, '0');
             }

           /* Convert the unsigned value to a string. */

           llutooct(obj, (unsigned long long)lln);
         }
         break;

      case 'b':
        /* Binary */
        {
          /* Convert the unsigned value to a string. */

          llutobin(obj, (unsigned long long)lln);
        }
        break;

      case 'p':
      default:
        break;
    }
}

/****************************************************************************
 * Name: llfixup
 ****************************************************************************/

#ifndef CONFIG_NOPRINTF_FIELDWIDTH
static void llfixup(ubyte fmt, FAR ubyte *flags, FAR long long *lln)
{
  /* Perform the integer conversion according to the format specifier */

  switch (fmt)
    {
      case 'd':
      case 'i':
        /* Signed base 10 */

        if (*lln < 0)
          {
            SET_NEGATE(*flags);
            CLR_SHOWPLUS(*flags);
            *lln    = -*lln;
          }
        break;

      case 'u':
        /* Unsigned base 10 */
        break;

      case 'p':
      case 'x':
      case 'X':
        /* Hexadecimal */
      case 'o':
        /* Octal */
      case 'b':
        /* Binary */
        CLR_SIGNED(*flags);
        break;

      default:
        break;
    }
}

/****************************************************************************
 * Name: getllusize
 ****************************************************************************/

static int getllusize(ubyte fmt, ubyte flags, unsigned long long lln)
{
  struct lib_stream_s nullstream;
  lib_nullstream(&nullstream);


  llutoascii(&nullstream, fmt, flags, lln);
  return nullstream.nput;
}

#endif /* CONFIG_NOPRINTF_FIELDWIDTH */
#endif /* CONFIG_HAVE_LONG_LONG */

/****************************************************************************
 * Name: prejustify
 ****************************************************************************/

#ifndef CONFIG_NOPRINTF_FIELDWIDTH
static void prejustify(FAR struct lib_stream_s *obj, ubyte fmt,
                       ubyte flags, int fieldwidth, int numwidth)
{
  int i;

  switch (fmt)
    {
      default:
      case FMT_RJUST:
        if (IS_SIGNED(flags))
          {
            numwidth++;
          }

        for (i = fieldwidth - numwidth; i > 0; i--)
          {
            obj->put(obj, ' ');
          }

        if (IS_NEGATE(flags))
          {
            obj->put(obj, '-');
          }
        else if (IS_SHOWPLUS(flags))
          {
            obj->put(obj, '+');
          }
        break;

      case FMT_RJUST0:
         if (IS_NEGATE(flags))
          {
            obj->put(obj, '-');
            numwidth++;
          }
        else if (IS_SHOWPLUS(flags))
          {
            obj->put(obj, '+');
            numwidth++;
          }

        for (i = fieldwidth - numwidth; i > 0; i--)
          {
            obj->put(obj, '0');
          }
        break;

      case FMT_LJUST:
         if (IS_NEGATE(flags))
          {
            obj->put(obj, '-');
          }
        else if (IS_SHOWPLUS(flags))
          {
            obj->put(obj, '+');
          }
        break;
    }
}
#endif

/****************************************************************************
 * Name: postjustify
 ****************************************************************************/

#ifndef CONFIG_NOPRINTF_FIELDWIDTH
static void postjustify(FAR struct lib_stream_s *obj, ubyte fmt,
                        ubyte flags, int fieldwidth, int numwidth)
{
  int i;

  /* Apply field justification to the integer value. */

  switch (fmt)
    {
      default:
      case FMT_RJUST:
      case FMT_RJUST0:
        break;

      case FMT_LJUST:
        if (IS_SIGNED(flags))
          {
            numwidth++;
          }

        for (i = fieldwidth - numwidth; i > 0; i--)
          {
            obj->put(obj, ' ');
          }
        break;
    }
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * lib_vsprintf
 ****************************************************************************/

int lib_vsprintf(FAR struct lib_stream_s *obj, const char *src, va_list ap)
{
  char           *ptmp;
#ifndef CONFIG_NOPRINTF_FIELDWIDTH
  int             width;
  int             trunc;
  ubyte           fmt;
#endif
  ubyte           flags;

  for (; *src; src++)
    {
      /* Just copy regular characters */

      if (*src != '%')
        {
           obj->put(obj, *src);
           continue;
        }

      /* We have found a format specifier. Move past it. */

      src++;

      /* Assume defaults */

      flags = 0;
#ifndef CONFIG_NOPRINTF_FIELDWIDTH
      fmt   = FMT_RJUST;
      width = 0;
      trunc = 0;
#endif

      /* Process each format qualifier. */

      for (; *src; src++)
        {
          /* Break out of the loop when the format is known. */

          if (strchr("diuxXpobeEfgGlLsc%", *src))
            {
              break;
            }

          /* Check for left justification. */

          else if (*src == '-')
            {
#ifndef CONFIG_NOPRINTF_FIELDWIDTH
              fmt = FMT_LJUST;
#endif
            }

          /* Check for leading zero fill right justification. */

          else if (*src == '0')
            {
#ifndef CONFIG_NOPRINTF_FIELDWIDTH
              fmt = FMT_RJUST0;
#endif
            }
#if 0
          /* Center justification. */

          else if (*src == '~')
            {
#ifndef CONFIG_NOPRINTF_FIELDWIDTH
              fmt = FMT_CENTER;
#endif
            }
#endif

          else if (*src == '*')
            {
#ifndef CONFIG_NOPRINTF_FIELDWIDTH
              int value = va_arg(ap, int);
              if (IS_HASDOT(flags))
                {
                  trunc = value;
                  SET_HASASTERISKTRUNC(flags);
                }
              else
                {
                  width = value;
                  SET_HASASTERISKWIDTH(flags);
                }
#endif
            }

          /* Check for field width */

          else if (*src >= '1' && *src <= '9')
            {
#ifdef CONFIG_NOPRINTF_FIELDWIDTH
              for (src++; (*src >= '0' && *src <= '9'); src++);
#else
              /* Accumulate the field width integer. */

              int n = ((int)(*src)) - (int)'0';
              for (src++; (*src >= '0' && *src <= '9'); src++)
                {
                  n = 10*n + (((int)(*src)) - (int)'0');
                }

              if (IS_HASDOT(flags))
                {
                  trunc = n;
                }
              else
                {
                  width = n;
                }
#endif
              /* Back up to the last digit. */

              src--;
            }

          /* Check for a decimal point. */

          else if (*src == '.')
            {
#ifndef CONFIG_NOPRINTF_FIELDWIDTH
              SET_HASDOT(flags);
#endif
            }

          /* Check for leading plus sign. */

          else if (*src == '+')
            {
              SET_SHOWPLUS(flags);
            }

          /* Check for alternate form. */

          else if (*src == '#')
            {
              SET_ALTFORM(flags);
            }
        }

      /* "%%" means that a literal '%' was intended (instead of a format
       * specification).
       */

      if (*src == '%')
        {
          obj->put(obj, '%');
          continue;
        }

      /* Check for the string format. */

      if (*src == 's')
        {
          /* Just concatenate the string into the output */

          ptmp = va_arg(ap, char *);
          if (!ptmp)
            {
              ptmp = (char*)g_nullstring;
            }

          while(*ptmp)
            {
              obj->put(obj, *ptmp);
              ptmp++;
            }
          continue;
        }

      /* Check for the character output */

      else if (*src == 'c')
        {
          /* Just copy the character into the output. */

          int n = va_arg(ap, int);
          obj->put(obj, n);
          continue;
        }

      /* Check for the long long prefix. */

      if (*src == 'L')
        {
           SET_LONGLONGPRECISION(flags);
           ++src;
        }
      else if (*src == 'l')
        {
          SET_LONGPRECISION(flags);
          if (*++src == 'l')
            {
              SET_LONGLONGPRECISION(flags);
              ++src;
            }
        }

      /* Handle integer conversions */

      if (strchr("diuxXpob", *src))
        {
#ifdef CONFIG_HAVE_LONG_LONG
          if (IS_LONGLONGPRECISION(flags) && *src != 'p')
            {
              long long lln;
#ifndef CONFIG_NOPRINTF_FIELDWIDTH
              int lluwidth;
#endif
              /* Extract the long long value. */

              lln = va_arg(ap, long long);

#ifdef CONFIG_NOPRINTF_FIELDWIDTH
              /* Output the number */

              llutoascii(obj, *src, flags, (unsigned long long)lln);
#else
              /* Resolve sign-ness and format issues */

              llfixup(*src, &flags, &lln);

              /* Get the width of the output */

              lluwidth = getllusize(*src, flags, lln);

              /* Perform left field justification actions */

              prejustify(obj, fmt, flags, width, lluwidth);

              /* Output the number */

              llutoascii(obj, *src, flags, (unsigned long long)lln);

              /* Perform right field justification actions */

              postjustify(obj, fmt, flags, width, lluwidth);
#endif
            }
          else
#endif /* CONFIG_HAVE_LONG_LONG */
#ifdef CONFIG_LONG_IS_NOT_INT
          if (IS_LONGPRECISION(flags) && *src != 'p')
            {
              long ln;
#ifndef CONFIG_NOPRINTF_FIELDWIDTH
              int luwidth;
#endif
              /* Extract the long value. */

              ln = va_arg(ap, long);

#ifdef CONFIG_NOPRINTF_FIELDWIDTH
              /* Output the number */

              lutoascii(obj, *src, flags, (unsigned long)ln);
#else
              /* Resolve sign-ness and format issues */

              lfixup(*src, &flags, &ln);

              /* Get the width of the output */

              luwidth = getlusize(*src, flags, ln);

              /* Perform left field justification actions */

              prejustify(obj, fmt, flags, width, luwidth);

              /* Output the number */

              lutoascii(obj, *src, flags, (unsigned long)ln);

              /* Perform right field justification actions */

              postjustify(obj, fmt, flags, width, luwidth);
#endif
            }
          else
#endif /* CONFIG_LONG_IS_NOT_INT */
#ifdef CONFIG_PTR_IS_NOT_INT
          if (*src == 'p')
            {
              void *p;
#ifndef CONFIG_NOPRINTF_FIELDWIDTH
              int pwidth;
#endif
              /* Extract the integer value. */

              p = va_arg(ap, void *);

#ifdef CONFIG_NOPRINTF_FIELDWIDTH
              /* Output the pointer value */

              ptohex(obj, flags, p);
#else
              /* Resolve sign-ness and format issues */

              lfixup(*src, &flags, &ln);

              /* Get the width of the output */

              luwidth = getpsize(*src, flags, p);

              /* Perform left field justification actions */

              prejustify(obj, fmt, flags, width, pwidth);

              /* Output the pointer value */

              ptohex(obj, flags, p);

              /* Perform right field justification actions */

              postjustify(obj, fmt, flags, width, pwidth);
#endif
            }
          else
#endif
            {
              int n;
#ifndef CONFIG_NOPRINTF_FIELDWIDTH
              int uwidth;
#endif
              /* Extract the long long value. */

              n = va_arg(ap, int);

#ifdef CONFIG_NOPRINTF_FIELDWIDTH
              /* Output the number */

              utoascii(obj, *src, flags, (unsigned int)n);
#else
              /* Resolve sign-ness and format issues */

              fixup(*src, &flags, &n);

              /* Get the width of the output */

              uwidth = getusize(*src, flags, n);

              /* Perform left field justification actions */

              prejustify(obj, fmt, flags, width, uwidth);

              /* Output the number */

              utoascii(obj, *src, flags, (unsigned int)n);

              /* Perform right field justification actions */

              postjustify(obj, fmt, flags, width, uwidth);
#endif
            }
        }

      /* Handle floating point conversions */

      else if (strchr("eEfgG", *src))
        {
#ifdef CONFIG_CPP_HAVE_WARNING
#  warning "No floating point support"
#endif
        }
    }

  return obj->nput;
}

