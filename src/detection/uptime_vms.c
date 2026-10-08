/* uptime_vms.c - SYI$_BOOTTIME against $GETTIM.  */

#include "detection/detection.h"

#include "vms/vmsapi.h"

#ifdef __VMS
#include <syidef.h>
#endif

const char *
ffDetectUptime (FFUptimeResult *result)
{
#ifdef __VMS
  uint64_t now;

  if (!FF_VMS_OK (ffVmsSyiU64 (SYI$_BOOTTIME, &result->bootTime)))
    return "$GETSYI BOOTTIME failed";
  if (!FF_VMS_OK (ffVmsGetTime (&now)) || now < result->bootTime)
    return "$GETTIM failed";
  result->uptimeSeconds = (now - result->bootTime) / 10000000u;
  return NULL;
#else
  (void) result;
  return "Not supported on this platform";
#endif
}
