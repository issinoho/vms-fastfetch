/* config.c - see config.h.

   The keys understood are the subset of fastfetch's schema that applies on
   OpenVMS (docs/CONFIG.md); others are ignored, so upstream configs load.
   Unknown module types and unsupported values give a warning, not an
   error.  The parsed document lives as long as the program: module
   instances point into it.  */

#include "common/config.h"

#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "common/format.h"
#include "fastfetch.h"
#include "modules/modules.h"
#include "yyjson.h"

static yyjson_doc *doc;

static void
warn (const char *fmt, ...)
{
  char msg[512];
  va_list ap;

  /* One call: on VMS each stderr write is a record, and a message split
     over three calls ran into the next line on a terminal (D3).  */
  va_start (ap, fmt);
  vsnprintf (msg, sizeof msg, fmt, ap);
  va_end (ap);
  fprintf (stderr, "fastfetch: config: %s\n", msg);
}

const char *
ffJsonGetString (const void *obj, const char *key, const char *def)
{
  yyjson_val *v = obj ? yyjson_obj_get ((yyjson_val *) obj, key) : NULL;
  return yyjson_is_str (v) ? yyjson_get_str (v) : def;
}

bool
ffJsonGetBool (const void *obj, const char *key, bool def)
{
  yyjson_val *v = obj ? yyjson_obj_get ((yyjson_val *) obj, key) : NULL;
  return yyjson_is_bool (v) ? yyjson_get_bool (v) : def;
}

int64_t
ffJsonGetInt (const void *obj, const char *key, int64_t def)
{
  yyjson_val *v = obj ? yyjson_obj_get ((yyjson_val *) obj, key) : NULL;
  return yyjson_is_int (v) ? yyjson_get_sint (v) : def;
}

#ifdef __VMS
/* SYS$LOGIN's translation with subdirectories appended:
   "USER$ROOT:[username]" + ".^.config" -> "USER$ROOT:[username.^.config]".
   "SYS$LOGIN:[.x]" does not work: SYS$LOGIN is a plain directory, not a
   rooted logical, so RMS will not merge a relative directory onto it.  */
static void
loginSubdir (char *out, size_t size, const char *sub, const char *file)
{
  const char *login = getenv ("SYS$LOGIN");
  size_t n = login ? strlen (login) : 0;

  if (n < 3 || (login[n - 1] != ']' && login[n - 1] != '>'))
    {
      snprintf (out, size, "SYS$LOGIN:%s", file);   /* unusual: no subdirectory */
      return;
    }
  if (n >= 8 && strncmp (login + n - 8, "[000000]", 8) == 0)
    snprintf (out, size, "%.*s[%s]%s", (int) (n - 8), login, sub + 1, file);
  else
    snprintf (out, size, "%.*s%s]%s", (int) (n - 1), login, sub, file);
}
#endif

const char *
ffConfigDefaultPath (void)
{
#ifdef __VMS
  /* ~/.config/fastfetch/config.jsonc; "^." escapes the leading dot (ODS-5).  */
  static char path[512];
  if (!path[0])
    loginSubdir (path, sizeof path, ".^.config.fastfetch", "config.jsonc");
  return path;
#else
  static char path[1024];
  const char *xdg = getenv ("XDG_CONFIG_HOME");
  const char *home = getenv ("HOME");
  if (xdg && *xdg)
    snprintf (path, sizeof path, "%s/fastfetch/config.jsonc", xdg);
  else
    snprintf (path, sizeof path, "%s/.config/fastfetch/config.jsonc", home ? home : ".");
  return path;
#endif
}

static char *
readFile (const char *path, size_t *len)
{
  FILE *f = fopen (path, "r");
  char *buf = NULL;
  size_t cap = 0, n = 0, got;

  if (!f)
    return NULL;
  do
    {
      if (cap - n < 4096)
        {
          char *p = realloc (buf, cap + 16384);
          if (!p)
            {
              free (buf);
              fclose (f);
              return NULL;
            }
          buf = p;
          cap += 16384;
        }
      got = fread (buf + n, 1, cap - n - 1, f);
      n += got;
    }
  while (got > 0);
  fclose (f);
  buf[n] = 0;
  *len = n;
  return buf;
}

/* --- applying the document ------------------------------------------ */

