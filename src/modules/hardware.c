/* hardware.c - CPU, Memory, Swap ("Page file") and Disk modules.

   Argument lists follow fastfetch's, in order; values OpenVMS does not
   have are empty.  */

#include <string.h>

#include "common/printing.h"
#include "detection/detection.h"
#include "fastfetch.h"
#include "modules/modules.h"

static double
percent (uint64_t part, uint64_t whole)
{
  return whole ? (double) part * 100.0 / (double) whole : 0;
}

void
ffPrintCPU (const FFModuleArgs *m)
{
  FFCPUResult cpu;
  const char *error;

  memset (&cpu, 0, sizeof cpu);
  ffStrbufInit (&cpu.vendor);
  ffStrbufInit (&cpu.name);
  ffStrbufInit (&cpu.hypervisor);
  error = ffDetectCPU (&cpu);
  if (error)
    ffOutputError (m, "CPU", error);
  else
    {
      FFformatarg args[] = {
        ffArgS ("name", cpu.name.chars),
        ffArgS ("vendor", cpu.vendor.chars),
        ffArgU ("cores-physical", 0),
        ffArgU ("cores-logical", cpu.coresAvailable),
        ffArgU ("cores-online", cpu.coresActive),
        ffArgGhz ("freq-base", cpu.ghz),
        ffArgGhz ("freq-max", cpu.ghz),
        ffArgS ("temperature", ""),
        ffArgS ("core-types", ""),
        ffArgU ("packages", 0),
        ffArgS ("march", ""),
        ffArgU ("numa-nodes", 0),
        ffArgS ("code-name", ""),
        ffArgS ("technology", ""),
        ffArgS ("hypervisor", cpu.hypervisor.chars),    /* OpenVMS addition */
      };
      /* fastfetch's default: "{name} ({cores-online}) @ {freq-max}"  */
      ffOutput (m, "CPU", "{1}{?5} ({5}){?}{?7} @ {7}{?}", sizeof args / sizeof args[0], args);
    }
  ffStrbufDestroy (&cpu.vendor);
  ffStrbufDestroy (&cpu.name);
  ffStrbufDestroy (&cpu.hypervisor);
}

void
ffPrintMemory (const FFModuleArgs *m)
{
  FFMemoryResult mem;
  const char *error = ffDetectMemory (&mem);

  if (error)
    ffOutputError (m, "Memory", error);
  else
    {
      FFformatarg args[] = {
        ffArgSize ("used", mem.used),
        ffArgSize ("total", mem.total),
        ffArgPercent ("percentage", percent (mem.used, mem.total)),
        ffArgS ("percentage-bar", ""),
      };
      ffOutput (m, "Memory", "{1} / {2} ({3})", sizeof args / sizeof args[0], args);
    }
}

void
ffPrintSwap (const FFModuleArgs *m)
{
  FFSwapResult swap;
  const char *error = ffDetectSwap (&swap);

  /* D7: page and swap files together, under the VMS name.  */
  if (error)
    ffOutputError (m, "Page file", error);
  else
    {
      FFformatarg args[] = {
        ffArgSize ("used", swap.used),
        ffArgSize ("total", swap.total),
        ffArgPercent ("percentage", percent (swap.used, swap.total)),
        ffArgS ("percentage-bar", ""),
        ffArgS ("name", "Page file"),
      };
      ffOutput (m, "Page file", swap.total ? "{1} / {2} ({3})" : "Disabled",
                sizeof args / sizeof args[0], args);
    }
}

void
ffPrintDisk (const FFModuleArgs *m)
{
  FFlist disks;
  const char *error;
  uint32_t i;

  ffListInit (&disks, sizeof (FFDiskResult));
  error = ffDetectDisks (&disks);
  if (error)
    ffOutputError (m, "Disk", error);
  for (i = 0; i < disks.length; i++)
    {
      FFDiskResult *d = ffListGet (&disks, i);
      const char *name = ffStrbufIsEmpty (&d->label) ? d->device.chars : d->label.chars;
      FFformatarg args[] = {
        ffArgSize ("size-used", d->total - d->free),
        ffArgSize ("size-total", d->total),
        ffArgPercent ("size-percentage", percent (d->total - d->free, d->total)),
        ffArgS ("files-used", ""),
        ffArgS ("files-total", ""),
        ffArgS ("files-percentage", ""),
        ffArgB ("is-external", false),
        ffArgB ("is-hidden", false),
        ffArgS ("filesystem", d->filesystem.chars),
        ffArgS ("name", name),
        ffArgB ("is-readonly", false),
        ffArgS ("create-time", ""),
        ffArgS ("size-percentage-bar", ""),
        ffArgS ("files-percentage-bar", ""),
        ffArgU ("days", 0),
        ffArgU ("hours", 0),
        ffArgU ("minutes", 0),
        ffArgU ("seconds", 0),
        ffArgU ("milliseconds", 0),
        ffArgS ("mountpoint", d->logicalVolume.chars),
        ffArgS ("mount-from", d->device.chars),
        ffArgU ("years", 0),
        ffArgU ("days-of-year", 0),
        ffArgD ("years-fraction", 0),
        ffArgSize ("size-free", d->free),
        ffArgSize ("size-available", d->free),
      };
      /* fastfetch: "Disk (/): 47.42 GiB / 467.84 GiB (10%) - ext4"; the
         volume label stands in for the mount point.  */
      ffOutput (m, "Disk ({name})", "{1} / {2} ({3}){?9} - {9}{?}",
                sizeof args / sizeof args[0], args);
      ffStrbufDestroy (&d->label);
      ffStrbufDestroy (&d->logicalVolume);
      ffStrbufDestroy (&d->device);
      ffStrbufDestroy (&d->filesystem);
    }
  ffListDestroy (&disks);
}
