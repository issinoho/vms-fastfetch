/* config.h - the JSONC configuration file (fastfetch's format) and
   --gen-config.

   Search order when -c is not given (DECISIONS.md D12):
     1. the file named by the logical name FASTFETCH$CONFIG
     2. [<login>.^.config.fastfetch]config.jsonc, built from SYS$LOGIN's
        translation (upstream's ~/.config/fastfetch)
     3. SYS$LOGIN:FASTFETCH.JSONC
   -c <file> loads that file; -c <name> also tries FASTFETCH$PRESETS:<name>.jsonc;
   -c none loads nothing.  */

#ifndef FF_CONFIG_H
#define FF_CONFIG_H

#include <stdbool.h>
#include <stdint.h>

/* Load the configuration (instance.options.config or the search above) and
   apply it to instance.  Returns false after printing an error for an
   explicit -c that cannot be loaded or parsed.  */
bool ffConfigLoad (void);

/* Write a config for the current modules (--gen-config).  */
bool ffConfigGenerate (const char *path, bool force);

/* The default config path (2. above) for this platform.  */
const char *ffConfigDefaultPath (void);

/* Read options from a module's config object (FFModuleArgs.json; NULL is
   allowed and gives the default).  */
const char *ffJsonGetString (const void *obj, const char *key, const char *def);
bool ffJsonGetBool (const void *obj, const char *key, bool def);
int64_t ffJsonGetInt (const void *obj, const char *key, int64_t def);

#endif
