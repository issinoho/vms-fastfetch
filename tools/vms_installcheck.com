$! VMS_INSTALLCHECK.COM - install the FASTFETCH kit, verify, smoke-test the
$! installed image, then remove it.  Changes the system while it runs (PCSI
$! database, SYS$COMMON:[FASTFETCH], SYS$STARTUP:FASTFETCH$STARTUP.COM, system
$! logicals FASTFETCH$ROOT and FASTFETCH$PRESETS); leaves it as it was.
$! Run from the work directory (tools/installcheck.sh does).
$ set noon
$ arch = f$edit(f$getsyi("ARCH_NAME"), "UPCASE")
$ base = "I64VMS"
$ if arch .eqs. "X86_64" then base = "X86VMS"
$ top = f$environment("DEFAULT")
$ kitdir = top - "]" + ".KIT_''arch']"
$ write sys$output "=== INSTALL from ", kitdir
$ product install FASTFETCH /producer=ISSINOHO /base_system='base' /source='kitdir' /options=noconfirm /log
$ write sys$output "=== install status ", $status
$ product show product FASTFETCH /producer=ISSINOHO
$ write sys$output "=== VERIFY"
$ write sys$output "startup procedure: [", f$search("SYS$STARTUP:FASTFETCH$STARTUP.COM"), "]"
$ show logical FASTFETCH$ROOT
$ show logical FASTFETCH$PRESETS
$ directory/nohead/notrail FASTFETCH$ROOT:[000000...]*.*
$ @FASTFETCH$ROOT:[000000]FASTFETCH$SETUP.COM
$ show symbol fastfetch
$ fastfetch --version
$ write sys$output "=== SMOKE TEST on installed image"
$ @[.VMS]TEST_SMOKE.COM
$ write sys$output "=== REMOVE"
$ product remove FASTFETCH /producer=ISSINOHO /options=noconfirm /log
$ write sys$output "=== remove status ", $status
$ write sys$output "FASTFETCH$ROOT after removal: [", f$trnlnm("FASTFETCH$ROOT"), "]"
$ write sys$output "FASTFETCH$PRESETS after removal: [", f$trnlnm("FASTFETCH$PRESETS"), "]"
$ write sys$output "files after removal: [", f$search("SYS$COMMON:[FASTFETCH...]*.*"), "]"
$ write sys$output "startup after removal: [", f$search("SYS$STARTUP:FASTFETCH$STARTUP.COM"), "]"
$ product show product FASTFETCH /producer=ISSINOHO
$ if f$type(fastfetch) .nes. "" then delete/symbol/global fastfetch
$ exit 1
