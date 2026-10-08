/* list.c - growable array (see list.h).  */

#include "common/list.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void
ffListInit (FFlist *list, uint32_t elementSize)
{
  list->data = NULL;
  list->elementSize = elementSize;
  list->length = 0;
  list->capacity = 0;
}

void
ffListDestroy (FFlist *list)
{
  free (list->data);
  ffListInit (list, list->elementSize);
}

void *
ffListAdd (FFlist *list)
{
  void *e;

  if (list->length == list->capacity)
    {
      uint32_t cap = list->capacity ? list->capacity * 2 : 16;
      uint8_t *p = realloc (list->data, (size_t) cap * list->elementSize);
      if (!p)
        {
          fputs ("fastfetch: out of memory\n", stderr);
          exit (EXIT_FAILURE);
        }
      list->data = p;
      list->capacity = cap;
    }
  e = ffListGet (list, list->length++);
  memset (e, 0, list->elementSize);
  return e;
}
