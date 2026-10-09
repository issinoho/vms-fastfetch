/* platform_vms.c - user, node and host names; the OS identity.  */

#include "detection/detection.h"

#include <string.h>
#include <unistd.h>

#include "vms/vmsapi.h"

#ifdef __VMS
#include <jpidef.h>
#include <syidef.h>
#endif

const FFPlatformResult *
ffDetectPlatform (void)
{
  static FFPlatformResult r;
  static bool done;
  char host[256];

  if (done)
    return &r;
  done = true;
  ffStrbufInit (&r.userName);
  ffStrbufInit (&r.hostName);
  ffStrbufInit (&r.fqdn);
  ffStrbufInit (&r.homeDir);
  ffStrbufInit (&r.exePath);
  ffStrbufInit (&r.cwd);
#ifdef __VMS
  ffVmsJpiStr (JPI$_USERNAME, &r.userName);
  ffVmsSyiStr (SYI$_NODENAME, &r.hostName);
  ffVmsJpiStr (JPI$_IMAGNAME, &r.exePath);
  ffVmsJpiU64 (JPI$_UIC, &r.uic);
  ffVmsJpiU64 (JPI$_PID, &r.pid);
  ffVmsTrnlnm ("SYS$LOGIN", &r.homeDir);
  /* The CRTL's PATH is the current default directory, in VMS syntax.  */
  ffVmsTrnlnm ("PATH", &r.cwd);
#else
  ffVmsTrnlnm ("USER", &r.userName);
  ffVmsTrnlnm ("HOME", &r.homeDir);
  ffVmsTrnlnm ("PWD", &r.cwd);
  r.pid = (uint64_t) getpid ();
#endif
  if (gethostname (host, sizeof host) == 0)
    {
      host[sizeof host - 1] = 0;
      ffStrbufSetS (&r.fqdn, host);
    }
  /* Off VMS (host tests) or without a node name, fall back to TCP/IP.  */
  if (ffStrbufIsEmpty (&r.hostName))
    {
      const char *dot = strchr (r.fqdn.chars, '.');
      ffStrbufSetNS (&r.hostName, dot ? (uint32_t) (dot - r.fqdn.chars) : r.fqdn.length,
                     r.fqdn.chars);
    }
  return &r;
}

const FFOSResult *
ffDetectOS (void)
{
  static FFOSResult r;
  static bool done;

  if (done)
    return &r;
  done = true;
  ffStrbufInitS (&r.name, "OpenVMS");
  ffStrbufInitS (&r.id, "openvms");
  ffStrbufInit (&r.version);
  ffStrbufInit (&r.prettyName);
  ffStrbufInit (&r.arch);
#ifdef __VMS
  ffVmsSyiStr (SYI$_VERSION, &r.version);
  ffVmsSyiStr (SYI$_ARCH_NAME, &r.arch);
#endif
  ffStrbufAppend (&r.prettyName, &r.name);
  if (!ffStrbufIsEmpty (&r.version))
    {
      ffStrbufAppendC (&r.prettyName, ' ');
      ffStrbufAppend (&r.prettyName, &r.version);
    }
  return &r;
}
