/* packages_vms.c - installed PCSI products (D5).

   Counting SYS$COMMON:[SYSEXE]*.PCSI$DATABASE is wrong (IA64: 82 files for
   55 products), so PRODUCT SHOW PRODUCT * runs in a subprocess (LIB$SPAWN,
   output to a scratch file) and its "Installed" lines are counted.  This
   costs about 0.2-0.5 s, most of it creating the subprocess.  */

#include "detection/detection.h"

#include <stdio.h>
#include <string.h>

#ifdef __VMS
#include <clidef.h>
#include <descrip.h>
#include <lib$routines.h>
#include <unistd.h>

static void
descr (struct dsc$descriptor_s *d, const char *s)
{
  d->dsc$w_length = (unsigned short) strlen (s);
  d->dsc$b_dtype = DSC$K_DTYPE_T;
  d->dsc$b_class = DSC$K_CLASS_S;
  d->dsc$a_pointer = (char *) s;
}
#endif

const char *
ffDetectPackages (FFPackagesResult *r)
{
#ifdef __VMS
  struct dsc$descriptor_s cmd, out;
  unsigned int flags = CLI$M_NOCLISYM | CLI$M_NOKEYPAD;
  unsigned int completion = 0;
  char file[64], line[512];
  FILE *f;
  int st;

  snprintf (file, sizeof file, "SYS$SCRATCH:FASTFETCH_PCSI_%08X.TMP", (unsigned) getpid ());
  descr (&cmd, "PRODUCT SHOW PRODUCT *");
  descr (&out, file);
  st = lib$spawn (&cmd, 0, &out, &flags, 0, 0, &completion, 0, 0, 0, 0, 0, 0);
  if (!(st & 1))
    return "LIB$SPAWN of PRODUCT failed";
  f = fopen (file, "r");
  if (!f)
    return "PRODUCT SHOW PRODUCT produced no output";
  /* "VSI X86VMS C V7.7-3        Full LP     Installed"  */
  while (fgets (line, sizeof line, f))
    {
      size_t n = strlen (line);
      while (n && (line[n - 1] == '\n' || line[n - 1] == ' ' || line[n - 1] == '\r'))
        line[--n] = 0;
      if (n > 10 && strcmp (line + n - 10, " Installed") == 0)
        r->pcsi++;
    }
  fclose (f);
  remove (file);
  if (!(completion & 1) && r->pcsi == 0)
    return "PRODUCT SHOW PRODUCT failed";
  return NULL;
#else
  (void) r;
  return "Not supported on this platform";
#endif
}
