/* probe_c99.c - the C99 features and CRTL calls vms-fastfetch's core relies on.
   A clean compile with /STANDARD=C99 is the main result; the run checks the
   CRTL behaviour.  */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
#include <ctype.h>
#include <locale.h>
#include <time.h>
#include <errno.h>

#define LOG(...) printf ("C99 " __VA_ARGS__)

struct flex { size_t n; char data[]; };
struct opt { const char *key; int width; bool on; };

static inline int
twice (int x)
{
  return x * 2;
}

static int
vsn (char *buf, size_t sz, const char *fmt, ...)
{
  va_list ap, aq;
  int n;
  va_start (ap, fmt);
  va_copy (aq, ap);
  n = vsnprintf (NULL, 0, fmt, aq);
  va_end (aq);
  vsnprintf (buf, sz, fmt, ap);
  va_end (ap);
  return n;
}

int
main (void)
{
  struct opt o = { .key = "OS", .on = true };
  struct opt *list = (struct opt[]) { { "a", 1, true }, { "b", 2, false } };
  struct flex *f = malloc (sizeof *f + 6);
  uint64_t big = UINT64_C (18446744073709551615);
  char buf[16];
  int n;
  time_t now = time (NULL);
  struct tm *tm = localtime (&now);
  char tbuf[64];

  for (int i = 0; i < 1; i++)
    LOG ("for-init decl ok, __func__=%s\n", __func__);
  LOG ("designated %s %d %d, compound %s, inline %d\n", o.key, o.width, o.on,
       list[1].key, twice (21));
  f->n = 5;
  memcpy (f->data, "hello", 6);
  LOG ("flexible array %s\n", f->data);
  free (f);
  LOG ("PRIu64 %" PRIu64 " PRIx64 %" PRIx64 "\n", big, big);
  /* No %zu: VSI C 7.4 (IA64) rejects it with CC-W-UNSUPCONVSPEC.  */
  LOG ("sizeof %lu %%lld %lld %%llu %llu\n", (unsigned long) sizeof (struct opt),
       -1LL, 1ULL << 40);
  n = vsn (buf, sizeof buf, "%s-%d", "truncated-string", 12345);
  LOG ("vsnprintf returned %d buf \"%s\"\n", n, buf);
  n = snprintf (buf, 4, "abcdef");
  LOG ("snprintf(4,abcdef) returned %d buf \"%s\"\n", n, buf);
  LOG ("strtoull %llu errno %d\n", strtoull ("18446744073709551615", NULL, 10), errno);
  LOG ("sizeof int %u long %u long long %u void* %u size_t %u\n",
       (unsigned) sizeof (int), (unsigned) sizeof (long), (unsigned) sizeof (long long),
       (unsigned) sizeof (void *), (unsigned) sizeof (size_t));
  LOG ("setlocale(LC_ALL,\"\") -> %s\n", setlocale (LC_ALL, "") ? setlocale (LC_ALL, NULL) : "(null)");
  strftime (tbuf, sizeof tbuf, "%Y-%m-%d %H:%M:%S %Z", tm);
  LOG ("strftime %s\n", tbuf);
#ifdef __STDC_VERSION__
  LOG ("__STDC_VERSION__ %ldL\n", (long) __STDC_VERSION__);
#endif
#ifdef __DECC_VER
  LOG ("__DECC_VER %d\n", __DECC_VER);
#endif
#ifdef __CRTL_VER
  LOG ("__CRTL_VER %d\n", __CRTL_VER);
#endif
#ifdef __INITIAL_POINTER_SIZE
  LOG ("__INITIAL_POINTER_SIZE %d\n", __INITIAL_POINTER_SIZE);
#endif
  return 0;
}
