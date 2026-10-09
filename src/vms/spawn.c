/* spawn.c - run a DCL command in a subprocess and capture its output.  */

#include "vms/vmsapi.h"

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

const char *
ffVmsSpawnCapture (const char *command, FFstrbuf *output, uint32_t *completion)
{
  static unsigned serial;
  struct dsc$descriptor_s cmd, out;
  unsigned int flags = CLI$M_NOCLISYM | CLI$M_NOKEYPAD;
  unsigned int status = 0;
  char file[80], line[1024];
  FILE *f;
  int st;

  snprintf (file, sizeof file, "SYS$SCRATCH:FASTFETCH_%08X_%u.TMP", (unsigned) getpid (), serial++);
  descr (&cmd, command);
  descr (&out, file);
  ffStrbufClear (output);
  st = lib$spawn (&cmd, 0, &out, &flags, 0, 0, &status, 0, 0, 0, 0, 0, 0);
  if (completion)
    *completion = status;
  if (!(st & 1))
    return "LIB$SPAWN failed";
  f = fopen (file, "r");
  if (!f)
    return "The command produced no output file";
  while (fgets (line, sizeof line, f))
    ffStrbufAppendS (output, line);
  fclose (f);
  remove (file);
  return NULL;
}

#else

const char *
ffVmsSpawnCapture (const char *command, FFstrbuf *output, uint32_t *completion)
{
  (void) command;
  ffStrbufClear (output);
  if (completion)
    *completion = 0;
  return "Not supported on this platform";
}

#endif
