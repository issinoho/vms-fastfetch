/* list.h - growable array of fixed-size elements (fastfetch's FFlist).  */

#ifndef FF_LIST_H
#define FF_LIST_H

#include <stdint.h>

typedef struct FFlist
{
  uint8_t *data;
  uint32_t elementSize;
  uint32_t length;
  uint32_t capacity;
} FFlist;

void ffListInit (FFlist *list, uint32_t elementSize);
void ffListDestroy (FFlist *list);
/* Append a zero-filled element and return it.  */
void *ffListAdd (FFlist *list);

static inline void *
ffListGet (const FFlist *list, uint32_t index)
{
  return list->data + (uint64_t) index * list->elementSize;
}

#endif
