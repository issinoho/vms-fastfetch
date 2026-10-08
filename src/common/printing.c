/* printing.c - see printing.h.  */

#include "common/printing.h"

#include <stdio.h>
#include <string.h>

#include "fastfetch.h"
#include "vms/vmsapi.h"

void
ffAppendColor (FFstrbuf *sb, const char *sgr)
{
  if (instance.options.pipe || !sgr || !*sgr)
    return;
  ffStrbufAppendS (sb, "\033[");
  ffStrbufAppendS (sb, sgr);
  ffStrbufAppendC (sb, 'm');
}

void
ffAppendReset (FFstrbuf *sb)
{
  if (!instance.options.pipe)
    ffStrbufAppendS (sb, "\033[0m");
}

FFstrbuf *
ffPrintNewLine (void)
{
  FFstrbuf *line = ffListAdd (&instance.lines);
  ffStrbufInit (line);
  return line;
}

FFstrbuf *
ffPrintKey (const char *key)
{
  FFstrbuf *line = ffPrintNewLine ();
  uint32_t width;

  ffAppendColor (line, instance.keyColor);
  ffStrbufAppendS (line, key);
  ffAppendReset (line);
  ffStrbufAppendS (line, instance.options.keySeparator);
  width = (uint32_t) strlen (key) + (uint32_t) strlen (instance.options.keySeparator);
  if (instance.options.keyWidth > width)
    ffStrbufAppendRepeat (line, ' ', instance.options.keyWidth - width);
  return line;
}

void
ffPrintKV (const char *key, const char *value)
{
  ffStrbufAppendS (ffPrintKey (key), value);
}

void
ffPrintError (const char *key, const char *error)
{
  FFstrbuf *line;

  if (!instance.options.showErrors)
    return;
  line = ffPrintKey (key);
  ffAppendColor (line, "31");
  ffStrbufAppendS (line, error);
  ffAppendReset (line);
}

uint32_t
ffVisibleWidth (const char *s, uint32_t len)
{
  uint32_t i = 0, w = 0;

  while (i < len)
    {
      if (s[i] == '\033' && i + 1 < len && s[i + 1] == '[')
        {
          /* CSI: parameters and intermediates, then one final byte @..~ */
          i += 2;
          while (i < len && !(s[i] >= 0x40 && s[i] <= 0x7E))
            i++;
          i++;
          continue;
        }
      w++;
      i++;
    }
  return w;
}

void
ffPrintFinish (void)
{
  const FFOptions *opt = &instance.options;
  FFlist logoLines;
  uint32_t logoWidth = 0, top = 0, rows, i;
  FFstrbuf out;
  bool term;

  ffListInit (&logoLines, sizeof (FFstrbuf));
  if (instance.logo)
    {
      ffLogoRender (instance.logo, &logoLines, &logoWidth);
      top = opt->logoPaddingTop;
    }
  rows = instance.lines.length;
  if (logoLines.length + top > rows)
    rows = logoLines.length + top;

  /* D10: on a terminal, bypass the driver's wrapping (it counts escape
     sequences); no-format writes get no carriage control, hence CR LF.  */
  term = ffVmsTermOpen ();
  if (term)
    fflush (stdout);

  ffStrbufInit (&out);
  for (i = 0; i < rows; i++)
    {
      ffStrbufClear (&out);
      if (logoLines.length)
        {
          uint32_t used = 0;
          ffStrbufAppendRepeat (&out, ' ', opt->logoPaddingLeft);
          if (i >= top && i - top < logoLines.length)
            {
              FFstrbuf *l = ffListGet (&logoLines, i - top);
              ffStrbufAppend (&out, l);
              ffAppendReset (&out);
              used = ffVisibleWidth (l->chars, l->length);
            }
          ffStrbufAppendRepeat (&out, ' ', logoWidth - used + opt->logoPaddingRight);
        }
      if (i < instance.lines.length)
        ffStrbufAppend (&out, ffListGet (&instance.lines, i));
      ffStrbufTrimRight (&out, ' ');
      if (term)
        {
          ffStrbufAppendS (&out, "\r\n");
          if (ffVmsTermWrite (out.chars, out.length))
            continue;
          term = false;         /* write failed: fall back to stdout */
          out.length -= 2;
        }
      ffStrbufAppendC (&out, '\n');
      fputs (out.chars, stdout);
    }
  fflush (stdout);
  ffStrbufDestroy (&out);

  for (i = 0; i < logoLines.length; i++)
    ffStrbufDestroy (ffListGet (&logoLines, i));
  ffListDestroy (&logoLines);
  for (i = 0; i < instance.lines.length; i++)
    ffStrbufDestroy (ffListGet (&instance.lines, i));
  ffListDestroy (&instance.lines);
}
