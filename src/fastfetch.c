/* fastfetch.c - entry point: configuration, options, module loop, output.

   Order, as fastfetch: the config file (-c, or the search in config.h) is
   loaded first, then the command line overrides it.  Option names follow
   fastfetch and are matched case-insensitively: DCL upper-cases unquoted
   arguments (and the CRTL then lower-cases them); no fastfetch option
   differs only by case.  Quote values whose case matters (formats).  */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common/config.h"
#include "common/format.h"
#include "common/printing.h"
#include "detection/detection.h"
#include "fastfetch.h"
#include "modules/modules.h"
#include "vms/vmsapi.h"
#include "yyjson.h"

#ifdef __VMS
#include <dcdef.h>
#include <dvidef.h>
#endif

FFInstance instance;

/* --<module>-key/-format/-key-color/-output-color, by module index.  */
static FFModuleArgs *cliArgs;

static int
ieq (const char *a, const char *b)
{
  for (; *a && *b; a++, b++)
    if (tolower ((unsigned char) *a) != tolower ((unsigned char) *b))
      return 0;
  return *a == *b;
}

/* D4: colour when SYS$OUTPUT is a terminal device.  */
static bool
outputIsTerminal (void)
{
#ifdef __VMS
  uint64_t cls;
  if (FF_VMS_OK (ffVmsDviU64 ("SYS$OUTPUT", DVI$_DEVCLASS, &cls)))
    return cls == DC$_TERM;
#endif
  return isatty (1) != 0;
}

static void
printHelp (void)
{
  printf ("Usage: fastfetch <?options>\n"
          "\n"
          "A fastfetch-compatible system information tool for OpenVMS.\n"
          "Options are case-insensitive; quote values whose case matters (formats).\n"
          "\n"
          "  -h, --help                  show this help\n"
          "  -v, --version               show the version\n"
          "  -c, --config <file|preset|none>\n"
          "                              config to load (default: see below)\n"
          "      --gen-config [file]     write a config for the current modules\n"
          "      --gen-config-force [file]  the same, overwriting\n"
          "  -s, --structure <A:B:C>     modules to print, in order\n"
          "  -l, --logo <name|none>      logo to show (see --list-logos)\n"
          "      --logo-type <auto|builtin|small|none>\n"
          "      --logo-color-[1-9] <color>\n"
          "      --logo-padding-left/-right/-top <n>\n"
          "  -j, --json, --format json   print the results as JSON\n"
          "      --pipe [true|false]     no colours (default: on unless SYS$OUTPUT is a terminal)\n"
          "      --show-errors [bool]    print modules that failed or are unsupported\n"
          "      --title-fqdn [bool]     show the TCP/IP host.domain in the title\n"
          "      --separator <str>       text between key and value (default \": \")\n"
          "      --key-width <n>         align values at column n\n"
          "      --color <color>         colour of keys and title (e.g. blue, bold_red, 38;5;208)\n"
          "      --color-keys/--color-title/--color-output <color>\n"
          "      --size-binary-prefix <iec|si|jedec>, --size-ndigits <n>\n"
          "      --command-text <dcl>    the DCL command the Command module runs\n"
          "      --<module>-key <str>, --<module>-format <str>,\n"
          "      --<module>-key-color <color>, --<module>-output-color <color>\n"
          "      --list-modules          list modules\n"
          "      --list-logos            list builtin logos\n"
          "\n"
          "Config search: FASTFETCH$CONFIG, %s, SYS$LOGIN:FASTFETCH.JSONC\n"
          "Default structure: %s\n", ffConfigDefaultPath (), FF_DEFAULT_STRUCTURE);
}

static void
listModules (void)
{
  const FFModule *m;
  int n = 0;

  for (m = ffModules; m->name; m++)
    printf ("%2d) %-14s %s%s\n", ++n, m->name,
            m->description ? m->description : "",
            m->print ? "" : (m->description ? " (not yet)" : "(not supported on OpenVMS)"));
}

static void
listLogos (void)
{
  uint32_t i;
  int n;

  for (i = 0; i < ffLogoBuiltinCount; i++)
    {
      FFstrbuf line;
      ffStrbufInit (&line);
      for (n = 0; n < FF_LOGO_MAX_NAMES && ffLogoBuiltins[i].names[n]; n++)
        ffStrbufAppendF (&line, "%s\"%s\"", n ? " " : "", ffLogoBuiltins[i].names[n]);
      printf ("%s\n", line.chars);
      ffStrbufDestroy (&line);
    }
}

