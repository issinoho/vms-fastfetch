/* crtl_init.c - set C RTL features before main() runs (LIB$INITIALIZE).

   Each feature is set only if the corresponding DECC$ logical name is not
   defined, so a user or site can still override it.

     DECC$ARGV_PARSE_STYLE  with SET PROCESS/PARSE_STYLE=EXTENDED, keep the
                            case of unquoted arguments (format strings)
     DECC$EFS_CHARSET       ODS-5 names in UNIX-style paths, such as
                            ~/.config's leading dot  */

#ifdef __VMS

#include <stdlib.h>
#include <unixlib.h>

static void
setFeature (const char *name, int value)
{
  int index;

  if (getenv (name))
    return;
  index = decc$feature_get_index (name);
  if (index >= 0)
    decc$feature_set_value (index, 1, value);
}

static void
ffCrtlInit (void)
{
  setFeature ("DECC$ARGV_PARSE_STYLE", 1);
  setFeature ("DECC$EFS_CHARSET", 1);
}

/* Put a pointer to ffCrtlInit in the LIB$INITIALIZE psect, and make sure
   the LIB$INITIALIZE module, which calls it, is linked in.  */
#pragma nostandard
#pragma extern_model save
#pragma extern_model strict_refdef "LIB$INITIALIZE" nopic, con, rel, gbl, noshr, noexe, nowrt, novec, long
extern void (*const ffCrtlInitPtr) (void) = ffCrtlInit;
#pragma extern_model restore

int LIB$INITIALIZE (void);
#pragma extern_model save
#pragma extern_model strict_refdef
int ffLibInitializeRef = (int) LIB$INITIALIZE;
#pragma extern_model restore
#pragma standard

#else
/* ISO C forbids an empty translation unit.  */
typedef int ffCrtlInitUnused;
#endif
