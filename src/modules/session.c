/* session.c - Packages, Shell, Terminal, TerminalSize, LocalIp, Locale,
   Processes, Users, DateTime, Version and Cluster modules.

   Argument lists follow fastfetch's, in order (OpenVMS additions last);
   values OpenVMS does not have are empty.  */

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "common/printing.h"
#include "detection/detection.h"
#include "fastfetch.h"
#include "modules/modules.h"

void
ffPrintPackages (const FFModuleArgs *m)
{
  FFPackagesResult pkg = { 0 };
  const char *error = ffDetectPackages (&pkg);

  if (error)
    ffOutputError (m, "Packages", error);
  else
    {
      /* Only PCSI exists on OpenVMS: name-based {all}/{pcsi} carry over,
         upstream's numeric positions (one per Linux package manager) do not.  */
      FFformatarg args[] = {
        ffArgU ("all", pkg.pcsi),
        ffArgU ("pcsi", pkg.pcsi),
      };
      ffOutput (m, "Packages", "{2} (pcsi)", sizeof args / sizeof args[0], args);
    }
}

void
ffPrintShell (const FFModuleArgs *m)
{
  FFstrbuf shell;
  const char *error;

  ffStrbufInit (&shell);
  error = ffDetectShell (&shell);
  if (error)
    ffOutputError (m, "Shell", error);
  else
    {
      FFformatarg args[] = {
        ffArgS ("process-name", shell.chars),
        ffArgS ("exe", shell.chars),
        ffArgS ("exe-name", shell.chars),
        ffArgS ("version", ""),
        ffArgU ("pid", ffDetectPlatform ()->pid),
        ffArgS ("pretty-name", shell.chars),
        ffArgS ("exe-path", ""),
        ffArgS ("tty", ""),
      };
      ffOutput (m, "Shell", "{6}{?4} {4}{?}", sizeof args / sizeof args[0], args);
    }
  ffStrbufDestroy (&shell);
}

static void
terminalInit (FFTerminalResult *t)
{
  ffStrbufInit (&t->device);
  ffStrbufInit (&t->kind);
  ffStrbufInit (&t->type);
  ffStrbufInit (&t->remote);
  t->columns = t->rows = 0;
}

static void
terminalDestroy (FFTerminalResult *t)
{
  ffStrbufDestroy (&t->device);
  ffStrbufDestroy (&t->kind);
  ffStrbufDestroy (&t->type);
  ffStrbufDestroy (&t->remote);
}

void
ffPrintTerminal (const FFModuleArgs *m)
{
  FFTerminalResult t;
  const char *error;

  terminalInit (&t);
  error = ffDetectTerminal (&t);
  if (error)
    ffOutputError (m, "Terminal", error);
  else
    {
      /* pretty-name: "FTA1757: (pseudo-terminal) VT500 series",
         "TNA5: (telnet, 10.0.0.9)"  */
      FFstrbuf pretty;
      ffStrbufInit (&pretty);
      ffStrbufAppend (&pretty, &t.device);
      if (!ffStrbufIsEmpty (&t.kind) || !ffStrbufIsEmpty (&t.remote))
        {
          ffStrbufAppendS (&pretty, " (");
          ffStrbufAppend (&pretty, &t.kind);
          if (!ffStrbufIsEmpty (&t.kind) && !ffStrbufIsEmpty (&t.remote))
            ffStrbufAppendS (&pretty, ", ");
          ffStrbufAppend (&pretty, &t.remote);
          ffStrbufAppendC (&pretty, ')');
        }
      if (!ffStrbufIsEmpty (&t.type))
        {
          ffStrbufAppendC (&pretty, ' ');
          ffStrbufAppend (&pretty, &t.type);
        }
      {
        FFformatarg args[] = {
          ffArgS ("process-name", ""),
          ffArgS ("exe", ""),
          ffArgS ("exe-name", ""),
          ffArgU ("pid", 0),
          ffArgS ("pretty-name", pretty.chars),
          ffArgS ("version", ""),
          ffArgS ("exe-path", ""),
          ffArgS ("tty", t.device.chars),
          ffArgS ("kind", t.kind.chars),        /* OpenVMS additions */
          ffArgS ("type", t.type.chars),
          ffArgS ("remote", t.remote.chars),
        };
        ffOutput (m, "Terminal", "{5}", sizeof args / sizeof args[0], args);
      }
      ffStrbufDestroy (&pretty);
    }
  terminalDestroy (&t);
}