/* Optional boolean after a flag: "--pipe", "--pipe true", "--pipe false".  */
static bool
boolArg (int argc, char **argv, int *i)
{
  if (*i + 1 < argc)
    {
      const char *v = argv[*i + 1];
      if (ieq (v, "true") || ieq (v, "yes") || ieq (v, "on") || strcmp (v, "1") == 0)
        return ++*i, true;
      if (ieq (v, "false") || ieq (v, "no") || ieq (v, "off") || strcmp (v, "0") == 0)
        return ++*i, false;
    }
  return true;
}

static const char *
valueArg (int argc, char **argv, int *i)
{
  if (*i + 1 >= argc)
    {
      fprintf (stderr, "fastfetch: %s needs a value\n", argv[*i]);
      exit (EXIT_FAILURE);
    }
  return argv[++*i];
}

/* An optional value: the next argument unless it is another option.  */
static const char *
optValueArg (int argc, char **argv, int *i)
{
  if (*i + 1 < argc && argv[*i + 1][0] != '-')
    return argv[++*i];
  return "";
}

static uint32_t
numArg (int argc, char **argv, int *i)
{
  const char *v = valueArg (argc, argv, i);
  char *end;
  unsigned long n = strtoul (v, &end, 10);
  if (*end || n > 1000)
    {
      fprintf (stderr, "fastfetch: %s: bad number \"%s\"\n", argv[*i - 1], v);
      exit (EXIT_FAILURE);
    }
  return (uint32_t) n;
}

static void
colorArg (int argc, char **argv, int *i, char *out, size_t size)
{
  const char *v = valueArg (argc, argv, i);
  if (!ffColorParse (v, out, (uint32_t) size))
    {
      fprintf (stderr, "fastfetch: %s: unknown colour \"%s\"\n", argv[*i - 1], v);
      exit (EXIT_FAILURE);
    }
}

/* --<module>-key, --<module>-format, --<module>-key-color, --<module>-output-color  */
static bool
moduleArg (int argc, char **argv, int *i)
{
  static const char *suffixes[] = { "-key-color", "-output-color", "-key", "-format", NULL };
  const char *a = argv[*i];
  size_t len = strlen (a);
  int s;

  if (strncmp (a, "--", 2) != 0)
    return false;
  for (s = 0; suffixes[s]; s++)
    {
      size_t sl = strlen (suffixes[s]);
      const FFModule *m;
      FFModuleArgs *ma;
      if (len <= sl + 2 || !ieq (a + len - sl, suffixes[s]))
        continue;
      m = ffModuleFind (a + 2, (unsigned) (len - sl - 2));
      if (!m)
        continue;
      ma = &cliArgs[m - ffModules];
      if (s == 0)
        colorArg (argc, argv, i, ma->keyColor, sizeof ma->keyColor);
      else if (s == 1)
        colorArg (argc, argv, i, ma->outputColor, sizeof ma->outputColor);
      else if (s == 2)
        ma->key = valueArg (argc, argv, i);
      else
        ma->format = valueArg (argc, argv, i);
      return true;
    }
  return false;
}

