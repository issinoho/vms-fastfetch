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

#define FF_SCHEMA_URL "https://github.com/fastfetch-cli/fastfetch/raw/dev/doc/json_schema.json"

typedef enum FFSizePrefix { FF_SIZE_IEC, FF_SIZE_SI, FF_SIZE_JEDEC } FFSizePrefix;

typedef struct FFOptions
{
  const char *structure;        /* -s; NULL: the config's modules, else the default */
  const char *config;           /* -c; NULL: search the usual places; "none": no config */
  const char *logoName;         /* NULL: the OS default; "none": no logo */
  const char *logoType;         /* auto, builtin, small, file, data, none */
  bool pipe;                    /* no escape sequences */
  bool pipeSet;                 /* --pipe or display.pipe given explicitly */
  bool showErrors;
  bool json;                    /* --format json */
  uint32_t logoPaddingLeft;
  uint32_t logoPaddingRight;
  uint32_t logoPaddingTop;
  const char *keySeparator;
  uint32_t keyWidth;            /* 0: no alignment */
  bool titleFqdn;
  FFSizePrefix sizePrefix;
  uint32_t sizeDigits;
  uint32_t percentGreen;        /* below: green */
  uint32_t percentYellow;       /* below: yellow; above: red */
  const char *genConfig;        /* --gen-config path ("" = default) or NULL */
  bool genConfigForce;
} FFOptions;

typedef struct FFInstance
{
  FFOptions options;
  const FFlogo *logo;
  FFlogo logoStore;             /* the logo in use, with colour overrides applied */
  char logoColors[FF_LOGO_MAX_COLORS][64];
  FFstrbuf logoData;            /* logo text from a file or "data" */
  FFlist modules;               /* FFModuleInstance */
  FFlist lines;                 /* FFstrbuf: the info column, one per line */
  char keyColor[64];            /* SGR parameters, e.g. "1;34" */
  char titleColor[64];
  char outputColor[64];
  bool keyColorSet, titleColorSet;
  /* --format json */
  void *jsonDoc;                /* yyjson_mut_doc * */
  void *jsonArray;              /* yyjson_mut_val *: the result array */
  void *jsonEntry;              /* the current module's entry, or NULL */
  const char *currentModule;
} FFInstance;

extern FFInstance instance;

#endif
