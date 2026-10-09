/* detection.h - result structures and detectors.

   As in fastfetch, every ffDetect* fills its result and returns NULL on
   success or a static error string.  The implementations are the VMS ones
   (*_vms.c); there is no other platform.  */

#ifndef FF_DETECTION_H
#define FF_DETECTION_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "common/list.h"
#include "common/strbuf.h"

typedef struct FFPlatformResult
{
  FFstrbuf userName;            /* JPI$_USERNAME */
  FFstrbuf hostName;            /* SYI$_NODENAME */
  FFstrbuf fqdn;                /* gethostname(): TCP/IP host.domain */
  FFstrbuf homeDir;             /* SYS$LOGIN */
  FFstrbuf exePath;             /* JPI$_IMAGNAME */
  FFstrbuf cwd;                 /* current default directory */
  uint64_t uic;                 /* JPI$_UIC */
  uint64_t pid;                 /* JPI$_PID */
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

typedef struct FFCPUResult
{
  FFstrbuf vendor;              /* "GenuineIntel" */
  FFstrbuf name;                /* "Intel(R) Celeron(R) J4005", "Intel Itanium 2 9000 series" */
  FFstrbuf hypervisor;          /* "KVM", "VMware", ... when running as a guest */
  uint32_t coresActive;         /* SYI$_ACTIVECPU_CNT */
  uint32_t coresAvailable;      /* SYI$_AVAILCPU_CNT */
  double ghz;                   /* 0: unknown */
} FFCPUResult;

typedef struct FFMemoryResult
{
  uint64_t total, used, free, modified;   /* bytes */
} FFMemoryResult;

typedef struct FFSwapResult
{
  uint64_t total, used;                   /* bytes: page files + swap files */
} FFSwapResult;

typedef struct FFDiskResult
{
  FFstrbuf label;               /* volume label, "X86SYS" */
  FFstrbuf logicalVolume;       /* "DISK$X86SYS" */
  FFstrbuf device;              /* "DKA0:" */
  FFstrbuf filesystem;          /* "ODS-5" */
  uint64_t total, free;         /* bytes */
  bool systemDisk;
} FFDiskResult;

typedef struct FFLocalIpResult
{
  FFstrbuf interface;           /* "IE0" */
  FFstrbuf address;             /* "10.0.0.5/24" */
} FFLocalIpResult;

typedef struct FFPackagesResult
{
  uint32_t pcsi;                /* installed PCSI products */
} FFPackagesResult;

typedef struct FFTerminalResult
{
  FFstrbuf device;              /* "FTA1757:" */
  FFstrbuf kind;                /* "pseudo-terminal", "telnet", "LAT", ... */
  FFstrbuf type;                /* "VT500 series"; empty when unknown */
  FFstrbuf remote;              /* TT_ACCPORNAM, e.g. a telnet client's address */
  uint32_t columns, rows;
} FFTerminalResult;

typedef struct FFProcessesResult
{
  uint32_t processes;
  uint32_t hidden;              /* processes $GETJPI would not show (NOPRIV) */
  FFlist users;                 /* FFUserResult: interactive sessions by user */
} FFProcessesResult;

typedef struct FFUserResult
{
  FFstrbuf name;
  uint32_t sessions;
} FFUserResult;

typedef struct FFClusterResult
{
  bool member;
  uint32_t nodes, votes, quorum;
} FFClusterResult;

/* Detected once, shared by the modules that need them.  */
const FFPlatformResult *ffDetectPlatform (void);
const FFOSResult *ffDetectOS (void);

const char *ffDetectHost (FFHostResult *result);
const char *ffDetectKernel (FFKernelResult *result);
const char *ffDetectUptime (FFUptimeResult *result);
const char *ffDetectCPU (FFCPUResult *result);
const char *ffDetectMemory (FFMemoryResult *result);
const char *ffDetectSwap (FFSwapResult *result);
/* disks: FFDiskResult, initialised here; mounted local disks, system disk first.  */
const char *ffDetectDisks (FFlist *disks);
/* ips: FFLocalIpResult; IPv4 on interfaces that are up, loopback excluded.  */
const char *ffDetectLocalIps (FFlist *ips);
const char *ffDetectPackages (FFPackagesResult *result);
const char *ffDetectShell (FFstrbuf *result);
const char *ffDetectTerminal (FFTerminalResult *result);
const char *ffDetectProcesses (FFProcessesResult *result);
const char *ffDetectLocale (FFstrbuf *result);
const char *ffDetectCluster (FFClusterResult *result);

#endif
