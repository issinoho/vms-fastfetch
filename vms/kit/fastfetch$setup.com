$! FASTFETCH$SETUP.COM - define the FASTFETCH command for a user
$!
$! Add to LOGIN.COM (or SYS$MANAGER:SYLOGIN.COM for everyone):
$!     $ @FASTFETCH$ROOT:[000000]FASTFETCH$SETUP.COM
$!
$! Options are case-insensitive.  Quote format strings to keep their case
$! and spaces, e.g.  fastfetch --os-format "{name} {version}"
$!
$ if f$trnlnm("FASTFETCH$ROOT") .eqs. ""
$ then
$   write sys$error "FASTFETCH$SETUP: FASTFETCH$ROOT is not defined; run FASTFETCH$STARTUP.COM first"
$   exit 44
$ endif
$ fastfetch :== $FASTFETCH$ROOT:[BIN]FASTFETCH.EXE
$ exit 1