static void
parseArgs (int argc, char **argv)
{
  FFOptions *o = &instance.options;
  int i;

  for (i = 1; i < argc; i++)
    {
      const char *a = argv[i];
      if (ieq (a, "-h") || ieq (a, "--help"))
        printHelp (), exit (EXIT_SUCCESS);
      else if (ieq (a, "-v") || ieq (a, "--version"))
        {
          printf ("vms-fastfetch %s (fastfetch %s compatible, %s)\n", FF_PROGRAM_VERSION,
                  FF_UPSTREAM_VERSION, ffDetectOS ()->arch.chars);
          exit (EXIT_SUCCESS);
        }
      else if (ieq (a, "--list-modules"))
        listModules (), exit (EXIT_SUCCESS);
      else if (ieq (a, "--list-logos"))
        listLogos (), exit (EXIT_SUCCESS);
      else if (ieq (a, "-c") || ieq (a, "--config") || ieq (a, "--load-config"))
        valueArg (argc, argv, &i);      /* loaded before this pass */
      else if (ieq (a, "--gen-config"))
        o->genConfig = optValueArg (argc, argv, &i);
      else if (ieq (a, "--gen-config-force"))
        o->genConfig = optValueArg (argc, argv, &i), o->genConfigForce = true;
      else if (ieq (a, "-s") || ieq (a, "--structure"))
        o->structure = valueArg (argc, argv, &i);
      else if (ieq (a, "-l") || ieq (a, "--logo"))
        o->logoName = valueArg (argc, argv, &i);
      else if (ieq (a, "--logo-type"))
        {
          o->logoType = valueArg (argc, argv, &i);
          if (ieq (o->logoType, "none"))
            o->logoName = "none";
        }
      else if (strncmp (a, "--logo-color-", 13) == 0 && a[13] >= '1' && a[13] <= '9' && !a[14])
        colorArg (argc, argv, &i, instance.logoColors[a[13] - '1'], sizeof instance.logoColors[0]);
      else if (ieq (a, "-j") || ieq (a, "--json"))
        o->json = true;
      else if (ieq (a, "--format"))
        {
          const char *v = valueArg (argc, argv, &i);
          if (ieq (v, "json"))
            o->json = true;
          else if (!ieq (v, "default"))
            {
              fprintf (stderr, "fastfetch: --format: \"%s\" is not supported (default, json)\n", v);
              exit (EXIT_FAILURE);
            }
        }
      else if (ieq (a, "--pipe"))
        o->pipe = boolArg (argc, argv, &i), o->pipeSet = true;
      else if (ieq (a, "--show-errors"))
        o->showErrors = boolArg (argc, argv, &i);
      else if (ieq (a, "--title-fqdn"))
        o->titleFqdn = boolArg (argc, argv, &i);
      else if (ieq (a, "--separator"))
        o->keySeparator = valueArg (argc, argv, &i);
      else if (ieq (a, "--key-width"))
        o->keyWidth = numArg (argc, argv, &i);
      else if (ieq (a, "--logo-padding-left"))
        o->logoPaddingLeft = numArg (argc, argv, &i);
      else if (ieq (a, "--logo-padding-right"))
        o->logoPaddingRight = numArg (argc, argv, &i);
      else if (ieq (a, "--logo-padding-top"))
        o->logoPaddingTop = numArg (argc, argv, &i);
      else if (ieq (a, "--color"))
        {
          colorArg (argc, argv, &i, instance.keyColor, sizeof instance.keyColor);
          strcpy (instance.titleColor, instance.keyColor);
          instance.keyColorSet = instance.titleColorSet = true;
        }
      else if (ieq (a, "--color-keys"))
        colorArg (argc, argv, &i, instance.keyColor, sizeof instance.keyColor), instance.keyColorSet = true;
      else if (ieq (a, "--color-title"))
        colorArg (argc, argv, &i, instance.titleColor, sizeof instance.titleColor), instance.titleColorSet = true;
      else if (ieq (a, "--color-output"))
        colorArg (argc, argv, &i, instance.outputColor, sizeof instance.outputColor);
      else if (ieq (a, "--size-binary-prefix"))
        {
          const char *v = valueArg (argc, argv, &i);
          o->sizePrefix = ieq (v, "si") ? FF_SIZE_SI : ieq (v, "jedec") ? FF_SIZE_JEDEC : FF_SIZE_IEC;
        }
      else if (ieq (a, "--size-ndigits"))
        o->sizeDigits = numArg (argc, argv, &i);
      else if (ieq (a, "--command-text"))
        o->commandText = valueArg (argc, argv, &i);
      else if (!moduleArg (argc, argv, &i))
        {
          fprintf (stderr, "fastfetch: unknown option \"%s\" (see --help)\n", a);
          exit (EXIT_FAILURE);
        }
    }
}

/* -s replaces the config's module list.  */
static void
buildFromStructure (const char *structure)
{
  const char *p = structure;

  instance.modules.length = 0;
  while (*p)
    {
      const char *end = strchr (p, ':');
      unsigned n = end ? (unsigned) (end - p) : (unsigned) strlen (p);
      const FFModule *m = ffModuleFind (p, n);

      if (n && !m)
        fprintf (stderr, "fastfetch: unknown module \"%.*s\" (see --list-modules)\n", (int) n, p);
      else if (n)
        {
          FFModuleInstance *inst = ffListAdd (&instance.modules);
          inst->module = m;
        }
      p += n;
      if (*p == ':')
        p++;
    }
}

/* --<module>-... options override the config's settings for that module.  */
static void
applyModuleOverrides (void)
{
  uint32_t i;

  for (i = 0; i < instance.modules.length; i++)
    {
      FFModuleInstance *inst = ffListGet (&instance.modules, i);
      const FFModuleArgs *c = &cliArgs[inst->module - ffModules];
      if (c->key)
        inst->args.key = c->key;
      if (c->format)
        inst->args.format = c->format;
      if (c->keyColor[0])
        strcpy (inst->args.keyColor, c->keyColor);
      if (c->outputColor[0])
        strcpy (inst->args.outputColor, c->outputColor);
    }
}

