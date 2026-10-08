/* fastfetch.h - program-wide options and state.  */

#ifndef FF_FASTFETCH_H
#define FF_FASTFETCH_H

#include <stdbool.h>
#include <stdint.h>

#include "common/list.h"
#include "common/strbuf.h"
#include "logo/logo.h"

#define FF_PROGRAM_VERSION "0.1.0"
#define FF_UPSTREAM_VERSION "2.70.0"   /* the fastfetch release we follow */

/* fastfetch's default, minus the modules OpenVMS cannot support, plus Cluster
   (shown only on cluster members).  */
#define FF_DEFAULT_STRUCTURE \
  "Title:Separator:OS:Host:Kernel:Uptime:Cluster:Packages:Shell:Terminal:CPU:Memory:Swap:Disk:LocalIp:Locale:Break:Colors"

typedef struct FFOptions
{
  const char *structure;
  const char *logoName;         /* NULL: the OS default */
  bool pipe;                    /* no escape sequences */
  bool pipeSet;                 /* --pipe given explicitly */
  bool showErrors;
  uint32_t logoPaddingLeft;
  uint32_t logoPaddingRight;
  uint32_t logoPaddingTop;
  const char *keySeparator;
  uint32_t keyWidth;            /* 0: no alignment */
  bool titleFqdn;
} FFOptions;

typedef struct FFInstance
{
  FFOptions options;
  const FFlogo *logo;
  FFlist lines;                 /* FFstrbuf: the info column, one per line */
  char keyColor[32];            /* SGR parameters, e.g. "1;34" */
  char titleColor[32];
} FFInstance;

extern FFInstance instance;

#endif
