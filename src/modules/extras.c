/* extras.c - CPUUsage, Loadavg ("Load"), DNS, Command and Queues.

   Argument lists follow fastfetch's where it has the module (OpenVMS
   additions last); Queues is OpenVMS only.  */

#include <string.h>

#include "common/config.h"
#include "common/printing.h"
#include "detection/detection.h"
#include "fastfetch.h"
#include "modules/modules.h"
#include "vms/vmsapi.h"

void
ffPrintCPUUsage (const FFModuleArgs *m)
{
  double pct = 0;
  int64_t wait = ffJsonGetInt (m->json, "waitTime", 200);
  const char *error = ffDetectCPUUsage (&pct, wait > 0 && wait < 10000 ? (uint32_t) wait : 200);

  if (error)
    ffOutputError (m, "CPU Usage", error);
  else
    {
      /* $GETRMI gives all CPUs together: avg = max = min.  */
      FFformatarg args[] = {
        ffArgPercent ("avg", pct),
        ffArgPercent ("max", pct),
        ffArgU ("max-index", 0),
        ffArgPercent ("min", pct),
        ffArgU ("min-index", 0),
        ffArgS ("avg-bar", ""),
        ffArgS ("max-bar", ""),
        ffArgS ("min-bar", ""),
      };
      ffOutput (m, "CPU Usage", "{1}", sizeof args / sizeof args[0], args);
    }
}

void
ffPrintLoadavg (const FFModuleArgs *m)
{
  FFLoadResult load;
  const char *error;

  memset (&load, 0, sizeof load);
  error = ffDetectLoad (&load);
  if (error)
    ffOutputError (m, "Load", error);
  else
    {
      /* D15: OpenVMS keeps no load averages; these are instantaneous
         counts of runnable processes, so loadavg2/3 stay empty.  */
      uint32_t runnable = load.current + load.computable + load.computableOut;
      FFformatarg args[] = {
        ffArgU ("loadavg1", runnable),
        ffArgS ("loadavg2", ""),
        ffArgS ("loadavg3", ""),
        ffArgU ("runnable", runnable),                  /* OpenVMS additions */
        ffArgU ("current", load.current),
        ffArgU ("computable", load.computable),
        ffArgU ("computable-outswapped", load.computableOut),
      };
      ffOutput (m, "Load", "{runnable} runnable now ({current} running, {computable} waiting)",
                sizeof args / sizeof args[0], args);
    }
}

void
ffPrintDNS (const FFModuleArgs *m)
{
  FFstrbuf servers, domain;
  const char *error;

  ffStrbufInit (&servers);
  ffStrbufInit (&domain);
  error = ffDetectDNS (&servers, &domain);
  if (error)
    ffOutputError (m, "DNS", error);
  else
    {
      FFformatarg args[] = {
        ffArgS ("result", servers.chars),
        ffArgS ("domain", domain.chars),                /* OpenVMS addition */
      };
      ffOutput (m, "DNS", "{1}", sizeof args / sizeof args[0], args);
    }
  ffStrbufDestroy (&servers);
  ffStrbufDestroy (&domain);
}

void
ffPrintCommand (const FFModuleArgs *m)
{
  /* fastfetch's "text"; the "shell" option does not apply: it is DCL.  */
  const char *text = ffJsonGetString (m->json, "text", instance.options.commandText);
  FFstrbuf out;
  uint32_t completion = 0;
  const char *error;

  if (!text || !*text)
    {
      ffOutputError (m, "Command", "No command given (\"text\" or --command-text)");
      return;
    }
  ffStrbufInit (&out);
  error = ffVmsSpawnCapture (text, &out, &completion);
  while (out.length && (out.chars[out.length - 1] == '\n' || out.chars[out.length - 1] == '\r'
                        || out.chars[out.length - 1] == ' '))
    out.chars[--out.length] = 0;
  /* And leading blanks and blank lines: DCL output is often indented
     ("  9-OCT-2026 ...") or starts with an empty line (SEARCH/STATISTICS).  */
  {
    uint32_t lead = 0;
    while (lead < out.length && (out.chars[lead] == ' ' || out.chars[lead] == '\t'
                                 || out.chars[lead] == '\n' || out.chars[lead] == '\r'))
      lead++;
    if (lead)
      {
        memmove (out.chars, out.chars + lead, out.length - lead + 1);
        out.length -= lead;
      }
  }
  if (error)
    ffOutputError (m, "Command", error);
  else if (out.length == 0 && !(completion & 1))
    ffOutputError (m, "Command", "The command failed with no output");
  else
    {
      FFformatarg args[] = { ffArgS ("result", out.chars) };
      ffOutput (m, "Command", "{1}", 1, args);
    }
  ffStrbufDestroy (&out);
}

void
ffPrintQueues (const FFModuleArgs *m)
{
  FFQueuesResult q;
  const char *error;

  memset (&q, 0, sizeof q);
  error = ffDetectQueues (&q);
  if (error)
    ffOutputError (m, "Queues", error);
  else
    {
      uint32_t jobs = q.executing + q.pending + q.holding + q.timed;
      FFformatarg args[] = {
        ffArgU ("queues", q.queues),
        ffArgU ("batch", q.batch),
        ffArgU ("output", q.output),
        ffArgU ("generic", q.generic),
        ffArgU ("stopped", q.stopped),
        ffArgU ("jobs", jobs),
        ffArgU ("executing", q.executing),
        ffArgU ("pending", q.pending),
        ffArgU ("holding", q.holding),
        ffArgU ("timed", q.timed),
      };
      /* "4 (2 batch, 2 output), 2 jobs"; the job states are arguments.  */
      ffOutput (m, "Queues",
                "{queues} ({batch} batch, {output} output){?stopped}, {stopped} stopped{?}"
                "{?jobs}, {jobs} jobs{?}",
                sizeof args / sizeof args[0], args);
    }
}
