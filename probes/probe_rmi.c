/* probe_rmi.c - $GETRMI (the data MONITOR uses): free and modified page
   lists, and CPU mode ticks sampled twice a second apart, for Memory and
   CPUUsage.  */

#include <stdio.h>
#include <string.h>
#include <starlet.h>
#include <rmidef.h>
#include <ssdef.h>
#include <iledef.h>
#include <unistd.h>

static int
rmi (int code, void *buf, unsigned short sz, unsigned short *len)
{
  unsigned short iosb[4];
  ile3 il[2];
  int st;

  memset (il, 0, sizeof il);
  il[0].ile3$w_length = sz;
  il[0].ile3$w_code = code;
  il[0].ile3$ps_bufaddr = buf;
  il[0].ile3$ps_retlen_addr = len;
  st = sys$getrmi (0, 0, 0, il, (void *) iosb, 0, 0);
  if (st & 1)
    {
      st = sys$synch (0, (void *) iosb);
      if (st & 1)
        st = iosb[0];
    }
  return st;
}

static void
num (const char *name, int code)
{
  unsigned __int64 v = 0;
  unsigned short len = 0;
  int st = rmi (code, &v, sizeof v, &len);
  if (st & 1)
    printf ("RMI %-14s %llu (len %u)\n", name, v, len);
  else
    printf ("RMI %-14s status %%X%08X\n", name, st);
}

int
main (void)
{
  unsigned char a[4096], b[4096];
  unsigned short la = 0, lb = 0;
  int st, i;

  memset (a, 0, sizeof a);
  memset (b, 0, sizeof b);

  num ("FRLIST", RMI$_FRLIST);
  num ("MODLIST", RMI$_MODLIST);
#ifdef RMI$_CPUIDLE
  num ("CPUIDLE", RMI$_CPUIDLE);
#endif
#ifdef RMI$_COM
  num ("COM", RMI$_COM);
#endif
#ifdef RMI$_COMO
  num ("COMO", RMI$_COMO);
#endif
#ifdef RMI$_CUR
  num ("CUR", RMI$_CUR);
#endif
#ifdef RMI$_PROCS
  num ("PROCS", RMI$_PROCS);
#endif

  /* RMI$_MODES: per-mode ticks summed over all CPUs; the layout is part of
     what we are probing, so dump the raw longwords of two samples.  */
  st = rmi (RMI$_MODES, a, sizeof a, &la);
  sleep (1);
  if (st & 1)
    st = rmi (RMI$_MODES, b, sizeof b, &lb);
  if (!(st & 1))
    {
      printf ("RMI MODES status %%X%08X\n", st);
      return 0;
    }
  /* x86 returned retlen 0 with data present: dump a fixed window.  */
  printf ("RMI MODES len %u, delta of each longword over 1 s:\n", la);
  if (la == 0)
    la = 256;
  for (i = 0; i + 4 <= la && i < 256; i += 4)
    {
      unsigned int x, y;
      memcpy (&x, a + i, 4);
      memcpy (&y, b + i, 4);
      printf ("RMI MODES[%3d] %10u -> %10u  delta %u\n", i / 4, x, y, y - x);
    }
  return 0;
}
