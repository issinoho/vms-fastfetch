/* cpuid_x86.c - the x86 CPUID instruction, for VSI C callers.

   VSI C has no inline assembly and no CPUID builtin (PROBES.md), so this
   one file is compiled with VSI's clang (x86-64 only) and linked with the
   VSI C objects.  The interface is integers and a 4-word array; a 32-bit
   VSI C pointer arrives sign-extended, which is a valid 64-bit address.  */

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
