/* queues_vms.c - batch and output queues: a wildcard $GETQUI scan.

   Counts queues by kind (batch; output = printer, server or terminal,
   generic or not; generic), stopped queues, and their jobs by state.  */

#include "detection/detection.h"

#include <stdio.h>
#include <string.h>

#include "vms/vmsapi.h"

#ifdef __VMS
#include <iledef.h>
#include <jbcmsgdef.h>
#include <quidef.h>
#include <starlet.h>
#endif

const char *
ffDetectQueues (FFQueuesResult *r)
{
#ifdef __VMS
  static char search[] = "*";
  unsigned int ctx = 0xFFFFFFFF, flags, status, executing, pending, holding, timed;
  /* $GETQUI's IOSB starts with a longword condition (JBC$_ codes have
     facility bits), not the word of $GETSYI's: read as a word,
     JBC$_NOMOREQUE lost its facility and looked like an error.  */
  unsigned int iosb[2];
  unsigned short l[6];
  ile3 il[8];
  int st, i;

  memset (il, 0, sizeof il);
  il[0].ile3$w_length = 1;
  il[0].ile3$w_code = QUI$_SEARCH_NAME;
  il[0].ile3$ps_bufaddr = search;
  {
    static const int codes[] = { QUI$_QUEUE_FLAGS, QUI$_QUEUE_STATUS, QUI$_EXECUTING_JOB_COUNT,
                                 QUI$_PENDING_JOB_COUNT, QUI$_HOLDING_JOB_COUNT,
                                 QUI$_TIMED_RELEASE_JOB_COUNT };
    unsigned int *bufs[] = { &flags, &status, &executing, &pending, &holding, &timed };
    for (i = 0; i < 6; i++)
      {
        il[i + 1].ile3$w_length = sizeof (unsigned int);
        il[i + 1].ile3$w_code = (unsigned short) codes[i];
        il[i + 1].ile3$ps_bufaddr = bufs[i];
        il[i + 1].ile3$ps_retlen_addr = &l[i];
      }
  }
  for (;;)
    {
      flags = status = executing = pending = holding = timed = 0;
      st = sys$getquiw (0, QUI$_DISPLAY_QUEUE, &ctx, il, (void *) iosb, 0, 0);
      if (FF_VMS_OK (st))
        st = iosb[0];
      if (st == JBC$_NOMOREQUE || st == JBC$_NOSUCHQUE)
        break;
      if (!FF_VMS_OK (st))
        {
          static char msg[64];
          sys$getquiw (0, QUI$_CANCEL_OPERATION, &ctx, 0, (void *) iosb, 0, 0);
          snprintf (msg, sizeof msg, "$GETQUI DISPLAY_QUEUE failed: %%X%08X", (unsigned) st);
          return msg;
        }
      r->queues++;
      if (flags & QUI$M_QUEUE_GENERIC)
        r->generic++;
      /* Generic printer and server queues carry neither PRINTER nor SERVER
         (generic batch queues do carry BATCH): every non-batch queue is an
         output queue.  */
      if (flags & QUI$M_QUEUE_BATCH)
        r->batch++;
      else
        r->output++;
      if (status & QUI$M_QUEUE_STOPPED)
        r->stopped++;
      r->executing += executing;
      r->pending += pending;
      r->holding += holding;
      r->timed += timed;
    }
  sys$getquiw (0, QUI$_CANCEL_OPERATION, &ctx, 0, (void *) iosb, 0, 0);
  return r->queues ? NULL : "No queues";
#else
  (void) r;
  return "Not supported on this platform";
#endif
}
