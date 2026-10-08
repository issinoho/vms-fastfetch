/* host_vms.c - hardware model from $GETSYI.  */

#include "detection/detection.h"

#include "vms/vmsapi.h"

#ifdef __VMS
#include <syidef.h>
#endif

const char *
ffDetectHost (FFHostResult *result)
{
#ifdef __VMS
  uint64_t vm = 0;

  if (!FF_VMS_OK (ffVmsSyiStr (SYI$_HW_NAME, &result->name)))
    return "$GETSYI HW_NAME failed";
  /* IA64 pads: "HP rx2660  (1.59GHz/6.0MB)".  */
  ffStrbufCollapseSpaces (&result->name);
  if (FF_VMS_OK (ffVmsSyiU64 (SYI$_VIRTUAL_MACHINE, &vm)))
    result->virtualMachine = vm != 0;
  return ffStrbufIsEmpty (&result->name) ? "SYI$_HW_NAME is empty" : NULL;
#else
  (void) result;
  return "Not supported on this platform";
#endif
}
