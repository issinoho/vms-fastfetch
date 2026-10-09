/* title.c - Title ("USER@NODE"), Separator, Break, Colors and Custom.  */

#include <stdio.h>
#include <string.h>

#include "common/config.h"
#include "common/printing.h"
#include "detection/detection.h"
#include "fastfetch.h"
#include "modules/modules.h"

static const FFstrbuf *
titleHost (bool fqdn)
{
  const FFPlatformResult *p = ffDetectPlatform ();
  return fqdn && !ffStrbufIsEmpty (&p->fqdn) ? &p->fqdn : &p->hostName;
}

void
ffTitleText (FFstrbuf *out)
{
  ffStrbufAppend (out, &ffDetectPlatform ()->userName);
  ffStrbufAppendC (out, '@');
  ffStrbufAppend (out, titleHost (instance.options.titleFqdn));
}

void
ffPrintTitle (const FFModuleArgs *m)
{
  const FFPlatformResult *p = ffDetectPlatform ();
  const FFstrbuf *host = titleHost (ffJsonGetBool (m->json, "fqdn", instance.options.titleFqdn));
  FFstrbuf user, at, hostc, shell;
  char bold[80];

  /* fastfetch prints user and host bold in the logo's title colour.  */
  snprintf (bold, sizeof bold, "1;%s", instance.titleColor);
  ffStrbufInit (&user);
  ffStrbufInit (&at);
  ffStrbufInit (&hostc);
  ffStrbufInit (&shell);
  ffAppendColor (&user, bold);
  ffStrbufAppend (&user, &p->userName);
  ffAppendReset (&user);
  ffStrbufAppendC (&at, '@');
  ffAppendColor (&hostc, bold);
  ffStrbufAppend (&hostc, host);
  ffAppendReset (&hostc);
  ffDetectShell (&shell);
  {
    FFformatarg args[] = {
      ffArgS ("user-name", p->userName.chars),
      ffArgS ("host-name", host->chars),
      ffArgS ("home-dir", p->homeDir.chars),
      ffArgS ("exe-path", p->exePath.chars),
      ffArgS ("user-shell", shell.chars),
      ffArgRaw ("user-name-colored", user.chars),
      ffArgRaw ("at-symbol-colored", at.chars),
      ffArgRaw ("host-name-colored", hostc.chars),
      ffArgS ("full-user-name", ""),
      ffArgU ("user-id", p->uic),
      ffArgU ("pid", p->pid),
      ffArgS ("cwd", p->cwd.chars),
    };
    ffOutput (m, NULL, "{6}{7}{8}", sizeof args / sizeof args[0], args);
  }
  ffStrbufDestroy (&user);
  ffStrbufDestroy (&at);
  ffStrbufDestroy (&hostc);
  ffStrbufDestroy (&shell);
}

void
ffPrintSeparator (const FFModuleArgs *m)
{
  const char *s = ffJsonGetString (m->json, "string", "-");
  int64_t length = ffJsonGetInt (m->json, "length", 0);
  FFstrbuf title, line;
  uint32_t i, n = (uint32_t) strlen (s);

  if (n == 0)
    s = "-", n = 1;
  ffStrbufInit (&title);
  ffStrbufInit (&line);
  ffTitleText (&title);
  if (length <= 0)
    length = title.length;
  /* The string repeated up to the title's width (fastfetch).  */
  for (i = 0; i < (uint32_t) length; i++)
    ffStrbufAppendC (&line, s[i % n]);
  ffOutputLine (line.chars);
  ffStrbufDestroy (&title);
  ffStrbufDestroy (&line);
}

void
ffPrintBreak (const FFModuleArgs *m)
{
  (void) m;
  ffOutputLine ("");
}

void
ffPrintColors (const FFModuleArgs *m)
{
  int64_t pad = ffJsonGetInt (m->json, "paddingLeft", 0);
  FFstrbuf line;
  int row, i;

  /* Two rows of blocks, normal (40-47) and bright (100-107) backgrounds.
     Text only: block characters are not ASCII (DECterm, VTs).  */
  if (instance.options.pipe)
    return;
  ffStrbufInit (&line);
  for (row = 0; row < 2; row++)
    {
      ffStrbufClear (&line);
      ffStrbufAppendRepeat (&line, ' ', pad > 0 ? (uint32_t) pad : 0);
      for (i = 0; i < 8; i++)
        ffStrbufAppendF (&line, "\033[%dm   ", (row ? 100 : 40) + i);
      ffStrbufAppendS (&line, "\033[0m");
      ffOutputLine (line.chars);
    }
  ffStrbufDestroy (&line);
}

void
ffPrintCustom (const FFModuleArgs *m)
{
  /* The format is the text; a key is printed only if one is given.  */
  if (instance.options.json)
    return;
  ffOutput (m, NULL, m->format ? m->format : "", 0, NULL);
}
