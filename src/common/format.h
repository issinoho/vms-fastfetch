/* format.h - typed module values and fastfetch's format-string language.

   Each module describes its result as a list of named, typed arguments in
   upstream's order (so "{3}" and "{pretty-name}" both work as in
   fastfetch).  The same list renders text through a format string and, in
   --format json mode, becomes the module's JSON result (DECISIONS.md D13).

   Format syntax (as fastfetch):
     {N} {name} {}       argument N (1-based), by name, or the next one
     {N:len}             truncated to len characters
     {N<len} {N>len}     padded to len, left- or right-aligned
     {?N}...{?}          only if argument N is not empty
     {/N}...{/}          only if argument N is empty
     {#spec} {#}         colour (see ffColorParse), and reset
   Anything else is copied as is.  */

#ifndef FF_FORMAT_H
#define FF_FORMAT_H

#include <stdbool.h>
#include <stdint.h>

#include "common/strbuf.h"

typedef enum FFargType
{
  FF_ARG_STRING,                /* s */
  FF_ARG_UINT,                  /* u */
  FF_ARG_DOUBLE,                /* d, two decimals */
  FF_ARG_BOOL,                  /* u != 0 */
  FF_ARG_SIZE,                  /* u bytes: "4.18 GiB" (display.size) */
  FF_ARG_PERCENT,               /* d: "59%", coloured by display.percent */
  FF_ARG_GHZ,                   /* d: "2.00 GHz" */
  FF_ARG_RAW                    /* s: pre-rendered text (colours); not in JSON */
} FFargType;

typedef struct FFformatarg
{
  const char *name;
  FFargType type;
  const char *s;
  uint64_t u;
  double d;
} FFformatarg;

FFformatarg ffArgS (const char *name, const char *s);
FFformatarg ffArgU (const char *name, uint64_t u);
FFformatarg ffArgD (const char *name, double d);
FFformatarg ffArgB (const char *name, bool b);
FFformatarg ffArgSize (const char *name, uint64_t bytes);
FFformatarg ffArgPercent (const char *name, double percent);
FFformatarg ffArgGhz (const char *name, double ghz);
FFformatarg ffArgRaw (const char *name, const char *s);

/* Empty for {?}/{/}: empty strings, zero numbers, false.  */
bool ffArgIsEmpty (const FFformatarg *a);
void ffArgRender (FFstrbuf *out, const FFformatarg *a);
void ffFormatApply (FFstrbuf *out, const char *format, uint32_t n, const FFformatarg *args);

/* Colour spec -> SGR parameters, as fastfetch's color options:
   "blue", "bold_red", "light_green" / "bright_green", "dim_", "italic_",
   "underline_", "blink_", "inverse_", "reset_" prefixes, raw "38;5;208",
   "#ff8000" (24-bit).  Returns false (out = "") if unrecognised.  */
bool ffColorParse (const char *spec, char *out, uint32_t outSize);

#endif
