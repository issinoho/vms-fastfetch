$! MAKE_KIT.COM - build the PCSI kit for this node's architecture
$!
$! Usage:  @[.VMS.KIT]MAKE_KIT
$! Needs a built tree (@[.VMS]BUILD): FASTFETCH.EXE in [.BIN_<arch>].
$! Fills in the product description and text module templates from
$! [.VMS.KIT]KIT.ENV, gathers the material in [.KIT_<arch>.MAT] and writes
$! the kit to [.KIT_<arch>].
$!
$ set noon
$ status = 44
$ saved_default = f$environment("DEFAULT")
$ proc = f$environment("PROCEDURE")
$ kitdir = f$parse(proc,,,"DEVICE") + f$parse(proc,,,"DIRECTORY")
$ set default 'kitdir'
$ set default [--]
$ arch = f$edit(f$getsyi("ARCH_NAME"), "UPCASE")
$ base = "I64VMS"
$ if arch .eqs. "X86_64" then base = "X86VMS"
$!
$! KIT_PRODUCER, PCSI_VERSION, KIT_VERSION from kit.env
$ open/read env [.VMS.KIT]KIT.ENV
$env_loop:
$ read/end=env_done env line
$ name = f$element(0, "=", line)
$ 'name' = f$element(1, "=", line)
$ goto env_loop
$env_done:
$ close env
$!
$ if f$search("[.BIN_''arch']FASTFETCH.EXE") .eqs. ""
$ then
$   write sys$error "MAKE_KIT: no [.BIN_''arch']FASTFETCH.EXE; build first"
$   goto done
$ endif
$!
$! The PDF and text module, named as PRODUCT PACKAGE expects them.
$ pdf = "[.VMS.KIT]FASTFETCH-''base'"
$ call subst [.VMS.KIT]FASTFETCH.PCSI$DESC_TEMPLATE 'pdf'.PCSI$DESC
$ call subst [.VMS.KIT]FASTFETCH.PCSI$TEXT_TEMPLATE 'pdf'.PCSI$TEXT
$!
$! Gather the files flat in [.KIT_<arch>.MAT]: PRODUCT PACKAGE looks each
$! one up by name in the material directory (destinations are in the PDF).
$ mat = "[.KIT_''arch'.MAT]"
$ out = "[.KIT_''arch']"
$ if f$search("KIT_''arch'.DIR") .eqs. "" then create/directory 'out'
$ if f$search("[.KIT_''arch']MAT.DIR") .eqs. "" then create/directory 'mat'
$ if f$search("''out'*.PCSI;*") .nes. "" then delete/nolog 'out'*.PCSI;*
$ if f$search("''mat'*.*;*") .nes. "" then delete/nolog 'mat'*.*;*
$ copy/nolog [.BIN_'arch']FASTFETCH.EXE 'mat'
$ copy/nolog [.VMS.KIT]FASTFETCH$STARTUP.COM,FASTFETCH$SETUP.COM 'mat'
$ copy/nolog [.VMS.KIT]README.VMS 'mat'README.VMS
$ copy/nolog [.PRESETS]all.jsonc 'mat'ALL.JSONC
$ copy/nolog [.PRESETS]ci.jsonc 'mat'CI.JSONC
$ copy/nolog [.DOCS]CONFIG.MD 'mat'CONFIG.MD
$ copy/nolog []LICENSE. 'mat'LICENSE.
$ copy/nolog []NOTICE. 'mat'NOTICE.
$ copy/nolog [.SRC.3RDPARTY.YYJSON]LICENSE. 'mat'YYJSON-LICENSE.
$ matspec = f$parse(mat,,,"DEVICE","NO_CONCEAL") + f$parse(mat,,,"DIRECTORY","NO_CONCEAL")
$!
$ write sys$output "MAKE_KIT: ''KIT_PRODUCER' ''base' FASTFETCH ''PCSI_VERSION' (vms-fastfetch ''KIT_VERSION')"
$ product package FASTFETCH -
    /producer='KIT_PRODUCER' /base_system='base' /version='PCSI_VERSION' -
    /source='pdf'.PCSI$DESC -
    /material='matspec' -
    /destination='out' -
    /format=sequential -
    /options=noconfirm /log
$ status = $status
$ kit = f$search("''out'*.PCSI")
$ if kit .nes. "" then write sys$output "MAKE_KIT: kit ", kit
$ if kit .eqs. "" then status = 44
$done:
$ set default 'saved_default'
$ exit status
$!
$! CALL SUBST template output: copy template to output, replacing
$! @PRODUCER@, @BASE@, @PCSIVERSION@ and @VERSION@.
$subst: subroutine
$ set noon
$ open/read tin 'p1'
$ open/write tout 'p2'
$sloop:
$ read/end=sdone tin line
$ i = 0
$tloop:
$ tok = f$element(i, ",", "@PRODUCER@,@BASE@,@PCSIVERSION@,@VERSION@")
$ if tok .eqs. "," then goto twrite
$ val = KIT_PRODUCER
$ if tok .eqs. "@BASE@" then val = base
$ if tok .eqs. "@PCSIVERSION@" then val = PCSI_VERSION
$ if tok .eqs. "@VERSION@" then val = KIT_VERSION
$rloop:
$ pos = f$locate(tok, line)
$ if pos .eq. f$length(line) then goto rdone
$ line = f$extract(0, pos, line) + val + f$extract(pos + f$length(tok), f$length(line), line)
$ goto rloop
$rdone:
$ i = i + 1
$ goto tloop
$twrite:
$ write tout line
$ goto sloop
$sdone:
$ close tin
$ close tout
$ exit 1
$ endsubroutine
