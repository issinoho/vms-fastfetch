/* probe_jpi.c - $GETJPI items for this process, a wildcard process scan,
   and the environment variables fastfetch reads.  */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <starlet.h>
#include <jpidef.h>
#include <ssdef.h>
#include <iledef.h>
#include <unistd.h>

static int
jpi_str (unsigned int *pid, int code, char *out, size_t outsz)
{
  unsigned short len = 0, iosb[4];
  ile3 il[2];
  int st;

  memset (il, 0, sizeof il);
  il[0].ile3$w_length = (unsigned short) (outsz - 1);
  il[0].ile3$w_code = code;
  il[0].ile3$ps_bufaddr = out;
  il[0].ile3$ps_retlen_addr = &len;
  st = sys$getjpiw (0, pid, 0, il, (void *) iosb, 0, 0);
  if (st & 1)
    st = iosb[0];
  out[(st & 1) ? len : 0] = 0;
  while ((st & 1) && len > 0 && out[len - 1] == ' ')
    out[--len] = 0;
  return st;
}

static int
jpi_num (unsigned int *pid, int code, unsigned int *out)
{
  unsigned short len = 0, iosb[4];
  ile3 il[2];
  int st;

  *out = 0;
  memset (il, 0, sizeof il);
  il[0].ile3$w_length = sizeof *out;
  il[0].ile3$w_code = code;
  il[0].ile3$ps_bufaddr = out;
  il[0].ile3$ps_retlen_addr = &len;
  st = sys$getjpiw (0, pid, 0, il, (void *) iosb, 0, 0);
  if (st & 1)
    st = iosb[0];
  return st;
}

static void
show_str (const char *name, int code)
{
  char buf[256];
  int st = jpi_str (0, code, buf, sizeof buf);
  if (st & 1)
    printf ("JPI %-14s \"%s\"\n", name, buf);
  else
    printf ("JPI %-14s status %%X%08X\n", name, st);
}

static void
show_num (const char *name, int code)
{
  unsigned int v;
  int st = jpi_num (0, code, &v);
  if (st & 1)
    printf ("JPI %-14s %u (%%X%08X)\n", name, v, v);
  else
    printf ("JPI %-14s status %%X%08X\n", name, st);
}

int
main (void)
{
  static const char *envs[] = {
    "USER", "LOGNAME", "HOME", "SHELL", "TERM", "PATH", "LANG", "LC_ALL",
    "LC_CTYPE", "COLUMNS", "LINES", "EDITOR", "VISUAL", "TZ", "XDG_CONFIG_HOME",
    "SYS$LOGIN", "SYS$SCRATCH", "SYS$SYSDEVICE", "SYS$NODE", "SYS$TIMEZONE_NAME",
    "SYS$TIMEZONE_DIFFERENTIAL", "SYS$LANGUAGE", "TCPIP$INET_HOST",
    "TCPIP$INET_DOMAIN", "UCX$INET_HOST", "DECW$DISPLAY", "DISPLAY", 0
  };
  unsigned int ctx = 0xFFFFFFFF, pid, mode, total = 0, inter = 0, batch = 0,
    net = 0, other = 0, nopriv = 0;
  char host[256];
  int i, st;

  show_str ("USERNAME", JPI$_USERNAME);
  show_str ("CLINAME", JPI$_CLINAME);
  show_str ("TERMINAL", JPI$_TERMINAL);
  show_str ("PRCNAM", JPI$_PRCNAM);
  show_str ("IMAGNAME", JPI$_IMAGNAME);
  show_num ("PID", JPI$_PID);
  show_num ("MODE", JPI$_MODE);
  show_num ("UIC", JPI$_UIC);
  show_num ("JOBTYPE", JPI$_JOBTYPE);

  if (gethostname (host, sizeof host) == 0)
    printf ("CRT gethostname   \"%s\"\n", host);
  printf ("CRT getlogin      \"%s\"\n", getlogin () ? getlogin () : "(null)");
  printf ("CRT isatty(1)     %d\n", isatty (1));
  for (i = 0; envs[i]; i++)
    {
      const char *v = getenv (envs[i]);
      printf ("ENV %-26s %s%s%s\n", envs[i], v ? "\"" : "", v ? v : "(unset)",
              v ? "\"" : "");
    }

  /* Wildcard scan: count processes by mode.  */
  for (;;)
    {
      unsigned short iosb[4];
      ile3 il[3];
      unsigned short l1, l2;
      memset (il, 0, sizeof il);
      il[0].ile3$w_length = 4;
      il[0].ile3$w_code = JPI$_PID;
      il[0].ile3$ps_bufaddr = &pid;
      il[0].ile3$ps_retlen_addr = &l1;
      il[1].ile3$w_length = 4;
      il[1].ile3$w_code = JPI$_MODE;
      il[1].ile3$ps_bufaddr = &mode;
      il[1].ile3$ps_retlen_addr = &l2;
      st = sys$getjpiw (0, &ctx, 0, il, (void *) iosb, 0, 0);
      if (st & 1)
        st = iosb[0];
      if (st == SS$_NOMOREPROC)
        break;
      if (st == SS$_NOPRIV || st == SS$_SUSPENDED)
        {
          nopriv++;
          total++;
          continue;
        }
      if (!(st & 1))
        {
          printf ("SCAN status %%X%08X\n", st);
          break;
        }
      total++;
      if (mode == JPI$K_INTERACTIVE)
        inter++;
      else if (mode == JPI$K_BATCH)
        batch++;
      else if (mode == JPI$K_NETWORK)
        net++;
      else
        other++;
    }
  printf ("SCAN total %u interactive %u batch %u network %u other %u nopriv/susp %u\n",
          total, inter, batch, net, other, nopriv);
  return 0;
}
