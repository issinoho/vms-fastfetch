/* system.c - OS, Host, Kernel and Uptime modules.  */

#include "common/printing.h"
#include "detection/detection.h"
#include "fastfetch.h"
#include "modules/modules.h"

void
ffPrintOS (void)
{
  const FFOSResult *os = ffDetectOS ();
  FFstrbuf *line = ffPrintKey ("OS");

  /* fastfetch's default OS format: "{pretty-name} {arch}".  */
  ffStrbufAppend (line, &os->prettyName);
  if (!ffStrbufIsEmpty (&os->arch))
    {
      ffStrbufAppendC (line, ' ');
      ffStrbufAppend (line, &os->arch);
    }
}

void
ffPrintHost (void)
{
  FFHostResult host;
  const char *error;

  ffStrbufInit (&host.name);
  host.virtualMachine = false;
  error = ffDetectHost (&host);
  if (error)
    ffPrintError ("Host", error);
  else
    ffPrintKV ("Host", host.name.chars);
  ffStrbufDestroy (&host.name);
}

void
ffPrintKernel (void)
{
  FFKernelResult k;
  const char *error;

  ffStrbufInit (&k.sysname);
  ffStrbufInit (&k.release);
  ffStrbufInit (&k.version);
  ffStrbufInit (&k.machine);
  error = ffDetectKernel (&k);
  if (error)
    ffPrintError ("Kernel", error);
  else
    {
      /* fastfetch's default Kernel format: "{sysname} {release}".  */
      FFstrbuf *line = ffPrintKey ("Kernel");
      ffStrbufAppend (line, &k.sysname);
      ffStrbufAppendC (line, ' ');
      ffStrbufAppend (line, &k.release);
    }
  ffStrbufDestroy (&k.sysname);
  ffStrbufDestroy (&k.release);
  ffStrbufDestroy (&k.version);
  ffStrbufDestroy (&k.machine);
}

static void
appendUnit (FFstrbuf *sb, uint64_t n, const char *unit)
{
  if (n == 0)
    return;
  if (sb->length)
    ffStrbufAppendS (sb, ", ");
  ffStrbufAppendF (sb, "%lu %s%s", (unsigned long) n, unit, n == 1 ? "" : "s");
}

void
ffPrintUptime (void)
{
  FFUptimeResult up;
  const char *error = ffDetectUptime (&up);
  FFstrbuf value;
  uint64_t s;

  if (error)
    {
      ffPrintError ("Uptime", error);
      return;
    }
  /* fastfetch: "6 days, 6 hours, 12 mins"; "N secs" under a minute.  */
  s = up.uptimeSeconds;
  ffStrbufInit (&value);
  appendUnit (&value, s / 86400, "day");
  appendUnit (&value, s / 3600 % 24, "hour");
  appendUnit (&value, s / 60 % 60, "min");
  if (value.length == 0)
    appendUnit (&value, s, "sec");
  ffPrintKV ("Uptime", value.chars);
  ffStrbufDestroy (&value);
}

void
ffPrintColors (void)
{
  FFstrbuf *line;
  int i;

  /* Two rows of blocks, normal (40-47) and bright (100-107) backgrounds.  */
  if (instance.options.pipe)
    return;
  line = ffPrintNewLine ();
  for (i = 0; i < 8; i++)
    ffStrbufAppendF (line, "\033[%dm   ", 40 + i);
  ffStrbufAppendS (line, "\033[0m");
  line = ffPrintNewLine ();
  for (i = 0; i < 8; i++)
    ffStrbufAppendF (line, "\033[%dm   ", 100 + i);
  ffStrbufAppendS (line, "\033[0m");
}
