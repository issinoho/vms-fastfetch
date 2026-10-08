/* probe_out.c - how ANSI escapes and long lines come out through printf,
   fwrite and write() on SYS$OUTPUT (record boundaries show up as newlines
   in the fetched log).  */

#include <stdio.h>
#include <string.h>
#include <unistd.h>

int
main (void)
{
  char line[301];
  static const char esc[] = "\033[1;34mBLUE\033[0m \033[38;5;208mORANGE\033[0m";

  memset (line, 'x', 300);
  line[300] = 0;
  printf ("OUT printf-ansi [%s]\n", esc);
  printf ("OUT printf-split [part1 ");
  printf ("part2 ");
  printf ("part3]\n");
  printf ("OUT printf-300 [%s]\n", line);
  fflush (stdout);
  fwrite ("OUT fwrite-split [a ", 1, 20, stdout);
  fwrite ("b]\n", 1, 3, stdout);
  fflush (stdout);
  write (1, "OUT write-split [c ", 19);
  write (1, "d]\n", 3);
  printf ("OUT end\n");
  return 0;
}
