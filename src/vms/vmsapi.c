/* vmsapi.c - see vmsapi.h.  */

#include "vms/vmsapi.h"

#include <stdlib.h>
#include <string.h>

#ifdef __VMS

#include <descrip.h>
#include <dcdef.h>
#include <dvidef.h>
#include <iledef.h>
#include <iodef.h>
#include <ssdef.h>
#include <starlet.h>

enum svc { SVC_SYI, SVC_JPI, SVC_DVI, SVC_RMI };

static void
descr (struct dsc$descriptor_s *d, const char *s)
{
  d->dsc$w_length = (unsigned short) strlen (s);
  d->dsc$b_dtype = DSC$K_DTYPE_T;
  d->dsc$b_class = DSC$K_CLASS_S;
  d->dsc$a_pointer = (char *) s;
}

/* One item, one call: buf/len in, *retlen out.  */
static int
getitem (enum svc svc, const char *device, int code, void *buf,
         unsigned short len, unsigned short *retlen)
{
  unsigned short iosb[4];
  struct dsc$descriptor_s d;
  ile3 il[2];
  int st;

  memset (il, 0, sizeof il);
  memset (buf, 0, len);
  *retlen = 0;
  il[0].ile3$w_length = len;
  il[0].ile3$w_code = (unsigned short) code;
  il[0].ile3$ps_bufaddr = buf;
  il[0].ile3$ps_retlen_addr = retlen;
  switch (svc)
    {
    case SVC_SYI:
      st = sys$getsyiw (0, 0, 0, il, (void *) iosb, 0, 0);
      break;
    case SVC_JPI:
      st = sys$getjpiw (0, 0, 0, il, (void *) iosb, 0, 0);
      break;
    case SVC_DVI:
      descr (&d, device);
      st = sys$getdviw (0, 0, &d, il, (void *) iosb, 0, 0, 0);
      break;
    default:
      /* $GETRMI has no W form; synchronise on the IOSB.  */
      st = sys$getrmi (0, 0, 0, il, (void *) iosb, 0, 0);
      if (FF_VMS_OK (st))
        st = sys$synch (0, (void *) iosb);
      break;
    }
  if (FF_VMS_OK (st))
    st = iosb[0];
  return st;
}

static int
getstr (enum svc svc, const char *device, int code, FFstrbuf *out)
{
  char buf[256];
  unsigned short len;
  int st = getitem (svc, device, code, buf, sizeof buf - 1, &len);

  ffStrbufClear (out);
  if (FF_VMS_OK (st))
    {
      ffStrbufAppendNS (out, len, buf);
      ffStrbufTrimRight (out, ' ');
    }
  return st;
}

static int
getu64 (enum svc svc, const char *device, int code, uint64_t *out)
{
  unsigned char buf[8];
  unsigned short len;
  int st = getitem (svc, device, code, buf, sizeof buf, &len);

  *out = 0;
  if (FF_VMS_OK (st))
    memcpy (out, buf, len > 8 ? 8 : len);  /* little-endian on IA64 and x86-64 */
  return st;
}

int ffVmsSyiStr (int code, FFstrbuf *out) { return getstr (SVC_SYI, NULL, code, out); }
int ffVmsSyiU64 (int code, uint64_t *out) { return getu64 (SVC_SYI, NULL, code, out); }
int ffVmsJpiStr (int code, FFstrbuf *out) { return getstr (SVC_JPI, NULL, code, out); }
int ffVmsJpiU64 (int code, uint64_t *out) { return getu64 (SVC_JPI, NULL, code, out); }
int ffVmsDviStr (const char *dev, int code, FFstrbuf *out) { return getstr (SVC_DVI, dev, code, out); }
int ffVmsDviU64 (const char *dev, int code, uint64_t *out) { return getu64 (SVC_DVI, dev, code, out); }
int ffVmsRmiU64 (int code, uint64_t *out) { return getu64 (SVC_RMI, NULL, code, out); }

int
ffVmsGetTime (uint64_t *out)
{
  return sys$gettim ((void *) out);
}

