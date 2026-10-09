/* packages_vms.c - installed PCSI products (D5).

   Counting SYS$COMMON:[SYSEXE]*.PCSI$DATABASE is wrong (IA64: 82 files for
   55 products), so PRODUCT SHOW PRODUCT * runs in a subprocess and its
   "Installed" lines are counted.  This costs about 0.2-0.5 s, most of it
   creating the subprocess.  */

#include "detection/detection.h"

#include <string.h>

#include "vms/vmsapi.h"

const char *
ffDetectPackages (FFPackagesResult *r)
{
  FFstrbuf out;
  uint32_t completion = 0;
  const char *error, *p;

  ffStrbufInit (&out);
  error = ffVmsSpawnCapture ("PRODUCT SHOW PRODUCT *", &out, &completion);
  /* "VSI X86VMS C V7.7-3        Full LP     Installed"  */
  for (p = out.chars; !error && *p;)
    {
      const char *eol = strchr (p, '\n');
      size_t len = eol ? (size_t) (eol - p) : strlen (p);
      size_t n = len;
      while (n && (p[n - 1] == ' ' || p[n - 1] == '\r'))
        n--;
      if (n > 10 && memcmp (p + n - 10, " Installed", 10) == 0)
        r->pcsi++;
      p = eol ? eol + 1 : p + len;
    }
  ffStrbufDestroy (&out);
  if (error)
    return error;
  if (!(completion & 1) && r->pcsi == 0)
    return "PRODUCT SHOW PRODUCT failed";
  return NULL;
}