void
ffPrintTerminalSize (const FFModuleArgs *m)
{
  FFTerminalResult t;
  const char *error;

  terminalInit (&t);
  error = ffDetectTerminal (&t);
  if (error)
    ffOutputError (m, "Terminal Size", error);
  else
    {
      FFformatarg args[] = {
        ffArgU ("rows", t.rows),
        ffArgU ("columns", t.columns),
        ffArgU ("width", 0),
        ffArgU ("height", 0),
      };
      ffOutput (m, "Terminal Size", "{2} columns x {1} rows", sizeof args / sizeof args[0], args);
    }
  terminalDestroy (&t);
}

void
ffPrintLocalIp (const FFModuleArgs *m)
{
  FFlist ips;
  const char *error;
  uint32_t i;

  ffListInit (&ips, sizeof (FFLocalIpResult));
  error = ffDetectLocalIps (&ips);
  if (error)
    ffOutputError (m, "Local IP", error);
  for (i = 0; i < ips.length; i++)
    {
      FFLocalIpResult *ip = ffListGet (&ips, i);
      FFformatarg args[] = {
        ffArgS ("ipv4", ip->address.chars),
        ffArgS ("ipv6", ""),
        ffArgS ("mac", ""),
        ffArgS ("ifname", ip->interface.chars),
        ffArgB ("is-default-route", false),
        ffArgU ("mtu", 0),
        ffArgS ("speed", ""),
        ffArgS ("flags", ""),
      };
      /* fastfetch: "Local IP (eth0): 10.0.0.5/24"  */
      ffOutput (m, "Local IP ({ifname})", "{1}", sizeof args / sizeof args[0], args);
      ffStrbufDestroy (&ip->interface);
      ffStrbufDestroy (&ip->address);
    }
  ffListDestroy (&ips);
}

void
ffPrintLocale (const FFModuleArgs *m)
{
  FFstrbuf locale;

  ffStrbufInit (&locale);
  if (!ffDetectLocale (&locale))
    {
      FFformatarg args[] = { ffArgS ("result", locale.chars) };
      ffOutput (m, "Locale", "{1}", 1, args);
    }
  ffStrbufDestroy (&locale);
}

static void
processesDestroy (FFProcessesResult *p)
{
  uint32_t i;
  for (i = 0; i < p->users.length; i++)
    ffStrbufDestroy (&((FFUserResult *) ffListGet (&p->users, i))->name);
  ffListDestroy (&p->users);
}

void
ffPrintProcesses (const FFModuleArgs *m)
{
  FFProcessesResult p;
  const char *error;

  memset (&p, 0, sizeof p);
  ffListInit (&p.users, sizeof (FFUserResult));
  error = ffDetectProcesses (&p);
  if (error)
    ffOutputError (m, "Processes", error);
  else
    {
      FFformatarg args[] = {
        ffArgU ("result", p.processes),
        ffArgU ("threads", 0),
        ffArgU ("hidden", p.hidden),    /* OpenVMS addition: not visible without privilege */
      };
      ffOutput (m, "Processes", "{1}{?3} (+{3} not visible){?}", sizeof args / sizeof args[0], args);
    }
  processesDestroy (&p);
}

void
ffPrintUsers (const FFModuleArgs *m)
{
  FFProcessesResult p;
  const char *error;
  uint32_t i;

  memset (&p, 0, sizeof p);
  ffListInit (&p.users, sizeof (FFUserResult));
  error = ffDetectProcesses (&p);
  if (error)
    ffOutputError (m, "Users", error);
  else if (p.users.length == 0)
    ffOutputError (m, "Users", "No interactive sessions visible");
  else
    {
      /* One line: "IAIN (2), SYSTEM" (interactive sessions per user).  */
      FFstrbuf names;
      uint32_t sessions = 0;
      ffStrbufInit (&names);
      for (i = 0; i < p.users.length; i++)
        {
          FFUserResult *u = ffListGet (&p.users, i);
          if (i)
            ffStrbufAppendS (&names, ", ");
          ffStrbufAppend (&names, &u->name);
          if (u->sessions > 1)
            ffStrbufAppendF (&names, " (%u)", u->sessions);
          sessions += u->sessions;
        }
      {
        FFformatarg args[] = {
          ffArgS ("name", names.chars),
          ffArgS ("host-name", ""),
          ffArgS ("session-name", ""),
          ffArgS ("client-ip", ""),
          ffArgS ("login-time", ""),
          ffArgU ("sessions", sessions),        /* OpenVMS addition */
        };
        ffOutput (m, "Users", "{1}", sizeof args / sizeof args[0], args);
      }
      ffStrbufDestroy (&names);
    }
  processesDestroy (&p);
}

