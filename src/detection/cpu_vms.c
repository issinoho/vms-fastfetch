/* cpu_vms.c - CPU name, vendor, clock and count.

   x86-64: CPUID (src/vms/cpuid_x86.c): brand string, hypervisor signature.
   IA64:   the CPUID registers (__getIndReg): vendor, family and model,
           mapped to the Itanium product name; the clock is in HW_NAME,
           e.g. "HP rx2660 (1.59GHz/6.0MB)".
   Both:   SYI$_ACTIVECPU_CNT / AVAILCPU_CNT.  */

#include "detection/detection.h"

#include <stdlib.h>
#include <string.h>

#include "vms/vmsapi.h"

#ifdef __VMS
#include <syidef.h>
#endif
#if defined __VMS && defined __ia64
#include <builtins.h>
#endif

/* "@ 2.00GHz" or "(1.59GHz/..." anywhere in s: the number before "GHz".  */
static double
parseGhz (const char *s)
{
  const char *g = strstr (s, "GHz");
  const char *start;

  if (!g)
    return 0;
  start = g;
  while (start > s && (start[-1] == '.' || (start[-1] >= '0' && start[-1] <= '9')))
    start--;
  return start == g ? 0 : atof (start);
}

#if defined __VMS && defined __x86_64
void ffX86Cpuid (unsigned int leaf, unsigned int subleaf, unsigned int *regs);

static void
detectX86 (FFCPUResult *r)
{
  unsigned int reg[4], max, i;
  char buf[49];

  ffX86Cpuid (0, 0, reg);
  max = reg[0];
  memcpy (buf, &reg[1], 4);
  memcpy (buf + 4, &reg[3], 4);
  memcpy (buf + 8, &reg[2], 4);
  ffStrbufSetNS (&r->vendor, 12, buf);

  ffX86Cpuid (0x80000000, 0, reg);
  if (reg[0] >= 0x80000004)
    {
      for (i = 0; i < 3; i++)
        {
          ffX86Cpuid (0x80000002 + i, 0, reg);
          memcpy (buf + 16 * i, reg, 16);
        }
      buf[48] = 0;
      ffStrbufSetS (&r->name, buf);
      r->ghz = parseGhz (buf);
    }

  /* Leaf 1 ECX bit 31: running under a hypervisor; 0x40000000: its name.  */
  ffX86Cpuid (1, 0, reg);
  if (reg[2] & 0x80000000u)
    {
      static const struct { const char *sig, *name; } hv[] = {
        { "KVMKVMKVM", "KVM" }, { "VMwareVMware", "VMware" },
        { "Microsoft Hv", "Hyper-V" }, { "VBoxVBoxVBox", "VirtualBox" },
        { "TCGTCGTCGTCG", "QEMU" }, { "XenVMMXenVMM", "Xen" }, { 0, 0 } };
      ffX86Cpuid (0x40000000, 0, reg);
      memcpy (buf, &reg[1], 4);
      memcpy (buf + 4, &reg[2], 4);
      memcpy (buf + 8, &reg[3], 4);
      buf[12] = 0;
      ffStrbufSetS (&r->hypervisor, "hypervisor");
      for (i = 0; hv[i].sig; i++)
        if (strncmp (buf, hv[i].sig, strlen (hv[i].sig)) == 0)
          ffStrbufSetS (&r->hypervisor, hv[i].name);
    }

  /* Leaf 0x16: base frequency in MHz, when the brand string has none.  */
  if (r->ghz == 0 && max >= 0x16)
    {
      ffX86Cpuid (0x16, 0, reg);
      if (reg[0])
        r->ghz = reg[0] / 1000.0;
    }
}
#endif

#if defined __VMS && defined __ia64
static void
detectIA64 (FFCPUResult *r)
{
  unsigned __int64 v0 = __getIndReg (_IA64_REG_INDR_CPUID, 0);
  unsigned __int64 v1 = __getIndReg (_IA64_REG_INDR_CPUID, 1);
  unsigned __int64 v3 = __getIndReg (_IA64_REG_INDR_CPUID, 3);
  unsigned family = (unsigned) ((v3 >> 24) & 0xFF);
  unsigned model = (unsigned) ((v3 >> 16) & 0xFF);
  char vendor[17];
  const char *name;

  memcpy (vendor, &v0, 8);
  memcpy (vendor + 8, &v1, 8);
  vendor[16] = 0;
  ffStrbufSetS (&r->vendor, vendor);
  /* Intel's family/model numbers for the Itanium generations.  */
  switch (family)
    {
    case 0x07: name = "Itanium"; break;
    case 0x1F: name = "Itanium 2"; break;
    case 0x20: name = model == 0 ? "Itanium 2 9000 series" : "Itanium 2 9100 series"; break;
    case 0x21: name = "Itanium 9300 series"; break;
    case 0x22: name = model == 0 ? "Itanium 9500 series" : "Itanium 9700 series"; break;
    default: name = "Itanium"; break;
    }
  ffStrbufSetS (&r->name, strcmp (vendor, "GenuineIntel") == 0 ? "Intel " : "");
  ffStrbufAppendS (&r->name, name);
}
#endif

/* "Intel(R) Celeron(R) J4005 CPU @ 2.00GHz" -> "Intel(R) Celeron(R) J4005":
   the clock is printed separately, and " CPU" says nothing (as fastfetch).  */
static void
tidyName (FFstrbuf *name)
{
  char *at = strstr (name->chars, " @ ");
  char *cpu;

  if (at)
    name->length = (uint32_t) (at - name->chars);
  if (name->allocated)
    name->chars[name->length] = 0;
  while ((cpu = strstr (name->chars, " CPU")) && (cpu[4] == ' ' || cpu[4] == 0))
    memmove (cpu, cpu + 4, strlen (cpu + 4) + 1), name->length -= 4;
  while (name->length && name->chars[0] == ' ')
    memmove (name->chars, name->chars + 1, name->length--);
  ffStrbufCollapseSpaces (name);
  ffStrbufTrimRight (name, ' ');
}

const char *
ffDetectCPU (FFCPUResult *r)
{
#ifdef __VMS
  uint64_t n;
  FFstrbuf hw;

  if (FF_VMS_OK (ffVmsSyiU64 (SYI$_ACTIVECPU_CNT, &n)))
    r->coresActive = (uint32_t) n;
  if (FF_VMS_OK (ffVmsSyiU64 (SYI$_AVAILCPU_CNT, &n)))
    r->coresAvailable = (uint32_t) n;
#if defined __VMS && defined __x86_64
  detectX86 (r);
#elif defined __ia64
  detectIA64 (r);
#endif
  if (r->ghz == 0)
    {
      ffStrbufInit (&hw);
      ffVmsSyiStr (SYI$_HW_NAME, &hw);
      r->ghz = parseGhz (hw.chars);
      ffStrbufDestroy (&hw);
    }
  if (ffStrbufIsEmpty (&r->name))
    ffStrbufSetS (&r->name, ffDetectOS ()->arch.chars);
  tidyName (&r->name);
  return NULL;
#else
  (void) r;
  (void) parseGhz;
  (void) tidyName;
  return "Not supported on this platform";
#endif
}
