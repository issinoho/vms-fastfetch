/* format.c - see format.h.  */

#include "common/format.h"

#include <ctype.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common/printing.h"
#include "fastfetch.h"

static FFformatarg
arg (const char *name, FFargType type, const char *s, uint64_t u, double d)
{
  FFformatarg a;
  a.name = name;
  a.type = type;
  a.s = s ? s : "";
  a.u = u;
  a.d = d;
  return a;
}

FFformatarg ffArgS (const char *n, const char *s) { return arg (n, FF_ARG_STRING, s, 0, 0); }
FFformatarg ffArgU (const char *n, uint64_t u) { return arg (n, FF_ARG_UINT, NULL, u, 0); }
FFformatarg ffArgD (const char *n, double d) { return arg (n, FF_ARG_DOUBLE, NULL, 0, d); }
FFformatarg ffArgB (const char *n, bool b) { return arg (n, FF_ARG_BOOL, NULL, b, 0); }
FFformatarg ffArgSize (const char *n, uint64_t b) { return arg (n, FF_ARG_SIZE, NULL, b, 0); }
FFformatarg ffArgPercent (const char *n, double p) { return arg (n, FF_ARG_PERCENT, NULL, 0, p); }
FFformatarg ffArgGhz (const char *n, double g) { return arg (n, FF_ARG_GHZ, NULL, 0, g); }
FFformatarg ffArgRaw (const char *n, const char *s) { return arg (n, FF_ARG_RAW, s, 0, 0); }

bool
ffArgIsEmpty (const FFformatarg *a)
{
  switch (a->type)
    {
    case FF_ARG_STRING:
    case FF_ARG_RAW:
      return a->s[0] == 0;
    case FF_ARG_DOUBLE:
    case FF_ARG_GHZ:
      return a->d == 0;
    case FF_ARG_PERCENT:
      return false;             /* 0% is a value */
    default:
      return a->u == 0;
    }
}

void
ffArgRender (FFstrbuf *out, const FFformatarg *a)
{
  switch (a->type)
    {
    case FF_ARG_STRING:
    case FF_ARG_RAW:
      ffStrbufAppendS (out, a->s);
      break;
    case FF_ARG_UINT:
      ffStrbufAppendF (out, "%" PRIu64, a->u);
      break;
    case FF_ARG_DOUBLE:
      ffStrbufAppendF (out, "%.2f", a->d);
      break;
    case FF_ARG_BOOL:
      ffStrbufAppendS (out, a->u ? "true" : "false");
      break;
    case FF_ARG_SIZE:
      ffAppendSize (out, a->u);
      break;
    case FF_ARG_PERCENT:
      ffAppendPercentValue (out, a->d);
      break;
    case FF_ARG_GHZ:
      ffStrbufAppendF (out, "%.2f GHz", a->d);
      break;
    }
}

static const FFformatarg *
findArg (const char *ref, uint32_t len, uint32_t *next, uint32_t n, const FFformatarg *args)
{
  uint32_t i;

  if (len == 0)                 /* "{}": the next argument */
    return *next < n ? &args[(*next)++] : NULL;
  if (isdigit ((unsigned char) ref[0]))
    {
      unsigned long idx = strtoul (ref, NULL, 10);
      return idx >= 1 && idx <= n ? &args[idx - 1] : NULL;
    }
  for (i = 0; i < n; i++)
    if (strlen (args[i].name) == len)
      {
        uint32_t k;
        for (k = 0; k < len; k++)
          if (tolower ((unsigned char) args[i].name[k]) != tolower ((unsigned char) ref[k]))
            break;
        if (k == len)
          return &args[i];
      }
  return NULL;
}

/* Skip to the matching close tag ("{?}" or "{/}"), honouring nesting.  */
static const char *
skipBlock (const char *p, char kind)
{
  int depth = 1;
  while (*p)
    {
      if (p[0] == '{' && p[1] == kind)
        {
          if (p[2] == '}')
            {
              if (--depth == 0)
                return p + 3;
            }
          else
            depth++;
        }
      p++;
    }
  return p;
}