static void
chooseLogo (void)
{
  FFOptions *o = &instance.options;
  const FFlogo *base = NULL;
  const char *name;
  int i;

  instance.logo = NULL;
  if (o->json || (o->logoName && ieq (o->logoName, "none")))
    return;
  name = o->logoName ? o->logoName : ffDetectOS ()->id.chars;
  if (o->logoType && ieq (o->logoType, "small"))
    {
      char small[64];
      snprintf (small, sizeof small, "%s_small", name);
      base = ffLogoFind (small);
    }
  if (!base)
    base = ffLogoFind (name);
  if (!base)
    {
      fprintf (stderr, "fastfetch: unknown logo \"%s\" (see --list-logos)\n", name);
      base = ffLogoFind ("openvms");
    }
  /* The logo in use: a copy with colour overrides, or text from a file/data.  */
  instance.logoStore = *base;
  if (!ffStrbufIsEmpty (&instance.logoData))
    instance.logoStore.data = instance.logoData.chars;
  for (i = 0; i < FF_LOGO_MAX_COLORS; i++)
    if (instance.logoColors[i][0])
      instance.logoStore.colors[i] = instance.logoColors[i];
  instance.logo = &instance.logoStore;
}

static void
runModules (void)
{
  uint32_t i;

  for (i = 0; i < instance.modules.length; i++)
    {
      FFModuleInstance *inst = ffListGet (&instance.modules, i);
      instance.currentModule = inst->module->name;
      instance.jsonEntry = NULL;
      if (inst->module->print)
        inst->module->print (&inst->args);
      else
        ffOutputError (&inst->args, inst->module->name,
                       inst->module->description ? "Not implemented yet" : "Not supported on this platform");
    }
}

int
main (int argc, char **argv)
{
  FFOptions *o = &instance.options;
  const char *noColor;
  int i;

  o->keySeparator = ": ";
  o->logoPaddingRight = 4;
  o->sizePrefix = FF_SIZE_IEC;
  o->sizeDigits = 2;
  o->percentGreen = 50;
  o->percentYellow = 80;
  ffListInit (&instance.lines, sizeof (FFstrbuf));
  ffListInit (&instance.modules, sizeof (FFModuleInstance));
  ffStrbufInit (&instance.logoData);
  cliArgs = calloc (ffModuleCount, sizeof *cliArgs);
  if (!cliArgs)
    return EXIT_FAILURE;

  /* Pass 1: the config file; pass 2: the rest of the command line.  */
  for (i = 1; i + 1 < argc; i++)
    if (ieq (argv[i], "-c") || ieq (argv[i], "--config") || ieq (argv[i], "--load-config"))
      o->config = argv[i + 1];
  if (!ffConfigLoad ())
    return EXIT_FAILURE;
  parseArgs (argc, argv);

  /* -s wins; else the config's "modules"; else the default.  */
  if (o->structure || instance.modules.length == 0)
    buildFromStructure (o->structure ? o->structure : FF_DEFAULT_STRUCTURE);
  applyModuleOverrides ();

  noColor = getenv ("NO_COLOR");
  if (!o->pipeSet)
    o->pipe = !outputIsTerminal () || (noColor && *noColor);
  if (o->json)
    o->pipe = true;

  chooseLogo ();
  if (!instance.keyColorSet)
    snprintf (instance.keyColor, sizeof instance.keyColor, "1;%s",
              instance.logo ? instance.logo->colorKeys : "34");
  if (!instance.titleColorSet)
    snprintf (instance.titleColor, sizeof instance.titleColor, "%s",
              instance.logo ? instance.logo->colorTitle : "34");

  if (o->genConfig)
    return ffConfigGenerate (o->genConfig, o->genConfigForce) ? EXIT_SUCCESS : EXIT_FAILURE;

  if (o->json)
    {
      yyjson_mut_doc *doc = yyjson_mut_doc_new (NULL);
      instance.jsonDoc = doc;
      instance.jsonArray = yyjson_mut_arr (doc);
      yyjson_mut_doc_set_root (doc, instance.jsonArray);
    }
  runModules ();
  ffPrintFinish ();
  return EXIT_SUCCESS;
}
