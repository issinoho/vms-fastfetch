/* shell_vms.c - command language interpreter and locale.  */

#include "detection/detection.h"

#include <locale.h>
#include <stdlib.h>
#include <string.h>

#include "vms/vmsapi.h"

#ifdef __VMS
#include <jpidef.h>
#endif

const char *
ffDetectShell (FFstrbuf *result)
{
  /* Under GNV bash, SHELL names the shell; under DCL it is unset (or "DCL").  */
  const char *shell = getenv ("SHELL");

  if (shell && *shell && strcmp (shell, "DCL") != 0)
    {
      const char *base = strrchr (shell, '/');
      ffStrbufSetS (result, base ? base + 1 : shell);
      if (getenv ("BASH_VERSION"))
        ffStrbufAppendF (result, " %s", getenv ("BASH_VERSION"));
      return NULL;
    }
#ifdef __VMS
  if (FF_VMS_OK (ffVmsJpiStr (JPI$_CLINAME, result)) && !ffStrbufIsEmpty (result))
    return NULL;
#endif
  return "No CLI";
}

const char *
ffDetectLocale (FFstrbuf *result)
{
  static const char *vars[] = { "LC_ALL", "LC_CTYPE", "LANG", NULL };
  const char *v;
  int i;

  for (i = 0; vars[i]; i++)
    if ((v = getenv (vars[i])) && *v)
      {
        ffStrbufSetS (result, v);
        return NULL;
      }
  v = setlocale (LC_CTYPE, "");
  ffStrbufSetS (result, v && *v ? v : "C");
  return NULL;
}
