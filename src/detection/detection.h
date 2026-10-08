/* detection.h - result structures and detectors.

   As in fastfetch, every ffDetect* fills its result and returns NULL on
   success or a static error string.  The implementations are the VMS ones
   (*_vms.c); there is no other platform.  */

#ifndef FF_DETECTION_H
#define FF_DETECTION_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "common/strbuf.h"

typedef struct FFPlatformResult
{
  FFstrbuf userName;            /* JPI$_USERNAME */
  FFstrbuf hostName;            /* SYI$_NODENAME */
  FFstrbuf fqdn;                /* gethostname(): TCP/IP host.domain */
} FFPlatformResult;

typedef struct FFOSResult
{
  FFstrbuf name;                /* "OpenVMS" */
  FFstrbuf id;                  /* "openvms": selects the default logo */
  FFstrbuf version;             /* SYI$_VERSION, e.g. "E9.2-4", "V8.4-2L3" */
  FFstrbuf prettyName;          /* "OpenVMS E9.2-4" */
  FFstrbuf arch;                /* SYI$_ARCH_NAME: "x86_64", "IA64" */
} FFOSResult;

typedef struct FFHostResult
{
  FFstrbuf name;                /* SYI$_HW_NAME, runs of blanks collapsed */
  bool virtualMachine;          /* SYI$_VIRTUAL_MACHINE */
} FFHostResult;

typedef struct FFKernelResult
{
  FFstrbuf sysname;             /* uname() */
  FFstrbuf release;
  FFstrbuf version;
  FFstrbuf machine;
} FFKernelResult;

typedef struct FFUptimeResult
{
  uint64_t uptimeSeconds;
  uint64_t bootTime;            /* VMS time (100 ns since 17-NOV-1858) */
} FFUptimeResult;

/* Detected once, shared by the modules that need them.  */
const FFPlatformResult *ffDetectPlatform (void);
const FFOSResult *ffDetectOS (void);

const char *ffDetectHost (FFHostResult *result);
const char *ffDetectKernel (FFKernelResult *result);
const char *ffDetectUptime (FFUptimeResult *result);

#endif
