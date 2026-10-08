/* probe_cpuid.c - CPU identification: x86 CPUID (via cpuid_x86.c, clang)
   or IA64 CPUID registers (__getIndReg builtin).  */

#include <stdio.h>
#include <string.h>

#ifdef __x86_64
void ffX86Cpuid (unsigned int leaf, unsigned int subleaf, unsigned int *regs);

static void
show (unsigned int leaf)
{
  unsigned int r[4];
  ffX86Cpuid (leaf, 0, r);
  printf ("CPUID %08X: eax %08X ebx %08X ecx %08X edx %08X\n", leaf, r[0], r[1], r[2], r[3]);
}

int
main (void)
{
  unsigned int r[4], max, maxext, i;
  char vendor[13], brand[49], hv[13];

  ffX86Cpuid (0, 0, r);
  max = r[0];
  memcpy (vendor, &r[1], 4);
  memcpy (vendor + 4, &r[3], 4);
  memcpy (vendor + 8, &r[2], 4);
  vendor[12] = 0;
  printf ("CPUID max leaf %u vendor \"%s\"\n", max, vendor);
  show (1);
  ffX86Cpuid (1, 0, r);
  printf ("CPUID family %u model %u stepping %u hypervisor-bit %u sse4.2 %u avx %u\n",
          ((r[0] >> 8) & 0xF) + ((r[0] >> 20) & 0xFF),
          ((r[0] >> 4) & 0xF) | ((r[0] >> 12) & 0xF0), r[0] & 0xF,
          (r[2] >> 31) & 1, (r[2] >> 20) & 1, (r[2] >> 28) & 1);
  if (max >= 0x16)
    show (0x16);
  ffX86Cpuid (0x80000000, 0, r);
  maxext = r[0];
  printf ("CPUID max extended leaf %08X\n", maxext);
  if (maxext >= 0x80000004)
    {
      for (i = 0; i < 3; i++)
        {
          ffX86Cpuid (0x80000002 + i, 0, r);
          memcpy (brand + 16 * i, r, 16);
        }
      brand[48] = 0;
      printf ("CPUID brand \"%s\"\n", brand);
    }
  ffX86Cpuid (0x40000000, 0, r);
  memcpy (hv, &r[1], 4);
  memcpy (hv + 4, &r[2], 4);
  memcpy (hv + 8, &r[3], 4);
  hv[12] = 0;
  printf ("CPUID hypervisor leaf max %08X signature \"%s\"\n", r[0], hv);
  return 0;
}

#elif defined __ia64

#include <builtins.h>

int
main (void)
{
  unsigned __int64 v[5];
  char vendor[17];
  int i;

  for (i = 0; i < 5; i++)
    {
      v[i] = __getIndReg (_IA64_REG_INDR_CPUID, i);
      printf ("CPUID[%d] %016llX\n", i, v[i]);
    }
  memcpy (vendor, &v[0], 8);
  memcpy (vendor + 8, &v[1], 8);
  vendor[16] = 0;
  printf ("CPUID vendor \"%s\" number %u revision %u model %u family %u archrev %u\n", vendor,
          (unsigned) (v[3] & 0xFF), (unsigned) ((v[3] >> 8) & 0xFF),
          (unsigned) ((v[3] >> 16) & 0xFF), (unsigned) ((v[3] >> 24) & 0xFF),
          (unsigned) ((v[3] >> 32) & 0xFF));
  return 0;
}
#endif