static void
setColor (char *out, size_t size, yyjson_val *v, const char *what)
{
  if (!yyjson_is_str (v))
    return;
  if (!ffColorParse (yyjson_get_str (v), out, (uint32_t) size))
    warn ("unknown colour \"%s\" for %s", yyjson_get_str (v), what);
}

static uint32_t
getUint (yyjson_val *obj, const char *key, uint32_t def)
{
  yyjson_val *v = yyjson_obj_get (obj, key);
  return yyjson_is_int (v) && yyjson_get_sint (v) >= 0 ? (uint32_t) yyjson_get_sint (v) : def;
}

static void
applyLogo (yyjson_val *logo)
{
  FFOptions *o = &instance.options;
  yyjson_val *v;
  const char *type, *source;

  if (yyjson_is_str (logo))
    {
      o->logoName = yyjson_get_str (logo);
      return;
    }
  if (yyjson_is_null (logo))
    {
      o->logoName = "none";
      return;
    }
  if (!yyjson_is_obj (logo))
    return;
  type = ffJsonGetString (logo, "type", "auto");
  source = ffJsonGetString (logo, "source", NULL);
  o->logoType = type;
  if (strcmp (type, "none") == 0)
    o->logoName = "none";
  else if (strcmp (type, "file") == 0 || strcmp (type, "file-raw") == 0)
    {
      size_t len;
      char *text = source ? readFile (source, &len) : NULL;
      if (!text)
        warn ("cannot read logo file \"%s\"", source ? source : "");
      else
        {
          ffStrbufSetS (&instance.logoData, text);
          free (text);
        }
    }
  else if (strcmp (type, "data") == 0 || strcmp (type, "data-raw") == 0)
    ffStrbufSetS (&instance.logoData, source ? source : "");
  else if (strcmp (type, "auto") == 0 || strcmp (type, "builtin") == 0 || strcmp (type, "small") == 0)
    {
      if (source)
        o->logoName = source;
    }
  else
    warn ("logo type \"%s\" is not supported on OpenVMS; using the builtin logo", type);

  if ((v = yyjson_obj_get (logo, "color")) && yyjson_is_obj (v))
    {
      int i;
      for (i = 0; i < FF_LOGO_MAX_COLORS; i++)
        {
          char key[2] = { (char) ('1' + i), 0 };
          setColor (instance.logoColors[i], sizeof instance.logoColors[i], yyjson_obj_get (v, key),
                    "logo.color");
        }
    }
  if ((v = yyjson_obj_get (logo, "padding")) && yyjson_is_obj (v))
    {
      o->logoPaddingTop = getUint (v, "top", o->logoPaddingTop);
      o->logoPaddingLeft = getUint (v, "left", o->logoPaddingLeft);
      o->logoPaddingRight = getUint (v, "right", o->logoPaddingRight);
    }
}

