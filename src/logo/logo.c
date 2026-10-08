/* logo.c - see logo.h.  */

#include "logo/logo.h"

#include <ctype.h>
#include <stdbool.h>
#include <string.h>

#include "common/printing.h"
#include "common/strbuf.h"

static int
nameEq (const char *a, const char *b)
{
  for (; *a && *b; a++, b++)
    if (tolower ((unsigned char) *a) != tolower ((unsigned char) *b))
      return 0;
  return *a == *b;
}

const FFlogo *
ffLogoFind (const char *name)
{
  uint32_t i;
  int n;

  for (i = 0; i < ffLogoBuiltinCount; i++)
    for (n = 0; n < FF_LOGO_MAX_NAMES && ffLogoBuiltins[i].names[n]; n++)
      if (nameEq (ffLogoBuiltins[i].names[n], name))
        return &ffLogoBuiltins[i];
  return NULL;
}

void
ffLogoRender (const FFlogo *logo, FFlist *lines, uint32_t *width)
{
  const char *p = logo->data;
  const char *color = NULL;     /* colour in effect, carried across lines */
  bool pending = false;         /* colour chosen but not yet emitted on this line */
  bool emitted = false;         /* this line has emitted a colour */
  FFstrbuf *line = NULL;
  uint32_t w = 0;

  /* Colours are emitted lazily, just before the next visible character,
     so a line that starts by switching colour does not first restate the
     carried one.  */
  *width = 0;
  for (;;)
    {
      if (!line)
        {
          line = ffListAdd (lines);
          ffStrbufInit (line);
          w = 0;
          pending = color != NULL;
          emitted = false;
        }
      if (*p == 0 || *p == '\n')
        {
          if (w > *width)
            *width = w;
          if (*p == 0)
            break;
          line = NULL;
          p++;
          continue;
        }
      if (*p == '$' && p[1] >= '1' && p[1] <= '9')
        {
          const char *c = logo->colors[p[1] - '1'];
          if (c && c != color)
            {
              color = c;
              pending = true;
            }
          p += 2;
          continue;
        }
      if (*p == '$' && p[1] == '$')
        p++;
      if (pending)
        {
          if (emitted)
            ffAppendReset (line);
          ffAppendColor (line, color);
          pending = false;
          emitted = true;
        }
      ffStrbufAppendC (line, *p++);
      w++;
    }
  /* A trailing newline in the data leaves an empty last line: drop it.  */
  if (lines->length && w == 0)
    {
      ffStrbufDestroy (ffListGet (lines, lines->length - 1));
      lines->length--;
    }
}
