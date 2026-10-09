/* system.c - OS, Host, Kernel and Uptime modules.

   Argument lists follow fastfetch's, in order, so {N} formats carry over;
   values OpenVMS does not have are empty.  */

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "common/printing.h"
#include "detection/detection.h"
#include "fastfetch.h"
#include "modules/modules.h"
#include "vms/vmsapi.h"

#ifdef __VMS
#include <syidef.h>
#endif

void
ffPrintOS (const FFModuleArgs *m)
{
  const FFOSResult *os = ffDetectOS ();
  FFformatarg args[] = {
    ffArgS ("sysname", os->name.chars),
    ffArgS ("name", os->name.chars),
    ffArgS ("pretty-name", os->prettyName.chars),
    ffArgS ("id", os->id.chars),
    ffArgS ("id-like", ""),
    ffArgS ("variant", ""),
    ffArgS ("variant-id", ""),
    ffArgS ("version", os->version.chars),
    ffArgS ("version-id", os->version.chars),
    ffArgS ("codename", ""),
    ffArgS ("build-id", ""),
    ffArgS ("arch", os->arch.chars),
    ffArgS ("kernel-release", os->version.chars),
  };

  /* fastfetch's default: pretty name and architecture.  */
  ffOutput (m, "OS", "{3}{?12} {12}{?}", sizeof args / sizeof args[0], args);
}

void
ffPrintHost (const FFModuleArgs *m)
{
  FFHostResult host;
  const char *error;

  ffStrbufInit (&host.name);
  host.virtualMachine = false;
  error = ffDetectHost (&host);
  if (error)
    ffOutputError (m, "Host", error);
  else
    {
      FFformatarg args[] = {
        ffArgS ("family", ""),
        ffArgS ("name", host.name.chars),
        ffArgS ("version", ""),
        ffArgS ("sku", ""),
        ffArgS ("vendor", ""),
        ffArgS ("serial", ""),
        ffArgS ("uuid", ""),
      };
      ffOutput (m, "Host", "{2}", sizeof args / sizeof args[0], args);
    }
  ffStrbufDestroy (&host.name);
}

void
ffPrintKernel (const FFModuleArgs *m)
{
  FFKernelResult k;
  const char *error;
  uint64_t pagesize = 0;

  ffStrbufInit (&k.sysname);
  ffStrbufInit (&k.release);
  ffStrbufInit (&k.version);
  ffStrbufInit (&k.machine);
  error = ffDetectKernel (&k);
  if (error)
    ffOutputError (m, "Kernel", error);
  else
    {
#ifdef __VMS
      ffVmsSyiU64 (SYI$_PAGE_SIZE, &pagesize);
#endif
      {
        FFformatarg args[] = {
          ffArgS ("sysname", k.sysname.chars),
          ffArgS ("release", k.release.chars),
          ffArgS ("version", k.version.chars),
          ffArgS ("arch", k.machine.chars),
          ffArgU ("page-size", pagesize),
        };
        ffOutput (m, "Kernel", "{1} {2}", sizeof args / sizeof args[0], args);
      }
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
ffPrintUptime (const FFModuleArgs *m)
{
  FFUptimeResult up;
  const char *error = ffDetectUptime (&up);
  FFstrbuf formatted, boot;
  uint64_t s;
  time_t bootUnix;
  char buf[32];

  if (error)
    {
      ffOutputError (m, "Uptime", error);
      return;
    }
  /* fastfetch: "6 days, 6 hours, 12 mins"; "N secs" under a minute.  */
  s = up.uptimeSeconds;
  ffStrbufInit (&formatted);
  appendUnit (&formatted, s / 86400, "day");
  appendUnit (&formatted, s / 3600 % 24, "hour");
  appendUnit (&formatted, s / 60 % 60, "min");
  if (formatted.length == 0)
    appendUnit (&formatted, s, "sec");
  ffStrbufInit (&boot);
  bootUnix = time (NULL) - (time_t) s;
  strftime (buf, sizeof buf, "%Y-%m-%d %H:%M:%S", localtime (&bootUnix));
  ffStrbufSetS (&boot, buf);
  {
    FFformatarg args[] = {
      ffArgU ("days", s / 86400),
      ffArgU ("hours", s / 3600 % 24),
      ffArgU ("minutes", s / 60 % 60),
      ffArgU ("seconds", s % 60),
      ffArgU ("milliseconds", 0),
      ffArgS ("boot-time", boot.chars),
      ffArgU ("years", s / (365 * 86400)),
      ffArgU ("days-of-year", s / 86400 % 365),
      ffArgD ("years-fraction", (double) s / (365.25 * 86400)),
      ffArgS ("formatted", formatted.chars),
    };
    ffOutput (m, "Uptime", "{formatted}", sizeof args / sizeof args[0], args);
  }
  ffStrbufDestroy (&formatted);
  ffStrbufDestroy (&boot);
}
