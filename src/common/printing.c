/* printing.c - see printing.h.  */

#include "common/printing.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fastfetch.h"
#include "vms/vmsapi.h"
#include "yyjson.h"

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

void
ffAppendSize (FFstrbuf *sb, uint64_t bytes)
{
  static const char *iec[] = { "KiB", "MiB", "GiB", "TiB", "PiB", "EiB" };
  static const char *si[] = { "kB", "MB", "GB", "TB", "PB", "EB" };
  static const char *jedec[] = { "KB", "MB", "GB", "TB", "PB", "EB" };
  const FFOptions *o = &instance.options;
  const char **units = o->sizePrefix == FF_SIZE_SI ? si : o->sizePrefix == FF_SIZE_JEDEC ? jedec : iec;
  double base = o->sizePrefix == FF_SIZE_SI ? 1000.0 : 1024.0;
  double v = (double) bytes;
  int u = -1;

  if (v < base)
    {
      ffStrbufAppendF (sb, "%lu B", (unsigned long) bytes);
      return;
    }
  while (v >= base && u < 5)
    v /= base, u++;
  ffStrbufAppendF (sb, "%.*f %s", (int) o->sizeDigits, v, units[u]);
}

void
ffAppendPercentValue (FFstrbuf *sb, double percent)
{
  const FFOptions *o = &instance.options;
  unsigned pct = (unsigned) (percent + 0.5);

  ffAppendColor (sb, pct < o->percentGreen ? "32" : pct < o->percentYellow ? "33" : "31");
  ffStrbufAppendF (sb, "%u%%", pct);
  ffAppendReset (sb);
}

static FFstrbuf *
newLine (void)
{
  FFstrbuf *line = ffListAdd (&instance.lines);
  ffStrbufInit (line);
  return line;
}

/* --- JSON mode ------------------------------------------------------- */

/* "pretty-name" -> "prettyName" (fastfetch's JSON keys are camelCase).  */
static void
camel (char *out, size_t size, const char *name)
{
  size_t j = 0;
  bool up = false;
  for (; *name && j + 1 < size; name++)
    {
      if (*name == '-' || *name == '_')
        {
          up = true;
          continue;
        }
      out[j++] = up ? (char) toupper ((unsigned char) *name) : *name;
      up = false;
    }
  out[j] = 0;
}

/* The current module's entry {"type": ...}, created on first use.  */
static yyjson_mut_val *
jsonEntry (void)
{
  yyjson_mut_doc *doc = instance.jsonDoc;
  yyjson_mut_val *e = instance.jsonEntry;
  if (!e)
    {
      e = yyjson_mut_obj (doc);
      yyjson_mut_obj_add_str (doc, e, "type", instance.currentModule);
      yyjson_mut_arr_append (instance.jsonArray, e);
      instance.jsonEntry = e;
    }
  return e;
}

static void
jsonResult (uint32_t n, const FFformatarg *args)
{
  yyjson_mut_doc *doc = instance.jsonDoc;
  yyjson_mut_val *entry = jsonEntry ();
  yyjson_mut_val *obj = yyjson_mut_obj (doc);
  yyjson_mut_val *prev = yyjson_mut_obj_get (entry, "result");
  uint32_t i;

  for (i = 0; i < n; i++)
    {
      const FFformatarg *a = &args[i];
      char key[64];
      yyjson_mut_val *v;
      if (a->type == FF_ARG_RAW)
        continue;
      camel (key, sizeof key, a->name);
      switch (a->type)
        {
        case FF_ARG_STRING: v = yyjson_mut_strcpy (doc, a->s); break;
        case FF_ARG_BOOL: v = yyjson_mut_bool (doc, a->u != 0); break;
        case FF_ARG_DOUBLE:
        case FF_ARG_PERCENT:
        case FF_ARG_GHZ: v = yyjson_mut_real (doc, a->d); break;
        default: v = yyjson_mut_uint (doc, a->u); break;
        }
      yyjson_mut_obj_add (obj, yyjson_mut_strcpy (doc, key), v);
    }
  /* Multi-instance modules (Disk, LocalIp): the result becomes an array.  */
  if (!prev)
    yyjson_mut_obj_add_val (doc, entry, "result", obj);
  else if (yyjson_mut_is_arr (prev))
    yyjson_mut_arr_append (prev, obj);
  else
    {
      yyjson_mut_val *arr = yyjson_mut_arr (doc);
      yyjson_mut_arr_append (arr, yyjson_mut_obj_remove_key (entry, "result"));
      yyjson_mut_arr_append (arr, obj);
      yyjson_mut_obj_add_val (doc, entry, "result", arr);
    }
}

