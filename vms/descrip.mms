! DESCRIP.MMS - build vms-fastfetch (IA64, x86-64)
!
! Run from the top of the tree via [.VMS]BUILD.COM, which passes ARCH
! (IA64 or X86_64) and creates the output directories.  The object list and
! per-object rules come from [.VMS]SOURCES.MMS (tools/gen_mms.py).

.IFDEF ARCH
.ELSE
ARCH = IA64
.ENDIF

OBJ = [.OBJ_$(ARCH)]
BIN = [.BIN_$(ARCH)]

.INCLUDE [.VMS]SOURCES.MMS

CC = CC
! Default /STANDARD (relaxed): C99 features plus the "$" in SYI$_ names.
CFLAGS = $(CC_QUAL)/NOLIST/INCLUDE_DIRECTORY=("./src")

ALL : $(BIN)FASTFETCH.EXE
	@ CONTINUE

! "-": LINK ends with a warning status (%ILINK-W-COMPWARN) when a module
! compiled with warnings; tools/build.sh fails the build on real errors.
$(BIN)FASTFETCH.EXE : $(OBJS)
	- LINK/EXECUTABLE=$(MMS$TARGET)/MAP=$(OBJ)FASTFETCH.MAP $(OBJS)

CLEAN :
	IF F$SEARCH("$(OBJ)*.*") .NES. "" THEN DELETE/NOLOG $(OBJ)*.*;*
	IF F$SEARCH("$(BIN)*.*") .NES. "" THEN DELETE/NOLOG $(BIN)*.*;*
