/* session.c - Packages, Shell, Terminal, TerminalSize, LocalIp, Locale,
   Processes, Users, DateTime, Version and Cluster modules.  */

#include <time.h>

#include "common/printing.h"
#include "detection/detection.h"
#include "fastfetch.h"
#include "modules/modules.h"

void
ffPrintPackages (void)
{
  FFPackagesResult pkg = { 0 };
  const char *error = ffDetectPackages (&pkg);

  if (error)
    ffPrintError ("Packages", error);
  else
    ffStrbufAppendF (ffPrintKey ("Packages"), "%u (pcsi)", pkg.pcsi);
}

void
ffPrintShell (void)
{
  FFstrbuf shell;
  const char *error;

  ffStrbufInit (&shell);
  error = ffDetectShell (&shell);
  if (error)
    ffPrintError ("Shell", error);
  else
    ffPrintKV ("Shell", shell.chars);
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
ffPrintTerminal (void)
{
  FFTerminalResult t;
  const char *error;

  terminalInit (&t);
  error = ffDetectTerminal (&t);
  if (error)
    ffPrintError ("Terminal", error);
  else
    {
      /* "FTA1757: (pseudo-terminal)", "TNA5: (telnet, 10.0.0.9) VT400 series" */
      FFstrbuf *line = ffPrintKey ("Terminal");
      ffStrbufAppend (line, &t.device);
      if (!ffStrbufIsEmpty (&t.kind) || !ffStrbufIsEmpty (&t.remote))
        {
          ffStrbufAppendS (line, " (");
          ffStrbufAppend (line, &t.kind);
          if (!ffStrbufIsEmpty (&t.kind) && !ffStrbufIsEmpty (&t.remote))
            ffStrbufAppendS (line, ", ");
          ffStrbufAppend (line, &t.remote);
          ffStrbufAppendC (line, ')');
        }
      if (!ffStrbufIsEmpty (&t.type))
        {
          ffStrbufAppendC (line, ' ');
          ffStrbufAppend (line, &t.type);
        }
    }
  terminalDestroy (&t);
}

void
ffPrintTerminalSize (void)
{
  FFTerminalResult t;
  const char *error;

  terminalInit (&t);
  error = ffDetectTerminal (&t);
  if (error)
    ffPrintError ("Terminal Size", error);
  else
    ffStrbufAppendF (ffPrintKey ("Terminal Size"), "%u columns x %u rows", t.columns, t.rows);
  terminalDestroy (&t);
}

void
ffPrintLocalIp (void)
{
  FFlist ips;
  const char *error;
  uint32_t i;

  ffListInit (&ips, sizeof (FFLocalIpResult));
  error = ffDetectLocalIps (&ips);
  if (error)
    ffPrintError ("Local IP", error);
  for (i = 0; i < ips.length; i++)
    {
      FFLocalIpResult *ip = ffListGet (&ips, i);
      FFstrbuf key;

      /* fastfetch: "Local IP (eth0): 10.0.0.5/24"  */
      ffStrbufInit (&key);
      ffStrbufAppendF (&key, "Local IP (%s)", ip->interface.chars);
      ffPrintKV (key.chars, ip->address.chars);
      ffStrbufDestroy (&key);
      ffStrbufDestroy (&ip->interface);
      ffStrbufDestroy (&ip->address);
    }
  ffListDestroy (&ips);
}

void
ffPrintLocale (void)
{
  FFstrbuf locale;

  ffStrbufInit (&locale);
  if (!ffDetectLocale (&locale))
    ffPrintKV ("Locale", locale.chars);
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
ffPrintProcesses (void)
{
  FFProcessesResult p = { 0 };
  const char *error;

  ffListInit (&p.users, sizeof (FFUserResult));
  error = ffDetectProcesses (&p);
  if (error)
    ffPrintError ("Processes", error);
  else
    {
      FFstrbuf *line = ffPrintKey ("Processes");
      ffStrbufAppendF (line, "%u", p.processes);
      if (p.hidden)
        ffStrbufAppendF (line, " (+%u not visible)", p.hidden);
    }
  processesDestroy (&p);
}

void
ffPrintUsers (void)
{
  FFProcessesResult p = { 0 };
  const char *error;
  uint32_t i;

  ffListInit (&p.users, sizeof (FFUserResult));
  error = ffDetectProcesses (&p);
  if (error)
    ffPrintError ("Users", error);
  else if (p.users.length == 0)
    ffPrintError ("Users", "No interactive sessions visible");
  else
    {
      /* "IAIN (2), SYSTEM": interactive sessions per user.  */
      FFstrbuf *line = ffPrintKey ("Users");
      for (i = 0; i < p.users.length; i++)
        {
          FFUserResult *u = ffListGet (&p.users, i);
          if (i)
            ffStrbufAppendS (line, ", ");
          ffStrbufAppend (line, &u->name);
          if (u->sessions > 1)
            ffStrbufAppendF (line, " (%u)", u->sessions);
        }
    }
  processesDestroy (&p);
}

void
ffPrintDateTime (void)
{
  char buf[64];
  time_t now = time (NULL);

  /* fastfetch's default: "2026-10-08 23:30:00"  */
  strftime (buf, sizeof buf, "%Y-%m-%d %H:%M:%S", localtime (&now));
  ffPrintKV ("Date & Time", buf);
}

void
ffPrintVersion (void)
{
  ffStrbufAppendF (ffPrintKey ("Version"), "vms-fastfetch %s (%s)", FF_PROGRAM_VERSION,
                   ffDetectOS ()->arch.chars);
}

void
ffPrintCluster (void)
{
  FFClusterResult c = { 0 };
  const char *error = ffDetectCluster (&c);

  if (error)
    ffPrintError ("Cluster", error);
  else
    ffStrbufAppendF (ffPrintKey ("Cluster"), "%u node%s, %u votes (quorum %u)",
                     c.nodes, c.nodes == 1 ? "" : "s", c.votes, c.quorum);
}