static void
applyDisplay (yyjson_val *d)
{
  FFOptions *o = &instance.options;
  yyjson_val *v;

  if (!yyjson_is_obj (d))
    return;
  if ((v = yyjson_obj_get (d, "separator")) && yyjson_is_str (v))
    o->keySeparator = yyjson_get_str (v);
  if ((v = yyjson_obj_get (d, "showErrors")) && yyjson_is_bool (v))
    o->showErrors = yyjson_get_bool (v);
  if ((v = yyjson_obj_get (d, "pipe")) && yyjson_is_bool (v))
    o->pipe = yyjson_get_bool (v), o->pipeSet = true;
  if ((v = yyjson_obj_get (d, "color")))
    {
      if (yyjson_is_str (v))
        {
          /* A plain string colours both keys and title (fastfetch).  */
          setColor (instance.keyColor, sizeof instance.keyColor, v, "display.color");
          setColor (instance.titleColor, sizeof instance.titleColor, v, "display.color");
          instance.keyColorSet = instance.titleColorSet = true;
        }
      else if (yyjson_is_obj (v))
        {
          if (yyjson_obj_get (v, "keys"))
            {
              setColor (instance.keyColor, sizeof instance.keyColor, yyjson_obj_get (v, "keys"),
                        "display.color.keys");
              instance.keyColorSet = true;
            }
          if (yyjson_obj_get (v, "title"))
            {
              setColor (instance.titleColor, sizeof instance.titleColor, yyjson_obj_get (v, "title"),
                        "display.color.title");
              instance.titleColorSet = true;
            }
          setColor (instance.outputColor, sizeof instance.outputColor, yyjson_obj_get (v, "output"),
                    "display.color.output");
        }
    }
  if ((v = yyjson_obj_get (d, "key")) && yyjson_is_obj (v))
    o->keyWidth = getUint (v, "width", o->keyWidth);
  if ((v = yyjson_obj_get (d, "size")) && yyjson_is_obj (v))
    {
      const char *prefix = ffJsonGetString (v, "binaryPrefix", NULL);
      if (prefix && strcmp (prefix, "si") == 0)
        o->sizePrefix = FF_SIZE_SI;
      else if (prefix && strcmp (prefix, "jedec") == 0)
        o->sizePrefix = FF_SIZE_JEDEC;
      else if (prefix && strcmp (prefix, "iec") == 0)
        o->sizePrefix = FF_SIZE_IEC;
      else if (prefix)
        warn ("unknown display.size.binaryPrefix \"%s\"", prefix);
      o->sizeDigits = getUint (v, "ndigits", o->sizeDigits);
    }
  if ((v = yyjson_obj_get (d, "percent")) && yyjson_is_obj (v))
    {
      o->percentGreen = getUint (v, "green", o->percentGreen);
      o->percentYellow = getUint (v, "yellow", o->percentYellow);
    }
}

static void
addInstance (const FFModule *m, yyjson_val *obj)
{
  FFModuleInstance *inst = ffListAdd (&instance.modules);
  inst->module = m;
  inst->args.json = obj;
  if (obj)
    {
      inst->args.key = ffJsonGetString (obj, "key", NULL);
      inst->args.format = ffJsonGetString (obj, "format", NULL);
      setColor (inst->args.keyColor, sizeof inst->args.keyColor, yyjson_obj_get (obj, "keyColor"),
                "keyColor");
      setColor (inst->args.outputColor, sizeof inst->args.outputColor,
                yyjson_obj_get (obj, "outputColor"), "outputColor");
    }
}

static void
applyModules (yyjson_val *mods)
{
  size_t i, n;

  if (!yyjson_is_arr (mods))
    return;
  n = yyjson_arr_size (mods);
  for (i = 0; i < n; i++)
    {
      yyjson_val *e = yyjson_arr_get (mods, i);
      const char *type = yyjson_is_str (e) ? yyjson_get_str (e) : ffJsonGetString (e, "type", NULL);
      const FFModule *m = type ? ffModuleFind (type, (unsigned) strlen (type)) : NULL;
      if (!m)
        {
          warn ("unknown module \"%s\" (see --list-modules)", type ? type : "(no type)");
          continue;
        }
      addInstance (m, yyjson_is_obj (e) ? e : NULL);
    }
}

static bool
loadFile (const char *path, bool explicit_)
{
  yyjson_read_err err;
  yyjson_val *root;
  size_t len;
  char *text = readFile (path, &len);

  if (!text)
    {
      if (explicit_)
        fprintf (stderr, "fastfetch: cannot open config \"%s\": %s\n", path, strerror (errno));
      return false;
    }
  doc = yyjson_read_opts (text, len,
                          YYJSON_READ_ALLOW_COMMENTS | YYJSON_READ_ALLOW_TRAILING_COMMAS
                          | YYJSON_READ_ALLOW_INF_AND_NAN, NULL, &err);
  if (!doc)
    {
      size_t line = 1, i;
      for (i = 0; i < err.pos && i < len; i++)
        line += text[i] == '\n';
      fprintf (stderr, "fastfetch: %s: line %lu: %s\n", path, (unsigned long) line, err.msg);
      free (text);
      exit (EXIT_FAILURE);
    }
  free (text);
  root = yyjson_doc_get_root (doc);
  if (!yyjson_is_obj (root))
    {
      fprintf (stderr, "fastfetch: %s: the top level is not an object\n", path);
      exit (EXIT_FAILURE);
    }
  if (yyjson_obj_get (root, "logo"))
    applyLogo (yyjson_obj_get (root, "logo"));
  applyDisplay (yyjson_obj_get (root, "display"));
  applyModules (yyjson_obj_get (root, "modules"));
  return true;
}

