/* probe_rmi2.c - Phase 5: $GETRMI process-state counts (for a Loadavg
   stand-in) with different buffer sizes, and RMI$_CPUIDLE scaling
   (idle ticks over one second against the CPU count).  */

#include <stdio.h>
#include <string.h>
#include <starlet.h>
#include <rmidef.h>
#include <syidef.h>
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

int
main (void)
{
  static const struct { const char *name; int code; } items[] = {
    { "COM", RMI$_COM }, { "COMO", RMI$_COMO }, { "CUR", RMI$_CUR },
    { "LEF", RMI$_LEF }, { "HIB", RMI$_HIB }, { "PROCS", RMI$_PROCS }, { 0, 0 } };
  static const unsigned short sizes[] = { 1, 2, 4, 8, 16, 64 };
  unsigned __int64 a = 0, b = 0;
  unsigned short len;
  int i, s, st;

  for (i = 0; items[i].name; i++)
    for (s = 0; s < 6; s++)
      {
        unsigned char buf[64];
        memset (buf, 0, sizeof buf);
        len = 0;
        st = rmi (items[i].code, buf, sizes[s], &len);
        printf ("RMI %-5s size %2u: status %%X%08X len %u first-long %u\n", items[i].name,
                sizes[s], st, len, *(unsigned int *) buf);
      }
  rmi (RMI$_CPUIDLE, &a, 8, &len);
  sleep (1);
  rmi (RMI$_CPUIDLE, &b, 8, &len);
  printf ("RMI CPUIDLE delta over 1 s: %llu\n", b - a);
  return 0;
}
