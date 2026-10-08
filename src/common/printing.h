/* printing.h - building the info column and rendering it beside the logo.

   Modules add lines to instance.lines; ffPrintFinish() renders logo and
   lines side by side.  Every output line is assembled in full and written
   with one fputs (DECISIONS.md D3: the CRTL makes each fwrite item a record).  */

#ifndef FF_PRINTING_H
#define FF_PRINTING_H

#include <stdint.h>

#include "common/strbuf.h"

/* SGR helpers; no-ops in pipe mode.  sgr is e.g. "1;34".  */
void ffAppendColor (FFstrbuf *sb, const char *sgr);
void ffAppendReset (FFstrbuf *sb);

/* Start a new info line and return it.  */
FFstrbuf *ffPrintNewLine (void);

/* Start a "key: " line (key in the key colour, padded to keyWidth) and
   return it for the value.  */
FFstrbuf *ffPrintKey (const char *key);

void ffPrintKV (const char *key, const char *value);

/* A failed detection: hidden unless --show-errors.  */
void ffPrintError (const char *key, const char *error);

/* fastfetch's value styles: sizes in binary units with two decimals
   ("7.09 GiB", "512 B"), and percentages coloured green below 50, yellow
   below 80, red above ("45%").  */
void ffAppendSize (FFstrbuf *sb, uint64_t bytes);
void ffAppendPercent (FFstrbuf *sb, uint64_t part, uint64_t whole);
/* "{used} / {total} ({percent})" */
void ffAppendUsage (FFstrbuf *sb, uint64_t used, uint64_t total);

/* Visible width of a string, skipping ESC [ ... final-byte sequences.  */
uint32_t ffVisibleWidth (const char *s, uint32_t len);

void ffPrintFinish (void);

#endif
