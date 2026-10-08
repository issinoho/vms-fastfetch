/* logo.h - builtin ASCII logos.

   Logo text uses fastfetch's markup: "$1".."$9" switch to colour N, "$$" is
   a literal "$".  The table is generated from the .txt files in src/logo/ascii and
   src/logo/logos.def by tools/gen_logos.py into logo_builtin.c.  */

#ifndef FF_LOGO_H
#define FF_LOGO_H

#include <stdint.h>

#include "common/list.h"

#define FF_LOGO_MAX_NAMES 4
#define FF_LOGO_MAX_COLORS 9

typedef struct FFlogo
{
  const char *names[FF_LOGO_MAX_NAMES];     /* first is canonical; NULL-terminated */
  const char *data;
  const char *colors[FF_LOGO_MAX_COLORS];   /* SGR parameters for $1..$9 */
  const char *colorKeys;
  const char *colorTitle;
} FFlogo;

extern const FFlogo ffLogoBuiltins[];
extern const uint32_t ffLogoBuiltinCount;

/* Case-insensitive lookup by any name; NULL if unknown.  */
const FFlogo *ffLogoFind (const char *name);

/* Render the logo into lines (FFstrbuf, colours applied unless --pipe) and
   report the widest line's visible width.  */
void ffLogoRender (const FFlogo *logo, FFlist *lines, uint32_t *width);

#endif
