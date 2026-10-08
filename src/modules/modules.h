/* modules.h - the module registry.

   Names and descriptions follow fastfetch so that -s/--structure strings
   and configs written for it work here.  Modules fastfetch has but OpenVMS
   cannot support are registered with print = NULL and report
   "Not supported on this platform" (shown with --show-errors).  */

#ifndef FF_MODULES_H
#define FF_MODULES_H

#include "common/strbuf.h"

typedef struct FFModule
{
  const char *name;
  const char *description;
  void (*print) (void);
} FFModule;

extern const FFModule ffModules[];

/* Case-insensitive lookup by name (n bytes); NULL if unknown.  */
const FFModule *ffModuleFind (const char *name, unsigned n);

void ffPrintTitle (void);
void ffPrintSeparator (void);
void ffPrintOS (void);
void ffPrintHost (void);
void ffPrintKernel (void);
void ffPrintUptime (void);
void ffPrintBreak (void);
void ffPrintColors (void);

/* Title text as printed (no colours): shared with Separator.  */
void ffTitleText (FFstrbuf *out);

#endif