void
ffFormatApply (FFstrbuf *out, const char *fmt, uint32_t n, const FFformatarg *args)
{
  const char *p = fmt;
  uint32_t next = 0;

  while (*p)
    {
      const char *close;
      const char *body;
      uint32_t blen;

      if (*p != '{' || !(close = strchr (p + 1, '}')))
        {
          ffStrbufAppendC (out, *p++);
          continue;
        }
      body = p + 1;
      blen = (uint32_t) (close - body);

      if (body[0] == '?' || body[0] == '/')
        {
          const FFformatarg *a;
          char kind = body[0];
          if (blen == 1)        /* a closing tag reached normally */
            {
              p = close + 1;
              continue;
            }
          a = findArg (body + 1, blen - 1, &next, n, args);
          if (!a || ffArgIsEmpty (a) != (kind == '/'))
            p = skipBlock (close + 1, kind);
          else
            p = close + 1;
          continue;
        }
      if (body[0] == '#')
        {
          char spec[64], sgr[64];
          if (blen == 1)
            ffAppendReset (out);
          else if (blen - 1 < sizeof spec)
            {
              memcpy (spec, body + 1, blen - 1);
              spec[blen - 1] = 0;
              if (strcmp (spec, "keys") == 0)
                ffAppendColor (out, instance.keyColor);
              else if (strcmp (spec, "title") == 0)
                ffAppendColor (out, instance.titleColor);
              else if (ffColorParse (spec, sgr, sizeof sgr))
                ffAppendColor (out, sgr);
            }
          p = close + 1;
          continue;
        }

      {
        /* {ref}, {ref:len}, {ref<len}, {ref>len}  */
        uint32_t rlen = 0, width = 0;
        char op = 0;
        const FFformatarg *a;
        while (rlen < blen && body[rlen] != ':' && body[rlen] != '<' && body[rlen] != '>')
          rlen++;
        if (rlen < blen)
          {
            op = body[rlen];
            width = (uint32_t) strtoul (body + rlen + 1, NULL, 10);
          }
        a = findArg (body, rlen, &next, n, args);
        if (!a)
          {
            /* Not an argument reference: copy the text literally.  */
            ffStrbufAppendNS (out, blen + 2, p);
            p = close + 1;
            continue;
          }
        if (!op)
          ffArgRender (out, a);
        else
          {
            FFstrbuf v;
            uint32_t vis;
            ffStrbufInit (&v);
            ffArgRender (&v, a);
            vis = ffVisibleWidth (v.chars, v.length);
            if (op == ':' && vis > width && v.length == vis)
              v.length = width, v.chars[width] = 0;
            if (op == '>' && vis < width)
              ffStrbufAppendRepeat (out, ' ', width - vis);
            ffStrbufAppend (out, &v);
            if (op == '<' && vis < width)
              ffStrbufAppendRepeat (out, ' ', width - vis);
            ffStrbufDestroy (&v);
          }
        p = close + 1;
      }
    }
}

bool
ffColorParse (const char *spec, char *out, uint32_t outSize)
{
  static const char *names[] = { "black", "red", "green", "yellow", "blue",
                                 "magenta", "cyan", "white", NULL };
  static const struct { const char *prefix, *sgr; } mods[] = {
    { "reset_", "0" }, { "bold_", "1" }, { "dim_", "2" }, { "italic_", "3" },
    { "underline_", "4" }, { "blink_", "5" }, { "inverse_", "7" },
    { "hidden_", "8" }, { "strike_", "9" }, { NULL, NULL } };
  FFstrbuf sgr;
  const char *p = spec;
  bool bright = false, ok = false;
  int i;

  out[0] = 0;
  ffStrbufInit (&sgr);
  for (;;)
    {
      for (i = 0; mods[i].prefix; i++)
        if (strncmp (p, mods[i].prefix, strlen (mods[i].prefix)) == 0)
          break;
      if (!mods[i].prefix)
        break;
      if (sgr.length)
        ffStrbufAppendC (&sgr, ';');
      ffStrbufAppendS (&sgr, mods[i].sgr);
      p += strlen (mods[i].prefix);
    }
  if (strncmp (p, "light_", 6) == 0 || strncmp (p, "bright_", 7) == 0)
    {
      bright = true;
      p = strchr (p, '_') + 1;
    }
  if (sgr.length && *p)
    ffStrbufAppendC (&sgr, ';');
  if (*p == '#' && strlen (p) == 7)
    {
      unsigned long rgb = strtoul (p + 1, NULL, 16);
      ffStrbufAppendF (&sgr, "38;2;%lu;%lu;%lu", (rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF);
      ok = true;
    }
  else if (isdigit ((unsigned char) *p))
    {
      const char *q = p;
      while (*q && (isdigit ((unsigned char) *q) || *q == ';'))
        q++;
      if (*q == 0)
        {
          ffStrbufAppendS (&sgr, p);
          ok = true;
        }
    }
  else if (strcmp (p, "default") == 0)
    {
      ffStrbufAppendS (&sgr, "39");
      ok = true;
    }
  else if (*p == 0 && sgr.length)
    ok = true;                  /* just modifiers: "bold_" */
  else
    for (i = 0; names[i]; i++)
      if (strcmp (p, names[i]) == 0)
        {
          ffStrbufAppendF (&sgr, "%d", (bright ? 90 : 30) + i);
          ok = true;
          break;
        }
  if (ok && sgr.length < outSize)
    memcpy (out, sgr.chars, sgr.length + 1);
  else
    ok = false;
  ffStrbufDestroy (&sgr);
  return ok;
}
