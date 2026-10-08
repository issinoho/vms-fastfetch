/* title.c - Title ("USER@NODE") and Separator modules.  */

#include <stdio.h>

#include "common/printing.h"
#include "detection/detection.h"
#include "fastfetch.h"
#include "modules/modules.h"

static const FFstrbuf *
titleHost (void)
{
  const FFPlatformResult *p = ffDetectPlatform ();
  return instance.options.titleFqdn && !ffStrbufIsEmpty (&p->fqdn) ? &p->fqdn : &p->hostName;
}

void
ffTitleText (FFstrbuf *out)
{
  ffStrbufAppend (out, &ffDetectPlatform ()->userName);
  ffStrbufAppendC (out, '@');
  ffStrbufAppend (out, titleHost ());
}

void
ffPrintTitle (void)
{
  FFstrbuf *line = ffPrintNewLine ();
  char bold[40];

  /* fastfetch prints user and host bold in the logo's title colour.  */
  snprintf (bold, sizeof bold, "1;%s", instance.titleColor);
  ffAppendColor (line, bold);
  ffStrbufAppend (line, &ffDetectPlatform ()->userName);
  ffAppendReset (line);
  ffStrbufAppendC (line, '@');
  ffAppendColor (line, bold);
  ffStrbufAppend (line, titleHost ());
  ffAppendReset (line);
}

void
ffPrintSeparator (void)
{
  FFstrbuf title;

  ffStrbufInit (&title);
  ffTitleText (&title);
  ffStrbufAppendRepeat (ffPrintNewLine (), '-', title.length);
  ffStrbufDestroy (&title);
}

void
ffPrintBreak (void)
{
  ffPrintNewLine ();
}
