$! FASTFETCH$STARTUP.COM - system startup for vms-fastfetch
$!
$! Installed by PCSI into SYS$STARTUP.  Defines the system logical names
$! FASTFETCH$ROOT (the installed [FASTFETCH] directory) and FASTFETCH$PRESETS
$! (its [PRESETS], used by "fastfetch -c <preset>").  To run it at every
$! boot, add this line to SYS$MANAGER:SYSTARTUP_VMS.COM:
$!
$!     $ @SYS$STARTUP:FASTFETCH$STARTUP.COM
$!
$! P1 = "INSTALL": also print the post-installation tasks (PCSI runs it so).
$! P1 = "REMOVE":  deassign the logical names instead (PCSI runs it so at removal).
$!
$! Users then define the command with
$!     $ @FASTFETCH$ROOT:[000000]FASTFETCH$SETUP.COM
$!
$ set noon
$ mode = f$edit(p1, "UPCASE")
$ if mode .eqs. "REMOVE"
$ then
$   if f$trnlnm("FASTFETCH$PRESETS", "LNM$SYSTEM_TABLE") .nes. "" then -
        deassign/system/executive_mode FASTFETCH$PRESETS
$   if f$trnlnm("FASTFETCH$ROOT", "LNM$SYSTEM_TABLE") .nes. "" then -
        deassign/system/executive_mode FASTFETCH$ROOT
$   exit 1
$ endif
$!
$! This procedure sits in <destination>[SYS$STARTUP]; the product is in
$! <destination>[FASTFETCH].  Rooted logicals need the physical form:
$! DKA0:[SYS0.SYSCOMMON.SYS$STARTUP] -> DKA0:[SYS0.SYSCOMMON.FASTFETCH.]
$ proc = f$environment("PROCEDURE")
$ dev = f$parse(proc,,,"DEVICE","NO_CONCEAL")
$ dir = f$edit(f$parse(proc,,,"DIRECTORY","NO_CONCEAL"), "UPCASE") - "]["
$ root = dir - "SYS$STARTUP]" + "FASTFETCH.]"
$ if root .eqs. dir + "FASTFETCH.]"
$ then
$   write sys$error "FASTFETCH$STARTUP: expected to be in a [SYS$STARTUP] directory, not ''dir'"
$   exit 44
$ endif
$ root = root - ".000000"
$ define/system/executive_mode/translation_attributes=concealed FASTFETCH$ROOT 'dev''root'
$ define/system/executive_mode FASTFETCH$PRESETS FASTFETCH$ROOT:[PRESETS]
$ if f$search("FASTFETCH$ROOT:[BIN]FASTFETCH.EXE") .eqs. ""
$ then
$   write sys$error "FASTFETCH$STARTUP: FASTFETCH.EXE not found under ''dev'''root'"
$   exit 44
$ endif
$ if mode .nes. "INSTALL" then exit 1
$ say = "write sys$output"
$ say ""
$ say "    Post-installation tasks for vms-fastfetch"
$ say ""
$ say "    At system startup: to define FASTFETCH$ROOT and FASTFETCH$PRESETS at"
$ say "    every boot, add this line to SYS$MANAGER:SYSTARTUP_VMS.COM:"
$ say "    $ @SYS$STARTUP:FASTFETCH$STARTUP.COM"
$ say "    For each user: to define the FASTFETCH command, add this line to LOGIN.COM:"
$ say "    $ @FASTFETCH$ROOT:[000000]FASTFETCH$SETUP.COM"
$ say ""
$ say "    PRODUCT REMOVE FASTFETCH removes the product and deassigns the logical names."
$ say ""
$ exit 1
