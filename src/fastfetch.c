/* fastfetch.c - entry point: options, module loop, output.

   Option names follow fastfetch.  They are matched case-insensitively
   because DCL upper-cases unquoted arguments (fastfetch -s os:uptime
   arrives as "-S" "OS:UPTIME"); no fastfetch option differs only by case.  */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common/printing.h"
#include "detection/detection.h"
#include "fastfetch.h"
#include "modules/modules.h"
#include "vms/vmsapi.h"

#ifdef __VMS
#include <dcdef.h>
#include <dvidef.h>
#endif

FFInstance instance;

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
  puts ("Usage: fastfetch <?options>\n"
        "\n"
        "A fastfetch-compatible system information tool for OpenVMS.\n"
        "Options are case-insensitive; quote values whose case matters.\n"
        "\n"
        "  -h, --help                  show this help\n"
        "  -v, --version               show the version\n"
        "  -s, --structure <A:B:C>     modules to print, in order\n"
        "  -l, --logo <name|none>      logo to show (see --list-logos)\n"
        "      --pipe [true|false]     no colours (default: on unless SYS$OUTPUT is a terminal)\n"
        "      --show-errors [bool]    print modules that failed or are unsupported\n"
        "      --title-fqdn [bool]     show the TCP/IP host.domain in the title\n"
        "      --separator <str>       text between key and value (default \": \")\n"
        "      --key-width <n>         align values at column n\n"
        "      --logo-padding-left/-right/-top <n>\n"
        "      --list-modules          list modules\n"
        "      --list-logos            list builtin logos\n"
        "\n"
        "Default structure: " FF_DEFAULT_STRUCTURE);
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
      else if (ieq (a, "-s") || ieq (a, "--structure"))
        o->structure = valueArg (argc, argv, &i);
      else if (ieq (a, "-l") || ieq (a, "--logo"))
        o->logoName = valueArg (argc, argv, &i);
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
      else
        {
          fprintf (stderr, "fastfetch: unknown option \"%s\" (see --help)\n", a);
          exit (EXIT_FAILURE);
        }
    }
}

static void
runStructure (const char *structure)
{
  const char *p = structure;

  while (*p)
    {
      const char *end = strchr (p, ':');
      unsigned n = end ? (unsigned) (end - p) : (unsigned) strlen (p);
      const FFModule *m = ffModuleFind (p, n);

      if (n == 0)
        ;
      else if (!m)
        fprintf (stderr, "fastfetch: unknown module \"%.*s\" (see --list-modules)\n", (int) n, p);
      else if (m->print)
        m->print ();
      else
        ffPrintError (m->name, "Not supported on this platform");
      p += n;
      if (*p == ':')
        p++;
    }
}

int
main (int argc, char **argv)
{
  FFOptions *o = &instance.options;
  const char *noColor;

  o->structure = FF_DEFAULT_STRUCTURE;
  o->keySeparator = ": ";
  o->logoPaddingRight = 4;
  ffListInit (&instance.lines, sizeof (FFstrbuf));

  parseArgs (argc, argv);

  noColor = getenv ("NO_COLOR");
  if (!o->pipeSet)
    o->pipe = !outputIsTerminal () || (noColor && *noColor);

  if (o->logoName && ieq (o->logoName, "none"))
    instance.logo = NULL;
  else
    {
      const char *name = o->logoName ? o->logoName : ffDetectOS ()->id.chars;
      instance.logo = ffLogoFind (name);
      if (!instance.logo)
        {
          fprintf (stderr, "fastfetch: unknown logo \"%s\" (see --list-logos)\n", name);
          instance.logo = ffLogoFind ("openvms");
        }
    }
  snprintf (instance.keyColor, sizeof instance.keyColor, "1;%s",
            instance.logo ? instance.logo->colorKeys : "34");
  snprintf (instance.titleColor, sizeof instance.titleColor, "%s",
            instance.logo ? instance.logo->colorTitle : "34");

  runStructure (o->structure);
  ffPrintFinish ();
  return EXIT_SUCCESS;
}