void
ffPrintDateTime (const FFModuleArgs *m)
{
  time_t now = time (NULL);
  struct tm tm = *localtime (&now);
  char b[24][32];
  /* Each field has its own buffer: initialiser order is unspecified.  */
#define TF(n, fmt) (strftime (b[n], sizeof b[n], fmt, &tm), b[n])

  FFformatarg args[] = {
    ffArgU ("year", (uint64_t) tm.tm_year + 1900),
    ffArgS ("year-short", TF (0, "%y")),
    ffArgU ("month", (uint64_t) tm.tm_mon + 1),
    ffArgS ("month-pretty", TF (1, "%m")),
    ffArgS ("month-name", TF (2, "%B")),
    ffArgS ("month-name-short", TF (3, "%b")),
    ffArgS ("week", TF (4, "%V")),
    ffArgS ("weekday", TF (5, "%A")),
    ffArgS ("weekday-short", TF (6, "%a")),
    ffArgU ("day-in-year", (uint64_t) tm.tm_yday + 1),
    ffArgU ("day-in-month", (uint64_t) tm.tm_mday),
    ffArgU ("day-in-week", (uint64_t) (tm.tm_wday ? tm.tm_wday : 7)),
    ffArgU ("hour", (uint64_t) tm.tm_hour),
    ffArgS ("hour-pretty", TF (7, "%H")),
    ffArgU ("hour-12", (uint64_t) (tm.tm_hour % 12 ? tm.tm_hour % 12 : 12)),
    ffArgS ("hour-12-pretty", TF (8, "%I")),
    ffArgU ("minute", (uint64_t) tm.tm_min),
    ffArgS ("minute-pretty", TF (9, "%M")),
    ffArgU ("second", (uint64_t) tm.tm_sec),
    ffArgS ("second-pretty", TF (10, "%S")),
    ffArgS ("offset-from-utc", TF (11, "%z")),
    ffArgS ("timezone-name", TF (12, "%Z")),
    ffArgS ("day-pretty", TF (13, "%d")),
    ffArgS ("am-pm", TF (14, "%p")),
  };
#undef TF
  /* fastfetch's default: "2026-10-09 10:11:12"  */
  ffOutput (m, "Date & Time", "{year}-{month-pretty}-{day-pretty} {hour-pretty}:{minute-pretty}:{second-pretty}",
            sizeof args / sizeof args[0], args);
}

void
ffPrintVersion (const FFModuleArgs *m)
{
  char compiler[32];
  FFformatarg args[10];

#ifdef __DECC_VER
  snprintf (compiler, sizeof compiler, "VSI C %d.%d", __DECC_VER / 10000000, __DECC_VER / 100000 % 100);
#else
  snprintf (compiler, sizeof compiler, "C");
#endif
  args[0] = ffArgS ("project-name", "vms-fastfetch");
  args[1] = ffArgS ("version", FF_PROGRAM_VERSION);
  args[2] = ffArgS ("version-tweak", "");
  args[3] = ffArgS ("build-type", "");
  args[4] = ffArgS ("sysname", ffDetectOS ()->name.chars);
  args[5] = ffArgS ("arch", ffDetectOS ()->arch.chars);
  args[6] = ffArgS ("cmake-built-type", "");
  args[7] = ffArgS ("compile-time", __DATE__ " " __TIME__);
  args[8] = ffArgS ("compiler", compiler);
  args[9] = ffArgS ("libc", "OpenVMS CRTL");
  ffOutput (m, "Version", "{1} {2} ({6})", 10, args);
}

void
ffPrintCluster (const FFModuleArgs *m)
{
  FFClusterResult c;
  const char *error;

  memset (&c, 0, sizeof c);
  error = ffDetectCluster (&c);
  if (error)
    ffOutputError (m, "Cluster", error);
  else
    {
      FFformatarg args[] = {
        ffArgB ("member", c.member),
        ffArgU ("nodes", c.nodes),
        ffArgU ("votes", c.votes),
        ffArgU ("quorum", c.quorum),
      };
      ffOutput (m, "Cluster", "{2} nodes, {3} votes (quorum {4})", sizeof args / sizeof args[0], args);
    }
}