bool
ffConfigLoad (void)
{
  const char *c = instance.options.config;
  const char *env;
  char path[512];

  if (c)
    {
      if (strcmp (c, "none") == 0 || strcmp (c, "NONE") == 0)
        return true;
      if (loadFile (c, false))
        return true;
      /* A preset name: FASTFETCH$PRESETS:<name>.jsonc (the kit defines it).  */
      if (!strpbrk (c, ":[]<>/"))
        {
#ifdef __VMS
          snprintf (path, sizeof path, "FASTFETCH$PRESETS:%s%s", c, strchr (c, '.') ? "" : ".jsonc");
#else
          snprintf (path, sizeof path, "presets/%s%s", c, strchr (c, '.') ? "" : ".jsonc");
#endif
          if (loadFile (path, false))
            return true;
        }
      return loadFile (c, true);
    }
  env = getenv ("FASTFETCH$CONFIG");
  if (env && *env && loadFile (env, false))
    return true;
  if (loadFile (ffConfigDefaultPath (), false))
    return true;
#ifdef __VMS
  loadFile ("SYS$LOGIN:FASTFETCH.JSONC", false);
#endif
  return true;
}

/* --- --gen-config ----------------------------------------------------- */

static void
makeDirs (const char *path)
{
#ifdef __VMS
  /* Only the default location needs directories created.  */
  if (strcmp (path, ffConfigDefaultPath ()) == 0)
    {
      char dir[512];
      loginSubdir (dir, sizeof dir, ".^.config", "");
      mkdir (dir, 0700);
      loginSubdir (dir, sizeof dir, ".^.config.fastfetch", "");
      mkdir (dir, 0700);
    }
#else
  char dir[1024];
  char *p;
  snprintf (dir, sizeof dir, "%s", path);
  for (p = dir + 1; *p; p++)
    if (*p == '/')
      {
        *p = 0;
        mkdir (dir, 0700);
        *p = '/';
      }
#endif
}

bool
ffConfigGenerate (const char *path, bool force)
{
  FILE *f;
  uint32_t i;

  if (!path || !*path)
    path = ffConfigDefaultPath ();
  if (!force && (f = fopen (path, "r")))
    {
      fclose (f);
      fprintf (stderr, "fastfetch: %s exists; use --gen-config-force to overwrite\n", path);
      return false;
    }
  makeDirs (path);
  f = fopen (path, "w");
  if (!f)
    {
      fprintf (stderr, "fastfetch: cannot create %s: %s\n", path, strerror (errno));
      return false;
    }
  fprintf (f, "// fastfetch configuration, written by vms-fastfetch %s\n", FF_PROGRAM_VERSION);
  fprintf (f, "// The format is fastfetch's: see its wiki and the json_schema below.\n");
  fprintf (f, "{\n");
  fprintf (f, "    \"$schema\": \"%s\",\n", FF_SCHEMA_URL);
  if (instance.logo)
    fprintf (f, "    \"logo\": {\n        \"type\": \"builtin\",\n        \"source\": \"%s\"\n    },\n",
             instance.logo->names[0]);
  else
    fprintf (f, "    \"logo\": {\n        \"type\": \"none\"\n    },\n");
  fputs ("    \"display\": {\n        \"separator\": \"", f);
  {
    const char *s;
    for (s = instance.options.keySeparator; *s; s++)
      {
        if (*s == '"' || *s == '\\')
          fputc ('\\', f);
        fputc (*s, f);
      }
  }
  fputs ("\"\n    },\n", f);
  fprintf (f, "    \"modules\": [\n");
  for (i = 0; i < instance.modules.length; i++)
    {
      const FFModuleInstance *m = ffListGet (&instance.modules, i);
      const char *n = m->module->name;
      fputs ("        \"", f);
      for (; *n; n++)
        fputc (tolower ((unsigned char) *n), f);
      fprintf (f, "\"%s\n", i + 1 < instance.modules.length ? "," : "");
    }
  fprintf (f, "    ]\n}\n");
  if (fclose (f) != 0)
    {
      fprintf (stderr, "fastfetch: error writing %s\n", path);
      return false;
    }
  printf ("fastfetch: wrote %s\n", path);
  return true;
}
