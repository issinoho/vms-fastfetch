/* kernel_vms.c - the CRTL's uname().  */

#include "detection/detection.h"

#include <sys/utsname.h>

const char *
ffDetectKernel (FFKernelResult *result)
{
  struct utsname u;

  if (uname (&u) < 0)
    return "uname() failed";
  ffStrbufSetS (&result->sysname, u.sysname);
  ffStrbufSetS (&result->release, u.release);
  ffStrbufSetS (&result->version, u.version);
  ffStrbufSetS (&result->machine, u.machine);
  ffStrbufTrimRight (&result->sysname, ' ');
  ffStrbufTrimRight (&result->release, ' ');
  ffStrbufTrimRight (&result->version, ' ');
  ffStrbufTrimRight (&result->machine, ' ');
#ifdef __VMS
  /* The CRTL's uname() gives release "0" and the OpenVMS version
     ("E9.2-4", "V8.4-2L3") in version; machine is the HW_NAME, mangled and
     truncated.  fastfetch's Kernel shows "{sysname} {release}", so report
     the version as the release, and ARCH_NAME as the machine.  */
  ffStrbufSetNS (&result->release, result->version.length, result->version.chars);
  ffStrbufSetNS (&result->machine, ffDetectOS ()->arch.length, ffDetectOS ()->arch.chars);
#endif
  return NULL;
}
