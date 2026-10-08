/* disk_vms.c - mounted disks: $DEVICE_SCAN for disk-class devices, then
   $GETDVI for each.

   Shown: mounted (not foreign) local disks and shadow sets.  Skipped:
   shadow-set members (the DSA virtual unit is shown instead), devices
   served by another node, and memory-disk boot images (volumes SYS$MD*).
   The system disk is listed first.  */

#include "detection/detection.h"

#include <string.h>

#include "vms/vmsapi.h"

#ifdef __VMS
#include <dcdef.h>
#include <descrip.h>
#include <dvidef.h>
#include <dvsdef.h>
#include <iledef.h>
#include <ssdef.h>
#include <starlet.h>

/* DVI$_ACPTYPE codes (F$GETDVI names them F11V2, F11V5, ...).  */
static const char *
acpName (uint64_t acp)
{
  switch (acp)
    {
    case 1: return "ODS-1";
    case 2: return "ODS-2";
    case 7: return "ODS-3";
    case 8: return "ODS-4";
    case 11: return "ODS-5";
    case 12: return "ODS-6";
    default: return "";
    }
}

static bool
flag (const char *dev, int code)
{
  uint64_t v = 0;
  return FF_VMS_OK (ffVmsDviU64 (dev, code, &v)) && v != 0;
}

static void
addDisk (FFlist *disks, const char *dev, const FFstrbuf *sysdev)
{
  FFDiskResult *d;
  FFstrbuf name, label;
  uint64_t maxblock = 0, freeblocks = 0, acp = 0;
  const char *p;

  if (!flag (dev, DVI$_MNT) || flag (dev, DVI$_FOR) || flag (dev, DVI$_SHDW_MEMBER)
      || flag (dev, DVI$_REMOTE_DEVICE))
    return;
  ffStrbufInit (&label);
  ffVmsDviStr (dev, DVI$_VOLNAM, &label);
  if (strncmp (label.chars, "MD", 2) == 0)
    {
      /* Memory-disk boot images: logical volume SYS$MD..., label MD... */
      FFstrbuf lv;
      bool md;
      ffStrbufInit (&lv);
      ffVmsDviStr (dev, DVI$_LOGVOLNAM, &lv);
      md = strncmp (lv.chars, "SYS$MD", 6) == 0;
      ffStrbufDestroy (&lv);
      if (md)
        {
          ffStrbufDestroy (&label);
          return;
        }
    }
  ffVmsDviU64 (dev, DVI$_MAXBLOCK, &maxblock);
  ffVmsDviU64 (dev, DVI$_FREEBLOCKS, &freeblocks);
  ffVmsDviU64 (dev, DVI$_ACPTYPE, &acp);

  d = ffListAdd (disks);
  d->label = label;
  ffStrbufInit (&d->logicalVolume);
  ffStrbufInit (&d->device);
  ffStrbufInit (&d->filesystem);
  ffVmsDviStr (dev, DVI$_LOGVOLNAM, &d->logicalVolume);
  /* "_NODE$DKA0:" -> "DKA0:" */
  ffStrbufInit (&name);
  ffVmsDviStr (dev, DVI$_FULLDEVNAM, &name);
  p = strchr (name.chars, '$');
  p = p ? p + 1 : (name.chars[0] == '_' ? name.chars + 1 : name.chars);
  ffStrbufSetS (&d->device, p);
  d->systemDisk = strcmp (name.chars, sysdev->chars) == 0;
  ffStrbufDestroy (&name);
  ffStrbufSetS (&d->filesystem, acpName (acp));
  d->total = maxblock * 512;
  d->free = freeblocks * 512;
}
#endif

const char *
ffDetectDisks (FFlist *disks)
{
#ifdef __VMS
  char name[65];
  unsigned short len;
  unsigned int devclass = DC$_DISK;
  unsigned __int64 ctx = 0;
  struct dsc$descriptor_s nd;
  ile3 sel[2];
  FFstrbuf sysdev;
  uint32_t i;
  int st;

  ffStrbufInit (&sysdev);
  ffVmsDviStr ("SYS$SYSDEVICE", DVI$_FULLDEVNAM, &sysdev);
  memset (sel, 0, sizeof sel);
  sel[0].ile3$w_length = sizeof devclass;
  sel[0].ile3$w_code = DVS$_DEVCLASS;
  sel[0].ile3$ps_bufaddr = &devclass;
  nd.dsc$w_length = sizeof name - 1;
  nd.dsc$b_dtype = DSC$K_DTYPE_T;
  nd.dsc$b_class = DSC$K_CLASS_S;
  nd.dsc$a_pointer = name;
  for (;;)
    {
      len = 0;
      st = sys$device_scan (&nd, &len, 0, sel, &ctx);
      if (!FF_VMS_OK (st))
        break;
      name[len] = 0;
      addDisk (disks, name, &sysdev);
    }
  ffStrbufDestroy (&sysdev);
  if (st != SS$_NOMOREDEV && st != SS$_NOSUCHDEV)
    return "$DEVICE_SCAN failed";

  /* System disk first, the rest in scan order.  */
  for (i = 1; i < disks->length; i++)
    {
      FFDiskResult *d = ffListGet (disks, i);
      if (d->systemDisk)
        {
          FFDiskResult tmp = *d;
          memmove (ffListGet (disks, 1), ffListGet (disks, 0), (size_t) i * sizeof tmp);
          *(FFDiskResult *) ffListGet (disks, 0) = tmp;
          break;
        }
    }
  return disks->length ? NULL : "No mounted disks found";
#else
  (void) disks;
  return "Not supported on this platform";
#endif
}
