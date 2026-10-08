/* memory_vms.c - physical memory and paging/swap file usage (D7).

   $GETSYI has no free-page item: the free and modified page lists come
   from $GETRMI, as MONITOR and SHOW MEMORY report them.  "Used" is what
   SHOW MEMORY calls "In Use": total - free - modified.  */

#include "detection/detection.h"

#include "vms/vmsapi.h"

#ifdef __VMS
#include <rmidef.h>
#include <syidef.h>
#endif

const char *
ffDetectMemory (FFMemoryResult *r)
{
#ifdef __VMS
  uint64_t pages, pagesize, freelist = 0, modlist = 0;

  if (!FF_VMS_OK (ffVmsSyiU64 (SYI$_MEMSIZE, &pages))
      || !FF_VMS_OK (ffVmsSyiU64 (SYI$_PAGE_SIZE, &pagesize)))
    return "$GETSYI MEMSIZE/PAGE_SIZE failed";
  if (!FF_VMS_OK (ffVmsRmiU64 (RMI$_FRLIST, &freelist)))
    return "$GETRMI FRLIST failed";
  ffVmsRmiU64 (RMI$_MODLIST, &modlist);
  r->total = pages * pagesize;
  r->free = freelist * pagesize;
  r->modified = modlist * pagesize;
  r->used = r->total > r->free + r->modified ? r->total - r->free - r->modified : 0;
  return NULL;
#else
  (void) r;
  return "Not supported on this platform";
#endif
}

const char *
ffDetectSwap (FFSwapResult *r)
{
#ifdef __VMS
  uint64_t pagesize, pf = 0, pffree = 0, sf = 0, sffree = 0;

  if (!FF_VMS_OK (ffVmsSyiU64 (SYI$_PAGE_SIZE, &pagesize)))
    return "$GETSYI PAGE_SIZE failed";
  /* Sizes are in pages (SHOW MEMORY: "Paging File Usage (8KB pages)").  */
  ffVmsSyiU64 (SYI$_PAGEFILE_PAGE, &pf);
  ffVmsSyiU64 (SYI$_PAGEFILE_FREE, &pffree);
  ffVmsSyiU64 (SYI$_SWAPFILE_PAGE, &sf);
  ffVmsSyiU64 (SYI$_SWAPFILE_FREE, &sffree);
  r->total = (pf + sf) * pagesize;
  r->used = (pf + sf > pffree + sffree ? pf + sf - pffree - sffree : 0) * pagesize;
  return NULL;
#else
  (void) r;
  return "Not supported on this platform";
#endif
}
