/* processes_vms.c - process count and interactive users: one wildcard
   $GETJPI scan.

   Without WORLD privilege $GETJPI returns SS$_NOPRIV for other users'
   processes outside the group; those are counted as hidden.  A session is
   an interactive process with no owner (subprocesses are not sessions).  */

#include "detection/detection.h"

#include <string.h>

#ifdef __VMS
#include <iledef.h>
#include <jpidef.h>
#include <ssdef.h>
#include <starlet.h>

static void
addUser (FFlist *users, const char *name, unsigned len)
{
  uint32_t i;
  FFUserResult *u;

  while (len && name[len - 1] == ' ')
    len--;
  for (i = 0; i < users->length; i++)
    {
      u = ffListGet (users, i);
      if (u->name.length == len && memcmp (u->name.chars, name, len) == 0)
        {
          u->sessions++;
          return;
        }
    }
  u = ffListAdd (users);
  ffStrbufInit (&u->name);
  ffStrbufAppendNS (&u->name, len, name);
  u->sessions = 1;
}
#endif

const char *
ffDetectProcesses (FFProcessesResult *r)
{
#ifdef __VMS
  unsigned int ctx = 0xFFFFFFFF, mode, owner;
  char user[13];
  unsigned short l1, l2, l3, iosb[4];
  ile3 il[4];
  int st;

  memset (il, 0, sizeof il);
  il[0].ile3$w_length = sizeof mode;
  il[0].ile3$w_code = JPI$_MODE;
  il[0].ile3$ps_bufaddr = &mode;
  il[0].ile3$ps_retlen_addr = &l1;
  il[1].ile3$w_length = sizeof owner;
  il[1].ile3$w_code = JPI$_OWNER;
  il[1].ile3$ps_bufaddr = &owner;
  il[1].ile3$ps_retlen_addr = &l2;
  il[2].ile3$w_length = sizeof user - 1;
  il[2].ile3$w_code = JPI$_USERNAME;
  il[2].ile3$ps_bufaddr = user;
  il[2].ile3$ps_retlen_addr = &l3;
  for (;;)
    {
      mode = owner = 0;
      l3 = 0;
      st = sys$getjpiw (0, &ctx, 0, il, (void *) iosb, 0, 0);
      if (st & 1)
        st = iosb[0];
      if (st == SS$_NOMOREPROC)
        break;
      if (st == SS$_NOPRIV || st == SS$_SUSPENDED)
        {
          r->hidden++;
          continue;
        }
      if (!(st & 1))
        return "$GETJPI wildcard scan failed";
      r->processes++;
      if (mode == JPI$K_INTERACTIVE && owner == 0)
        addUser (&r->users, user, l3);
    }
  return NULL;
#else
  (void) r;
  return "Not supported on this platform";
#endif
}
