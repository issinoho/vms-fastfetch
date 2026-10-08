/* strbuf.h - growable NUL-terminated string buffer.

   Same role and naming as fastfetch's FFstrbuf, without the cleanup
   attribute: every buffer that is initialised must be destroyed.
   An initialised, empty buffer points at a shared "" and owns no memory.  */

#ifndef FF_STRBUF_H
#define FF_STRBUF_H

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct FFstrbuf
{
  char *chars;
  uint32_t length;
  uint32_t allocated;           /* 0: chars is the shared empty string */
} FFstrbuf;

void ffStrbufInit (FFstrbuf *sb);
void ffStrbufInitS (FFstrbuf *sb, const char *s);
void ffStrbufDestroy (FFstrbuf *sb);
void ffStrbufClear (FFstrbuf *sb);
void ffStrbufEnsureFree (FFstrbuf *sb, uint32_t free);

void ffStrbufAppendNS (FFstrbuf *sb, uint32_t n, const char *s);
void ffStrbufAppendS (FFstrbuf *sb, const char *s);
void ffStrbufAppend (FFstrbuf *sb, const FFstrbuf *other);
void ffStrbufAppendC (FFstrbuf *sb, char c);
void ffStrbufAppendRepeat (FFstrbuf *sb, char c, uint32_t n);
void ffStrbufAppendF (FFstrbuf *sb, const char *fmt, ...);
void ffStrbufAppendVF (FFstrbuf *sb, const char *fmt, va_list ap);

void ffStrbufSetS (FFstrbuf *sb, const char *s);
void ffStrbufSetNS (FFstrbuf *sb, uint32_t n, const char *s);

/* Remove trailing c (and NULs); collapse runs of blanks to one ("HP rx2660  (...)").  */
void ffStrbufTrimRight (FFstrbuf *sb, char c);
void ffStrbufCollapseSpaces (FFstrbuf *sb);

bool ffStrbufEqualIgnCaseS (const FFstrbuf *sb, const char *s);

static inline bool
ffStrbufIsEmpty (const FFstrbuf *sb)
{
  return sb->length == 0;
}

#endif
