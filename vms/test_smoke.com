$! TEST_SMOKE.COM - smoke test of an installed or built vms-fastfetch, in DCL.
$!
$! Usage:  @[.VMS]TEST_SMOKE [image]
$!         image defaults to FASTFETCH$ROOT:[BIN]FASTFETCH.EXE (the installed
$!         one).  Ends with "SMOKE: N passed, M failed".  The host-side
$!         tools/test.sh is the full regression suite; this runs on VMS alone,
$!         for the kit's install check.
$!
$ set noon
$ image = p1
$ if image .eqs. "" then image = "FASTFETCH$ROOT:[BIN]FASTFETCH.EXE"
$ ff = "$" + f$parse(image)
$ tmp = "SYS$SCRATCH:FASTFETCH_SMOKE_" + f$getjpi("","PID") + ".TMP"
$! Global: the CHECK subroutine updates them.
$ pass == 0
$ fail == 0
$!
$ call check "default banner"   ""                       1 "OS: OpenVMS"
$ call check "version"          "--version"              1 "vms-fastfetch"
$ call check "preset ci"        "-c ci"                  1 "Version: vms-fastfetch"
$ call check "preset all"       "-c all -l none"         1 "Date & Time: "
$ call check "json"             "-j -s os:cpu"           1 "prettyName"
$ call check "structure"        "-s cpu:memory -l none"  1 "Memory: "
$ call check "format"           "-s os -l none --os-format ""[{name}]""" 1 "OS: [OpenVMS]"
$ call check "bad option fails" "--no-such-option"       0 "unknown option"
$!
$ write sys$output "SMOKE: ''pass' passed, ''fail' failed"
$ delete/symbol/global pass
$ delete/symbol/global fail
$ exit 1
$!
$! CALL CHECK name args want-success expected-text
$check: subroutine
$ set noon
$ define/user sys$output 'tmp'
$ define/user sys$error 'tmp'
$ ff 'p2'
$ st = $status
$ ok = (st .and. 1) .eq. p3
$ if ok
$ then
$   search/nooutput 'tmp' "''p4'"
$!  SEARCH: success when matched, informational NOMATCHES otherwise.
$   ok = ($status .and. 7) .eq. 1
$ endif
$ if ok
$ then
$   pass == pass + 1
$   write sys$output "PASS: ", p1
$ else
$   fail == fail + 1
$   write sys$output "FAIL: ", p1, " (status ", st, ")"
$   type 'tmp'
$ endif
$ if f$search(tmp) .nes. "" then delete/nolog 'tmp';*
$ exit 1
$ endsubroutine
