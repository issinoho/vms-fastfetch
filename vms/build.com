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
$ mms/description=[.vms]descrip.mms/macro=("ARCH=''arch'")'mmsq' 'target'
$ status = $status
$ if status then write sys$output "BUILD: done"
$done:
$ set default 'saved_default'
$ exit status
