$! BUILD.COM - build vms-fastfetch for OpenVMS
$!
$! Usage:  @[.VMS]BUILD [target] [KEEP_GOING]
$!         target defaults to ALL; CLEAN also works.  KEEP_GOING carries on
$!         past failed compiles so one run reports every error.
$!
$! Runs from the top of the source tree regardless of where it is invoked
$! from.  Output: [.BIN_<arch>]FASTFETCH.EXE
$!
$ status = 44  ! SS$_ABORT unless the build runs
$ on control_y then goto done
$ saved_default = f$environment("DEFAULT")
$ proc = f$environment("PROCEDURE")
$ vmsdir = f$parse(proc,,,"DEVICE") + f$parse(proc,,,"DIRECTORY")
$ set default 'vmsdir'
$ set default [-]
$ arch = f$edit(f$getsyi("ARCH_NAME"),"UPCASE")
$ if arch .eqs. "IA64" .or. arch .eqs. "X86_64" then goto arch_ok
$ write sys$error "BUILD: unsupported architecture ''arch'"
$ goto done
$arch_ok:
$ if f$search("OBJ_''arch'.DIR") .eqs. "" then create/directory [.OBJ_'arch']
$ if f$search("BIN_''arch'.DIR") .eqs. "" then create/directory [.BIN_'arch']
$ target = p1
$ if target .eqs. "" then target = "ALL"
$ write sys$output "BUILD: ''target' for ''arch' in ''f$environment("DEFAULT")'"
$ mmsq = ""
$ if p2 .eqs. "KEEP_GOING" then mmsq = "/IGNORE=ERROR"
$ macros = "ARCH=''arch'"
$ if arch .nes. "X86_64" .or. target .eqs. "CLEAN" then goto mms
$!
$! x86-64: VSI C cannot emit CPUID, so cpuid_x86.c is compiled with VSI's
$! clang (D11) when its object is missing or older than the source.
$ macros = macros + """,""X86=1"
$ cpuobj = "[.OBJ_X86_64]cpuid_x86.obj"
$ cpusrc = "[.src.vms]cpuid_x86.c"
$ if f$search(cpuobj) .nes. ""
$ then
$   if f$cvtime(f$file_attributes(cpusrc,"RDT")) .les. -
       f$cvtime(f$file_attributes(cpuobj,"RDT")) then goto mms
$ endif
$ write sys$output "CLANG ''cpusrc'"
$ clang = "$sys$system:clang.exe"
$ set default [.src.vms]
$ define/user sys$error sys$output
$ clang -c -O2 -o cpuid_x86.obj cpuid_x86.c
$ cstatus = $status
$ set default [--]
$ if .not. cstatus
$ then
$   write sys$output "%CC-E-CLANG, clang failed on ''cpusrc'"
$   status = cstatus
$   goto done
$ endif
$ rename/nolog [.src.vms]cpuid_x86.obj 'cpuobj'
$mms:
$ mms/description=[.vms]descrip.mms/macro=("''macros'")'mmsq' 'target'
$ status = $status
$ if status then write sys$output "BUILD: done"
$done:
$ set default 'saved_default'
$ exit status
