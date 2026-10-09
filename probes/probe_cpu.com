$! PROBE_CPU.COM - Phase 2 probe: CPU identification on this node.
$!   x86-64: cpuid_x86.c compiled with VSI clang, linked with VSI C.
$!   IA64:   __getIndReg(_IA64_REG_INDR_CPUID) in VSI C.
$ set noon
$ proc = f$environment("PROCEDURE")
$ set default 'f$parse(proc,,,"DEVICE")''f$parse(proc,,,"DIRECTORY")'
$ arch = f$edit(f$getsyi("ARCH_NAME"),"UPCASE")
$ obj = "[.OBJ_''arch']"
$ if f$search("OBJ_''arch'.DIR") .eqs. "" then create/directory 'obj'
$ open/read cf [-.vms.config]ccflags.txt
$ read cf ccflags
$ close cf
$ objs = "''obj'probe_cpuid.obj"
$ if arch .nes. "X86_64" then goto vsic
$ clang :== $sys$system:clang.exe
$ define/user sys$error sys$output
$ clang --version
$ define/user sys$error sys$output
$ clang -c -O2 -fno-builtin-memset -o cpuid_x86.obj cpuid_x86.c
$ write sys$output "--- clang status ", $status
$ rename cpuid_x86.obj 'obj'
$ objs = objs + ",''obj'cpuid_x86.obj"
$vsic:
$ cc 'ccflags'/object='obj'probe_cpuid.obj probe_cpuid.c
$ write sys$output "--- cc status ", $status
$ link/executable='obj'probe_cpuid.exe 'objs'
$ write sys$output "--- link status ", $status
$ run 'obj'probe_cpuid.exe
$ write sys$output "--- run status ", $status
$! Facts for the other Phase 2 modules.
$ write sys$output "--- terminal type constants"
$ library/extract=TTDEF/output='obj'ttdef.h sys$library:sys$starlet_c.tlb
$ search/nohead 'obj'ttdef.h "#define TT$_VT","#define TT$_UNKNOWN","#define TT$_FT"
$ write sys$output "--- ACP types"
$ library/extract=DVIDEF/output='obj'dvidef.h sys$library:sys$starlet_c.tlb
$ search/nohead 'obj'dvidef.h "ACP$K_","ACPTYPE"
