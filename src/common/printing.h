/* printing.h - module output (text or JSON) and the final rendering.

   Modules report through ffOutput/ffOutputError/ffOutputLine.  In text
   mode a line "key: value" is added to the info column, both key and value
   rendered from format strings over the module's arguments; ffPrintFinish
   then renders logo and lines side by side.  Every output line is
   assembled in full and written with one call (DECISIONS.md D3, D10).
   In --format json mode the arguments become the module's JSON result.  */

#ifndef FF_PRINTING_H
#define FF_PRINTING_H

#include <stdint.h>

#include "common/format.h"
#include "common/strbuf.h"
#include "modules/modules.h"

/* SGR helpers; no-ops in pipe mode.  sgr is e.g. "1;34".  */
void ffAppendColor (FFstrbuf *sb, const char *sgr);
void ffAppendReset (FFstrbuf *sb);

/* fastfetch's value styles: sizes per display.size (default binary units,
   two decimals: "7.09 GiB"), percentages coloured by display.percent
   thresholds (green below 50, yellow below 80, else red).  */
void ffAppendSize (FFstrbuf *sb, uint64_t bytes);
void ffAppendPercentValue (FFstrbuf *sb, double percent);

/* One result.  defaultKey and the key in args are format strings over the
   same arguments ("Disk ({name})"); a NULL key prints the value alone.  */
void ffOutput (const FFModuleArgs *m, const char *defaultKey, const char *defaultFormat,
               uint32_t n, const FFformatarg *args);
/* A failed detection: hidden unless --show-errors; {"error": ...} in JSON.  */
void ffOutputError (const FFModuleArgs *m, const char *defaultKey, const char *error);
/* A line with no data (Break, Separator, Colors): omitted from JSON.  */
void ffOutputLine (const char *text);

/* Visible width of a string, skipping ESC [ ... final-byte sequences.  */
uint32_t ffVisibleWidth (const char *s, uint32_t len);

void ffPrintFinish (void);

#endif
