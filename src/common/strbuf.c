/* strbuf.c - growable string buffer (see strbuf.h).  */

#include "common/strbuf.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char empty[1] = "";

void
ffStrbufInit (FFstrbuf *sb)
{
  sb->chars = empty;
  sb->length = 0;
  sb->allocated = 0;
}

void
ffStrbufInitS (FFstrbuf *sb, const char *s)
{
  ffStrbufInit (sb);
  ffStrbufAppendS (sb, s);
}

void
ffStrbufDestroy (FFstrbuf *sb)
{
  if (sb->allocated)
    free (sb->chars);
  ffStrbufInit (sb);
}

void
ffStrbufClear (FFstrbuf *sb)
{
  sb->length = 0;
  if (sb->allocated)
    sb->chars[0] = 0;
}

void
ffStrbufEnsureFree (FFstrbuf *sb, uint32_t free_)
{
  uint32_t need = sb->length + free_ + 1;
  uint32_t cap;
  char *p;

  if (need <= sb->allocated)
    return;
  cap = sb->allocated ? sb->allocated : 32;
  while (cap < need)
    cap *= 2;
  p = sb->allocated ? realloc (sb->chars, cap) : malloc (cap);
  if (!p)
    {
      fputs ("fastfetch: out of memory\n", stderr);
      exit (EXIT_FAILURE);
    }
  if (!sb->allocated)
    p[0] = 0;
  sb->chars = p;
  sb->allocated = cap;
}

void
ffStrbufAppendNS (FFstrbuf *sb, uint32_t n, const char *s)
{
  if (n == 0)
    return;
  ffStrbufEnsureFree (sb, n);
  memcpy (sb->chars + sb->length, s, n);
  sb->length += n;
  sb->chars[sb->length] = 0;
}

void
ffStrbufAppendS (FFstrbuf *sb, const char *s)
{
  if (s)
    ffStrbufAppendNS (sb, (uint32_t) strlen (s), s);
}

void
ffStrbufAppend (FFstrbuf *sb, const FFstrbuf *other)
{
  ffStrbufAppendNS (sb, other->length, other->chars);
}

void
ffStrbufAppendC (FFstrbuf *sb, char c)
{
  ffStrbufAppendNS (sb, 1, &c);
}

void
ffStrbufAppendRepeat (FFstrbuf *sb, char c, uint32_t n)
{
  if (n == 0)
    return;
  ffStrbufEnsureFree (sb, n);
  memset (sb->chars + sb->length, c, n);
  sb->length += n;
  sb->chars[sb->length] = 0;
}

void
ffStrbufAppendVF (FFstrbuf *sb, const char *fmt, va_list ap)
{
  va_list aq;
  int n;

  va_copy (aq, ap);
  n = vsnprintf (NULL, 0, fmt, aq);
  va_end (aq);
  if (n <= 0)
    return;
  ffStrbufEnsureFree (sb, (uint32_t) n);
  vsnprintf (sb->chars + sb->length, (size_t) n + 1, fmt, ap);
  sb->length += (uint32_t) n;
}

void
ffStrbufAppendF (FFstrbuf *sb, const char *fmt, ...)
{
  va_list ap;
  va_start (ap, fmt);
  ffStrbufAppendVF (sb, fmt, ap);
  va_end (ap);
}

void
ffStrbufSetS (FFstrbuf *sb, const char *s)
{
  ffStrbufClear (sb);
  ffStrbufAppendS (sb, s);
}

void
ffStrbufSetNS (FFstrbuf *sb, uint32_t n, const char *s)
{
  ffStrbufClear (sb);
  ffStrbufAppendNS (sb, n, s);
}

void
ffStrbufTrimRight (FFstrbuf *sb, char c)
{
  while (sb->length > 0
         && (sb->chars[sb->length - 1] == c || sb->chars[sb->length - 1] == 0))
    sb->length--;
  if (sb->allocated)
    sb->chars[sb->length] = 0;
}

void
ffStrbufCollapseSpaces (FFstrbuf *sb)
{
  uint32_t i, j = 0;

  if (!sb->allocated)
    return;
  for (i = 0; i < sb->length; i++)
    if (!(sb->chars[i] == ' ' && j > 0 && sb->chars[j - 1] == ' '))
      sb->chars[j++] = sb->chars[i];
  sb->length = j;
  sb->chars[j] = 0;
}

bool
ffStrbufEqualIgnCaseS (const FFstrbuf *sb, const char *s)
{
  uint32_t i;

  for (i = 0; i < sb->length; i++, s++)
    if (*s == 0 || tolower ((unsigned char) sb->chars[i]) != tolower ((unsigned char) *s))
      return false;
  return *s == 0;
}