static unsigned short termChannel;

bool
ffVmsTermOpen (void)
{
  struct dsc$descriptor_s d;
  uint64_t cls;

  if (termChannel)
    return true;
  if (!FF_VMS_OK (ffVmsDviU64 ("SYS$OUTPUT", DVI$_DEVCLASS, &cls)) || cls != DC$_TERM)
    return false;
  descr (&d, "SYS$OUTPUT");
  return FF_VMS_OK (sys$assign (&d, &termChannel, 0, 0, 0));
}

/* A zero-length formatted write carrying only FORTRAN carriage control
   (P4): the driver emits the right line feed or carriage return for where
   it believes the cursor is, exactly as for a DCL or CRTL record.  */
static int
carriageControl (char fortran)
{
  unsigned short iosb[4];
  char dummy = 0;
  int st = sys$qiow (0, termChannel, IO$_WRITEVBLK, (void *) iosb, 0, 0, &dummy, 0, 0,
                     (unsigned char) fortran, 0, 0);
  return FF_VMS_OK (st) ? iosb[0] : st;
}

bool
ffVmsTermWrite (const char *buf, uint32_t len)
{
  unsigned short iosb[4];
  int st;

  /* One line, framed like a record: "$" = new line before, nothing after;
     the body with IO$M_NOFORMAT (not counted towards the wrap width);
     "+" = nothing before, carriage return after.  Raw LF/CR in the body
     instead gave blank or overwritten lines next to DCL and CRTL output.  */
  if (!FF_VMS_OK (carriageControl ('$')))
    return false;
  while (len > 0)
    {
      /* Stay well under the terminal driver's maximum buffer (MAXBUF).  */
      uint32_t n = len > 1024 ? 1024 : len;
      st = sys$qiow (0, termChannel, IO$_WRITEVBLK | IO$M_NOFORMAT, (void *) iosb,
                     0, 0, (void *) buf, n, 0, 0, 0, 0);
      if (FF_VMS_OK (st))
        st = iosb[0];
      if (!FF_VMS_OK (st))
        return false;
      buf += n;
      len -= n;
    }
  return FF_VMS_OK (carriageControl ('+'));
}

bool
ffVmsTrnlnm (const char *name, FFstrbuf *out)
{
  /* getenv() searches the same tables as DCL (process, job, group, system)
     and is enough for single-valued names.  */
  const char *v = getenv (name);

  ffStrbufClear (out);
  if (!v)
    return false;
  ffStrbufAppendS (out, v);
  return true;
}

#else /* !__VMS: host-side builds for tests */

#define NOT_VMS 0x2C /* SS$_ABORT, even = failure */

int ffVmsSyiStr (int code, FFstrbuf *out) { (void) code; ffStrbufClear (out); return NOT_VMS; }
int ffVmsSyiU64 (int code, uint64_t *out) { (void) code; *out = 0; return NOT_VMS; }
int ffVmsJpiStr (int code, FFstrbuf *out) { (void) code; ffStrbufClear (out); return NOT_VMS; }
int ffVmsJpiU64 (int code, uint64_t *out) { (void) code; *out = 0; return NOT_VMS; }
int ffVmsDviStr (const char *d, int code, FFstrbuf *out) { (void) d; (void) code; ffStrbufClear (out); return NOT_VMS; }
int ffVmsDviU64 (const char *d, int code, uint64_t *out) { (void) d; (void) code; *out = 0; return NOT_VMS; }
int ffVmsRmiU64 (int code, uint64_t *out) { (void) code; *out = 0; return NOT_VMS; }
int ffVmsGetTime (uint64_t *out) { *out = 0; return NOT_VMS; }
bool ffVmsTermOpen (void) { return false; }
bool ffVmsTermWrite (const char *buf, uint32_t len) { (void) buf; (void) len; return false; }

bool
ffVmsTrnlnm (const char *name, FFstrbuf *out)
{
  const char *v = getenv (name);
  ffStrbufClear (out);
  if (!v)
    return false;
  ffStrbufAppendS (out, v);
  return true;
}

#endif
