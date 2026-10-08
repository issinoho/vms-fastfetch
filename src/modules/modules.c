/* modules.c - module table and lookup.  */

#include "modules/modules.h"

#include <ctype.h>
#include <stddef.h>

const FFModule ffModules[] = {
  { "Title", "Print title, which contains your user name, hostname", ffPrintTitle },
  { "Separator", "Print a separator line", ffPrintSeparator },
  { "OS", "Print operating system name and version", ffPrintOS },
  { "Host", "Print product name of your computer", ffPrintHost },
  { "Kernel", "Print system kernel version", ffPrintKernel },
  { "Uptime", "Print how long system has been running", ffPrintUptime },
  { "Break", "Print a empty line", ffPrintBreak },
  { "Colors", "Print some colored blocks", ffPrintColors },

  /* Coming in later phases (README).  */
  { "Packages", "List installed package managers and count of installed packages", NULL },
  { "Shell", "Print current shell name and version", NULL },
  { "Terminal", "Print current terminal name and version", NULL },
  { "TerminalSize", "Print current terminal size", NULL },
  { "CPU", "Print CPU name, frequency and core count", NULL },
  { "CPUUsage", "Print CPU usage", NULL },
  { "Memory", "Print system memory usage info", NULL },
  { "Swap", "Print swap (paging file) space usage", NULL },
  { "Disk", "Print partitions, space usage, file system, etc", NULL },
  { "LocalIp", "List local IP addresses (v4 or v6), MAC addresses, etc", NULL },
  { "Locale", "Print system locale name", NULL },
  { "Processes", "Print number of running processes", NULL },
  { "Users", "Print users currently logged in", NULL },
  { "DateTime", "Print current date and time", NULL },
  { "Version", "Print Fastfetch version", NULL },

  /* fastfetch modules with no OpenVMS equivalent.  */
  { "Display", NULL, NULL }, { "DE", NULL, NULL }, { "WM", NULL, NULL },
  { "WMTheme", NULL, NULL }, { "Theme", NULL, NULL }, { "Icons", NULL, NULL },
  { "Font", NULL, NULL }, { "Cursor", NULL, NULL }, { "TerminalFont", NULL, NULL },
  { "GPU", NULL, NULL }, { "Battery", NULL, NULL }, { "PowerAdapter", NULL, NULL },
  { "Media", NULL, NULL }, { "Player", NULL, NULL }, { "Sound", NULL, NULL },
  { "Bluetooth", NULL, NULL }, { "Wifi", NULL, NULL }, { "Brightness", NULL, NULL },
  { NULL, NULL, NULL }
};

const FFModule *
ffModuleFind (const char *name, unsigned n)
{
  const FFModule *m;

  for (m = ffModules; m->name; m++)
    {
      unsigned i;
      for (i = 0; i < n && m->name[i]; i++)
        if (tolower ((unsigned char) m->name[i]) != tolower ((unsigned char) name[i]))
          break;
      if (i == n && m->name[i] == 0)
        return m;
    }
  return NULL;
}
