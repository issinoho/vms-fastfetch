/* cpuusage_vms.c - CPU usage and load, from $GETRMI (MONITOR's data).

   RMI$_CPUIDLE counts idle ticks summed over all CPUs, 100 per second per
   CPU (probes/probe_rmi2.c: an idle 1-CPU IA64 gained 100 in one second).
   Usage = 1 - idle ticks / (elapsed seconds * 100 * active CPUs).

   OpenVMS keeps no load averages (D15).  The process-state counts
   RMI$_CUR, RMI$_COM and RMI$_COMO are instantaneous; they need a 4-byte
   buffer (8 gives SS$_BADPARAM).  */

#include "detection/detection.h"

#include <string.h>
#include <unistd.h>

#include "vms/vmsapi.h"

#ifdef __VMS
#include <iledef.h>
#include <rmidef.h>
#include <starlet.h>
#include <syidef.h>

static int
rmi32 (int code, uint32_t *out)
{
  unsigned short iosb[4], len = 0;
  ile3 il[2];
  int st;

  *out = 0;
  memset (il, 0, sizeof il);
  il[0].ile3$w_length = sizeof *out;
  il[0].ile3$w_code = (unsigned short) code;
  il[0].ile3$ps_bufaddr = out;
  il[0].ile3$ps_retlen_addr = &len;
  st = sys$getrmi (0, 0, 0, il, (void *) iosb, 0, 0);
  if (FF_VMS_OK (st))
    {
      st = sys$synch (0, (void *) iosb);
      if (FF_VMS_OK (st))
        st = iosb[0];
    }
  return st;
}
#endif

const char *
ffDetectCPUUsage (double *percent, uint32_t waitMs)
{
#ifdef __VMS
  uint64_t idle0, idle1, t0, t1, cpus = 0;
  double elapsed, capacity, busy;

  if (!FF_VMS_OK (ffVmsSyiU64 (SYI$_ACTIVECPU_CNT, &cpus)) || cpus == 0)
    return "$GETSYI ACTIVECPU_CNT failed";
  if (!FF_VMS_OK (ffVmsRmiU64 (RMI$_CPUIDLE, &idle0)) || !FF_VMS_OK (ffVmsGetTime (&t0)))
    return "$GETRMI CPUIDLE failed";
  usleep ((waitMs ? waitMs : 200) * 1000u);
  if (!FF_VMS_OK (ffVmsRmiU64 (RMI$_CPUIDLE, &idle1)) || !FF_VMS_OK (ffVmsGetTime (&t1)))
    return "$GETRMI CPUIDLE failed";
  elapsed = (double) (t1 - t0) / 1e7;            /* 100 ns units */
  capacity = elapsed * 100.0 * (double) cpus;    /* ticks available */
  if (capacity <= 0)
    return "No time elapsed";
  busy = 100.0 * (1.0 - (double) (idle1 - idle0) / capacity);
  *percent = busy < 0 ? 0 : busy > 100 ? 100 : busy;
  return NULL;
#else
  (void) percent;
  (void) waitMs;
  return "Not supported on this platform";
#endif
}

const char *
ffDetectLoad (FFLoadResult *r)
{
#ifdef __VMS
  if (!FF_VMS_OK (rmi32 (RMI$_CUR, &r->current)))
    return "$GETRMI CUR failed";
  rmi32 (RMI$_COM, &r->computable);
  rmi32 (RMI$_COMO, &r->computableOut);
  return NULL;
#else
  (void) r;
  return "Not supported on this platform";
#endif
}
