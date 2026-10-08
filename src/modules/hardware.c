/* hardware.c - CPU, Memory, Swap ("Page file") and Disk modules.  */

#include <string.h>

#include "common/printing.h"
#include "detection/detection.h"
#include "fastfetch.h"
#include "modules/modules.h"

void
ffPrintCPU (void)
{
  FFCPUResult cpu;
  const char *error;
  FFstrbuf *line;

  memset (&cpu, 0, sizeof cpu);
  ffStrbufInit (&cpu.vendor);
  ffStrbufInit (&cpu.name);
  ffStrbufInit (&cpu.hypervisor);
  error = ffDetectCPU (&cpu);
  if (error)
    ffPrintError ("CPU", error);
  else
    {
      /* fastfetch's default: "{name} ({cores-online}) @ {freq-max}"  */
      line = ffPrintKey ("CPU");
      ffStrbufAppend (line, &cpu.name);
      if (cpu.coresActive)
        ffStrbufAppendF (line, " (%u)", cpu.coresActive);
      if (cpu.ghz > 0)
        ffStrbufAppendF (line, " @ %.2f GHz", cpu.ghz);
    }
  ffStrbufDestroy (&cpu.vendor);
  ffStrbufDestroy (&cpu.name);
  ffStrbufDestroy (&cpu.hypervisor);
}

void
ffPrintMemory (void)
{
  FFMemoryResult mem;
  const char *error = ffDetectMemory (&mem);

  if (error)
    ffPrintError ("Memory", error);
  else
    ffAppendUsage (ffPrintKey ("Memory"), mem.used, mem.total);
}

void
ffPrintSwap (void)
{
  FFSwapResult swap;
  const char *error = ffDetectSwap (&swap);

  /* D7: page and swap files together, under the VMS name.  */
  if (error)
    ffPrintError ("Page file", error);
  else if (swap.total == 0)
    ffPrintKV ("Page file", "Disabled");
  else
    ffAppendUsage (ffPrintKey ("Page file"), swap.used, swap.total);
}

void
ffPrintDisk (void)
{
  FFlist disks;
  const char *error;
  uint32_t i;

  ffListInit (&disks, sizeof (FFDiskResult));
  error = ffDetectDisks (&disks);
  if (error)
    ffPrintError ("Disk", error);
  for (i = 0; i < disks.length; i++)
    {
      FFDiskResult *d = ffListGet (&disks, i);
      FFstrbuf key;
      FFstrbuf *line;

      /* fastfetch: "Disk (/): 47.42 GiB / 467.84 GiB (10%) - ext4"; the
         volume label stands in for the mount point.  */
      ffStrbufInit (&key);
      ffStrbufAppendF (&key, "Disk (%s)", ffStrbufIsEmpty (&d->label) ? d->device.chars : d->label.chars);
      line = ffPrintKey (key.chars);
      ffAppendUsage (line, d->total - d->free, d->total);
      if (!ffStrbufIsEmpty (&d->filesystem))
        {
          ffStrbufAppendS (line, " - ");
          ffStrbufAppend (line, &d->filesystem);
        }
      ffStrbufDestroy (&key);
      ffStrbufDestroy (&d->label);
      ffStrbufDestroy (&d->logicalVolume);
      ffStrbufDestroy (&d->device);
      ffStrbufDestroy (&d->filesystem);
    }
  ffListDestroy (&disks);
}
