/* cluster_vms.c - OpenVMS Cluster membership (a VMS-only module).  */

#include "detection/detection.h"

#include "vms/vmsapi.h"

#ifdef __VMS
#include <syidef.h>
#endif

const char *
ffDetectCluster (FFClusterResult *r)
{
#ifdef __VMS
  uint64_t v = 0;

  if (!FF_VMS_OK (ffVmsSyiU64 (SYI$_CLUSTER_MEMBER, &v)))
    return "$GETSYI CLUSTER_MEMBER failed";
  r->member = (v & 1) != 0;
  if (!r->member)
    return "Not a cluster member";
  if (FF_VMS_OK (ffVmsSyiU64 (SYI$_CLUSTER_NODES, &v)))
    r->nodes = (uint32_t) v;
  if (FF_VMS_OK (ffVmsSyiU64 (SYI$_CLUSTER_VOTES, &v)))
    r->votes = (uint32_t) v;
  if (FF_VMS_OK (ffVmsSyiU64 (SYI$_CLUSTER_QUORUM, &v)))
    r->quorum = (uint32_t) v;
  return NULL;
#else
  (void) r;
  return "Not supported on this platform";
#endif
}
