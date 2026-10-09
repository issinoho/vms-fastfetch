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
  { "Custom", "Print a custom string, with or without key", ffPrintCustom },
  { "Packages", "List installed package managers and count of installed packages", ffPrintPackages },
  { "Shell", "Print current shell name and version", ffPrintShell },
  { "Terminal", "Print current terminal device, kind and type", ffPrintTerminal },
  { "TerminalSize", "Print current terminal size", ffPrintTerminalSize },
  { "CPU", "Print CPU name, frequency and core count", ffPrintCPU },
  { "Memory", "Print system memory usage info", ffPrintMemory },
  { "Swap", "Print page and swap file usage", ffPrintSwap },
  { "Disk", "Print mounted volumes, space usage, file system", ffPrintDisk },
  { "LocalIp", "List local IPv4 addresses", ffPrintLocalIp },
  { "Locale", "Print system locale name", ffPrintLocale },
  { "Processes", "Print number of running processes", ffPrintProcesses },
  { "Users", "Print users with interactive sessions", ffPrintUsers },
  { "DateTime", "Print current date and time", ffPrintDateTime },
  { "Version", "Print Fastfetch version", ffPrintVersion },
  { "Cluster", "Print OpenVMS Cluster membership, nodes and votes (OpenVMS only)", ffPrintCluster },
  { "CPUUsage", "Print CPU usage (all CPUs, sampled for waitTime ms)", ffPrintCPUUsage },
  { "Loadavg", "Print runnable processes now (OpenVMS keeps no load averages)", ffPrintLoadavg },
  { "DNS", "Print configured DNS servers", ffPrintDNS },
  { "Command", "Run a DCL command and print its output", ffPrintCommand },
  { "Queues", "Print batch and output queues and their jobs (OpenVMS only)", ffPrintQueues },

  /* fastfetch modules with no OpenVMS equivalent.  */
  { "Display", NULL, NULL }, { "DE", NULL, NULL }, { "WM", NULL, NULL },
  { "WMTheme", NULL, NULL }, { "Theme", NULL, NULL }, { "Icons", NULL, NULL },
  { "Font", NULL, NULL }, { "Cursor", NULL, NULL }, { "TerminalFont", NULL, NULL },
  { "GPU", NULL, NULL }, { "Battery", NULL, NULL }, { "PowerAdapter", NULL, NULL },
  { "Media", NULL, NULL }, { "Player", NULL, NULL }, { "Sound", NULL, NULL },
  { "Bluetooth", NULL, NULL }, { "Wifi", NULL, NULL }, { "Brightness", NULL, NULL },
  { NULL, NULL, NULL }
};

const unsigned ffModuleCount = sizeof ffModules / sizeof ffModules[0] - 1;

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
