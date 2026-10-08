/* probe_json.c - yyjson built with VSI C: parse a JSONC config (comments,
   trailing commas) and write it back, pretty and minified.  */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "yyjson.h"

static const char cfg[] =
  "// fastfetch-style config\n"
  "{\n"
  "  \"$schema\": \"https://github.com/fastfetch-cli/fastfetch/raw/dev/doc/json_schema.json\",\n"
  "  /* block comment */\n"
  "  \"logo\": { \"type\": \"builtin\", \"source\": \"openvms\" },\n"
  "  \"display\": { \"separator\": \": \", \"key\": { \"width\": 12 }, },\n"
  "  \"modules\": [\"title\", \"separator\", { \"type\": \"os\", \"key\": \"OS\", \"format\": \"{name} {version}\" },\n"
  "              \"uptime\", 1.5e3, -42, 18446744073709551615, ],\n"
  "}\n";

int
main (void)
{
  yyjson_read_err err;
  yyjson_doc *doc;
  yyjson_val *mods, *v;
  size_t i, n;
  char *out;

  printf ("JSON yyjson %s\n", YYJSON_VERSION_STRING);
  doc = yyjson_read_opts ((char *) cfg, strlen (cfg),
                          YYJSON_READ_ALLOW_COMMENTS | YYJSON_READ_ALLOW_TRAILING_COMMAS,
                          NULL, &err);
  if (!doc)
    {
      printf ("JSON parse failed: %s at %u\n", err.msg, (unsigned) err.pos);
      return 0;
    }
  mods = yyjson_obj_get (yyjson_doc_get_root (doc), "modules");
  printf ("JSON modules: %u entries\n", (unsigned) yyjson_arr_size (mods));
  n = yyjson_arr_size (mods);
  for (i = 0; i < n; i++)
    {
      v = yyjson_arr_get (mods, i);
      if (yyjson_is_str (v))
        printf ("JSON  [%u] string %s\n", (unsigned) i, yyjson_get_str (v));
      else if (yyjson_is_obj (v))
        printf ("JSON  [%u] object type=%s format=%s\n", (unsigned) i,
                yyjson_get_str (yyjson_obj_get (v, "type")),
                yyjson_get_str (yyjson_obj_get (v, "format")));
      else if (yyjson_is_real (v))
        printf ("JSON  [%u] real %g\n", (unsigned) i, yyjson_get_real (v));
      else if (yyjson_is_sint (v))
        printf ("JSON  [%u] sint %lld\n", (unsigned) i, (long long) yyjson_get_sint (v));
      else if (yyjson_is_uint (v))
        printf ("JSON  [%u] uint %llu\n", (unsigned) i, (unsigned long long) yyjson_get_uint (v));
    }
  out = yyjson_write (doc, 0, NULL);
  printf ("JSON minified: %s\n", out ? out : "(null)");
  free (out);
  out = yyjson_write (doc, YYJSON_WRITE_PRETTY, NULL);
  printf ("JSON pretty:\n%s\n", out ? out : "(null)");
  free (out);
  yyjson_doc_free (doc);
  return 0;
}
