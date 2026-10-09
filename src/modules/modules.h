/* modules.h - the module registry and per-instance module settings.

   Names and descriptions follow fastfetch so that -s/--structure strings
   and configs written for it work here.  Modules fastfetch has but OpenVMS
   cannot support are registered with print = NULL and report
   "Not supported on this platform" (shown with --show-errors).  */

#ifndef FF_MODULES_H
#define FF_MODULES_H

#include "common/strbuf.h"

/* Settings of one module instance: from a "modules" entry in the config,
   from --<module>-key/-format/... options, or defaults (all NULL/"").  */
typedef struct FFModuleArgs
{
  const char *key;              /* format string for the key; NULL: default */
  const char *format;           /* NULL: the module's default format */
  char keyColor[64];            /* SGR; "": display.color.keys */
  char outputColor[64];         /* SGR; "": display.color.output */
  const void *json;             /* yyjson_val *: the module's config object, or NULL */
} FFModuleArgs;

typedef struct FFModule
{
  const char *name;
  const char *description;
  void (*print) (const FFModuleArgs *args);
} FFModule;

typedef struct FFModuleInstance
{
  const FFModule *module;
  FFModuleArgs args;
} FFModuleInstance;

extern const FFModule ffModules[];
extern const unsigned ffModuleCount;

/* Case-insensitive lookup by name (n bytes); NULL if unknown.  */
const FFModule *ffModuleFind (const char *name, unsigned n);

void ffPrintTitle (const FFModuleArgs *args);
void ffPrintSeparator (const FFModuleArgs *args);
void ffPrintOS (const FFModuleArgs *args);
void ffPrintHost (const FFModuleArgs *args);
void ffPrintKernel (const FFModuleArgs *args);
void ffPrintUptime (const FFModuleArgs *args);
void ffPrintBreak (const FFModuleArgs *args);
void ffPrintColors (const FFModuleArgs *args);
void ffPrintCustom (const FFModuleArgs *args);
void ffPrintCPU (const FFModuleArgs *args);
void ffPrintMemory (const FFModuleArgs *args);
void ffPrintSwap (const FFModuleArgs *args);
void ffPrintDisk (const FFModuleArgs *args);
void ffPrintPackages (const FFModuleArgs *args);
void ffPrintShell (const FFModuleArgs *args);
void ffPrintTerminal (const FFModuleArgs *args);
void ffPrintTerminalSize (const FFModuleArgs *args);
void ffPrintLocalIp (const FFModuleArgs *args);
void ffPrintLocale (const FFModuleArgs *args);
void ffPrintProcesses (const FFModuleArgs *args);
void ffPrintUsers (const FFModuleArgs *args);
void ffPrintDateTime (const FFModuleArgs *args);
void ffPrintVersion (const FFModuleArgs *args);
void ffPrintCluster (const FFModuleArgs *args);
void ffPrintCPUUsage (const FFModuleArgs *args);
void ffPrintLoadavg (const FFModuleArgs *args);
void ffPrintDNS (const FFModuleArgs *args);
void ffPrintCommand (const FFModuleArgs *args);
void ffPrintQueues (const FFModuleArgs *args);

/* Title text as printed (no colours): shared with Separator.  */
void ffTitleText (FFstrbuf *out);

#endif
