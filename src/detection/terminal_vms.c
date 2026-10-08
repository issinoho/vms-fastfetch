/* terminal_vms.c - the terminal on SYS$OUTPUT (D4): device, port kind,
   VT type, remote port and size.

   ssh sessions run on FTA pseudo-terminals with DEVTYPE 0 (unknown); real
   VT terminals and terminals set with SET TERMINAL/INQUIRE report their
   VT series.  */

#include "detection/detection.h"

#include <string.h>

#include "vms/vmsapi.h"

#ifdef __VMS
#include <dcdef.h>
#include <dvidef.h>
#include <ttdef.h>

static const char *
portKind (const char *dev)
{
  static const struct { const char *prefix, *kind; } kinds[] = {
    { "FTA", "pseudo-terminal" },     /* ssh and other PTY users */
    { "TNA", "telnet" },
    { "LTA", "LAT" },
    { "RTA", "DECnet SET HOST" },
    { "TWA", "DECterm" },
    { "OPA", "console" },
    { "TTA", "serial" }, { "TXA", "serial" }, { "TTB", "serial" }, { "TXB", "serial" },
    { 0, 0 }
  };
  int i;

  for (i = 0; kinds[i].prefix; i++)
    if (strncmp (dev, kinds[i].prefix, 3) == 0)
      return kinds[i].kind;
  return "";
}

static const char *
vtType (uint64_t t)
{
  switch (t)
    {
    case TT$_VT52: return "VT52";
    case TT$_VT100: return "VT100";
    case TT$_VT101: return "VT101";
    case TT$_VT102: return "VT102";
    case TT$_VT125: return "VT125";
    case TT$_VT131: return "VT131";
    case TT$_VT132: return "VT132";
    case TT$_VT200_SERIES: return "VT200 series";
    case TT$_VT300_SERIES: return "VT300 series";
    case TT$_VT400_SERIES: return "VT400 series";
    case TT$_VT500_SERIES: return "VT500 series";
    default: return "";
    }
}
#endif

const char *
ffDetectTerminal (FFTerminalResult *r)
{
#ifdef __VMS
  uint64_t cls = 0, type = 0, width = 0, page = 0;
  const char *p;

  if (!FF_VMS_OK (ffVmsDviU64 ("SYS$OUTPUT", DVI$_DEVCLASS, &cls)) || cls != DC$_TERM)
    return "SYS$OUTPUT is not a terminal";
  /* "_NODE$FTA1757:" -> "FTA1757:" */
  ffVmsDviStr ("SYS$OUTPUT", DVI$_FULLDEVNAM, &r->device);
  p = strchr (r->device.chars, '$');
  p = p ? p + 1 : (r->device.chars[0] == '_' ? r->device.chars + 1 : r->device.chars);
  memmove (r->device.chars, p, strlen (p) + 1);
  r->device.length = (uint32_t) strlen (r->device.chars);
  ffStrbufSetS (&r->kind, portKind (r->device.chars));
  if (FF_VMS_OK (ffVmsDviU64 ("SYS$OUTPUT", DVI$_DEVTYPE, &type)))
    ffStrbufSetS (&r->type, vtType (type));
  ffVmsDviStr ("SYS$OUTPUT", DVI$_TT_ACCPORNAM, &r->remote);
  ffVmsDviU64 ("SYS$OUTPUT", DVI$_DEVBUFSIZ, &width);
  ffVmsDviU64 ("SYS$OUTPUT", DVI$_TT_PAGE, &page);
  r->columns = (uint32_t) width;
  r->rows = (uint32_t) page;
  return NULL;
#else
  (void) r;
  return "Not supported on this platform";
#endif
}