/* --- output ---------------------------------------------------------- */

static void
appendKey (FFstrbuf *line, const FFModuleArgs *m, const char *keyFormat,
           uint32_t n, const FFformatarg *args)
{
  FFstrbuf key;
  uint32_t width;

  ffStrbufInit (&key);
  ffFormatApply (&key, keyFormat, n, args);
  ffAppendColor (line, m && m->keyColor[0] ? m->keyColor : instance.keyColor);
  ffStrbufAppend (line, &key);
  ffAppendReset (line);
  ffStrbufAppendS (line, instance.options.keySeparator);
  width = ffVisibleWidth (key.chars, key.length) + (uint32_t) strlen (instance.options.keySeparator);
  if (instance.options.keyWidth > width)
    ffStrbufAppendRepeat (line, ' ', instance.options.keyWidth - width);
  ffStrbufDestroy (&key);
}

void
ffOutput (const FFModuleArgs *m, const char *defaultKey, const char *defaultFormat,
          uint32_t n, const FFformatarg *args)
{
  const char *keyFormat = m && m->key ? m->key : defaultKey;
  const char *outColor = m && m->outputColor[0] ? m->outputColor : instance.outputColor;
  FFstrbuf *line;

  if (instance.options.json)
    {
      jsonResult (n, args);
      return;
    }
  line = newLine ();
  if (keyFormat && *keyFormat)
    appendKey (line, m, keyFormat, n, args);
  ffAppendColor (line, outColor);
  ffFormatApply (line, m && m->format ? m->format : defaultFormat, n, args);
  if (outColor && *outColor)
    ffAppendReset (line);
}

void
ffOutputError (const FFModuleArgs *m, const char *defaultKey, const char *error)
{
  FFstrbuf *line;

  if (instance.options.json)
    {
      yyjson_mut_val *e = jsonEntry ();
      if (!yyjson_mut_obj_get (e, "error"))
        yyjson_mut_obj_add_strcpy (instance.jsonDoc, e, "error", error);
      return;
    }
  if (!instance.options.showErrors)
    return;
  line = newLine ();
  appendKey (line, m, m && m->key ? m->key : defaultKey ? defaultKey : instance.currentModule, 0, NULL);
  ffAppendColor (line, "31");
  ffStrbufAppendS (line, error);
  ffAppendReset (line);
}

void
ffOutputLine (const char *text)
{
  if (!instance.options.json)
    ffStrbufAppendS (newLine (), text);
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

static void
printJson (void)
{
  char *text = yyjson_mut_write (instance.jsonDoc, YYJSON_WRITE_PRETTY, NULL);

  /* One fputs: the CRTL makes a record of each line (D3).  */
  if (text)
    {
      fputs (text, stdout);
      fputs ("\n", stdout);
      free (text);
    }
  fflush (stdout);
}

void
ffPrintFinish (void)
{
  const FFOptions *opt = &instance.options;
  FFlist logoLines;
  uint32_t logoWidth = 0, top = 0, rows, i;
  FFstrbuf out;
  bool term;

  if (opt->json)
    {
      printJson ();
      return;
    }

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
     sequences); ffVmsTermWrite frames each line as the driver frames a
     record, so it sits correctly between DCL and CRTL output.  */
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
          if (ffVmsTermWrite (out.chars, out.length))
            continue;
          term = false;         /* write failed: fall back to stdout */
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
