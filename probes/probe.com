$! PROBE.COM - Phase 0 probes for vms-fastfetch.
$!
$! Usage:  @[.PROBES]PROBE
$!
$! Compiles and runs each probe_*.c, compiles yyjson, and records the system
$! facts the detection code will be checked against.  Every step runs with
$! NOON so one failure does not hide the rest; sections start "=== ".
$!
$ set noon
$ saved_default = f$environment("DEFAULT")
$ proc = f$environment("PROCEDURE")
$ here = f$parse(proc,,,"DEVICE") + f$parse(proc,,,"DIRECTORY")
$ set default 'here'
$ arch = f$edit(f$getsyi("ARCH_NAME"),"UPCASE")
$ obj = "[.OBJ_''arch']"
$ if f$search("OBJ_''arch'.DIR") .eqs. "" then create/directory 'obj'
$ open/read cf [-.vms.config]ccflags.txt
$ read cf ccflags
$ close cf
$ ccflags = f$edit(ccflags,"TRIM")
$ yyinc = "/INCLUDE_DIRECTORY=([-.src.3rdparty.yyjson])"
$!
$ write sys$output "=== system"
$ write sys$output "node    ", f$getsyi("NODENAME"), " arch ", arch, " version ", f$getsyi("VERSION")
$ write sys$output "hw_name ", f$getsyi("HW_NAME")
$ write sys$output "boot    ", f$getsyi("BOOTTIME"), "  now ", f$time()
$ cc/version nl:
$ tcpip show version
$ show cpu
$ show memory/physical/files
$ show device d/mounted
$ show logical/full sys$sysdevice
$ show logical pcsi*
$!
$ write sys$output "=== pcsi"
$ t0 = f$time()
$ c0 = f$getjpi("","CPUTIM")
$ define/user sys$output 'obj'pcsi.lis
$ product show product *
$ write sys$output "PRODUCT SHOW PRODUCT: start ", t0, " end ", f$time(), " cpu(10ms) ", f$getjpi("","CPUTIM") - c0
$ type 'obj'pcsi.lis
$ t0 = f$time()
$ n = 0
$pcsi_loop:
$ f = f$search("sys$common:[sysexe]*.pcsi$database",1)
$ if f .eqs. "" then goto pcsi_end
$ n = n + 1
$ goto pcsi_loop
$pcsi_end:
$ write sys$output "PCSI database files: ''n' (search took ", t0, " -> ", f$time(), ")"
$!
$ write sys$output "=== headers"
$ library/extract=SYIDEF/output='obj'syidef.h sys$library:sys$starlet_c.tlb
$ search/nohead 'obj'syidef.h "FREE","CPU","FREQ","HYPER","SPEED","HZ","SERIAL","_NAME","PAGES","VIRT","MODEL","PROCESSOR"
$ library/extract=RMIDEF/output='obj'rmidef.h sys$library:sys$starlet_c.tlb
$ search/nohead 'obj'rmidef.h "#define RMI$_"
$ show cpu/full
$ library/extract=BUILTINS/output='obj'builtins.h sys$library:decc$rtldef.tlb
$ search/nohead 'obj'builtins.h "CPUID","RDTSC","CPU_ID","__asm"
$!
$ write sys$output "=== licenses"
$ show license/brief
$!
$ call probe probe_c99 "/STANDARD=C99"
$ call probe probe_syi ""
$ call probe probe_jpi ""
$ call probe probe_dvi ""
$ call probe probe_rmi ""
$ call probe probe_net ""
$ call probe probe_net_bsd "/DEFINE=(_SOCKADDR_LEN)" probe_net
$ call probe probe_ifaddrs ""
$ call probe probe_out ""
$!
$ write sys$output "=== yyjson"
$ write sys$output "--- compile yyjson.c /STANDARD=C99"
$ cc 'ccflags'/STANDARD=C99/OBJECT='obj'yyjson.obj [-.src.3rdparty.yyjson]yyjson.c
$ write sys$output "--- status ", $status
$ write sys$output "--- compile yyjson.c (default /STANDARD)"
$ cc 'ccflags'/OBJECT='obj'yyjson_rel.obj [-.src.3rdparty.yyjson]yyjson.c
$ write sys$output "--- status ", $status
$ write sys$output "--- compile probe_json.c"
$ cc 'ccflags'/STANDARD=C99'yyinc'/OBJECT='obj'probe_json.obj probe_json.c
$ write sys$output "--- status ", $status
$ link/executable='obj'probe_json.exe 'obj'probe_json.obj,'obj'yyjson.obj
$ write sys$output "--- link status ", $status
$ if f$search("''obj'probe_json.exe") .nes. "" then run 'obj'probe_json.exe
$!
$ write sys$output "PROBE: done"
$ set default 'saved_default'
$ exit
$!
$! CALL PROBE name extra-cc-qualifiers [source-name]
$probe: subroutine
$ set noon
$ src = p3
$ if src .eqs. "" then src = p1
$ write sys$output "=== ''p1'"
$ cc 'ccflags''p2'/OBJECT='obj''p1'.obj 'src'.c
$ st = $status
$ write sys$output "--- compile status ", st
$ if .not. st then exit 1
$ link/executable='obj''p1'.exe 'obj''p1'.obj
$ st = $status
$ write sys$output "--- link status ", st
$ if .not. st then exit 1
$ run 'obj''p1'.exe
$ write sys$output "--- run status ", $status
$ exit 1
$ endsubroutine
