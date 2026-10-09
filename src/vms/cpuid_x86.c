/* cpuid_x86.c - the x86 CPUID instruction, for the VSI C code (x86-64 only).

   VSI C has no inline assembly and no CPUID builtin (PROBES.md), so this
   one file is compiled with VSI's clang by BUILD.COM and linked with the
   VSI C objects (DECISIONS.md D11).  The interface is integers and a
   4-word array: a 32-bit VSI C pointer arrives sign-extended, which is a
   valid 64-bit address.  BUILD.COM compiles it with -fno-builtin-memset,
   as all clang code in the vms-* family (VSI clang's memset -> OTS$FILL
   miscompile).  */

void
ffX86Cpuid (unsigned int leaf, unsigned int subleaf, unsigned int *regs)
{
  unsigned int a, b, c, d;

  __asm__ __volatile__ ("cpuid"
                        : "=a" (a), "=b" (b), "=c" (c), "=d" (d)
                        : "a" (leaf), "c" (subleaf));
  regs[0] = a;
  regs[1] = b;
  regs[2] = c;
  regs[3] = d;
}
