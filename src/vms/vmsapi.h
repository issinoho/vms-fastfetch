/* vmsapi.h - thin wrappers over the OpenVMS system services the detections use.

   Each call fetches one item and returns the VMS condition value (odd =
   success).  String results are trimmed of trailing blanks and NULs.
   Numeric items may be 1, 2, 4 or 8 bytes long (PROBES.md); the buffer is
   zeroed and the returned length honoured, so every size reads correctly
   as a uint64_t.  On non-VMS hosts (host-side tests) every call fails.  */

#ifndef FF_VMSAPI_H
#define FF_VMSAPI_H

#include <stdbool.h>
#include <stdint.h>
#include "common/strbuf.h"

#define FF_VMS_OK(st) (((st) & 1) != 0)

int ffVmsSyiStr (int code, FFstrbuf *out);
int ffVmsSyiU64 (int code, uint64_t *out);
int ffVmsJpiStr (int code, FFstrbuf *out);
int ffVmsJpiU64 (int code, uint64_t *out);
int ffVmsDviStr (const char *device, int code, FFstrbuf *out);
int ffVmsDviU64 (const char *device, int code, uint64_t *out);
int ffVmsRmiU64 (int code, uint64_t *out);

/* Current system time: 100 ns units since 17-NOV-1858 ($GETTIM).  */
int ffVmsGetTime (uint64_t *out);

/* Terminal output (DECISIONS.md D10).  The terminal driver wraps at the
   terminal width counting escape sequences as printable, so coloured lines
   break early.  When SYS$OUTPUT is a terminal, each line's text is written
   with $QIOW IO$_WRITEVBLK|IO$M_NOFORMAT, which the driver does not count,
   between two empty writes whose carriage control lets the driver frame the
   line as it frames any record.  ffVmsTermWrite takes one line, without
   line terminators.  ffVmsTermOpen returns false (use stdout) when
   SYS$OUTPUT is not a terminal.  */
bool ffVmsTermOpen (void);
bool ffVmsTermWrite (const char *buf, uint32_t len);

/* Run one DCL command in a subprocess (LIB$SPAWN, no symbols or keypad
   copied) with its output to a scratch file, and return that output.
   *completion gets the command's final status.  NULL on success, or an
   error string.  Takes 0.1-0.5 s, mostly creating the subprocess.  */
const char *ffVmsSpawnCapture (const char *command, FFstrbuf *output, uint32_t *completion);

/* Translate a logical name (any table, as F$TRNLNM with default tables).  */
bool ffVmsTrnlnm (const char *name, FFstrbuf *out);

#endif
