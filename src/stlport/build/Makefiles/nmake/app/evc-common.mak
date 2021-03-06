# -*- makefile -*- Time-stamp: <04/05/01 00:46:25 ptr>
# $Id: evc-common.mak,v 1.1.1.1 2007/09/27 13:21:27 alexey Exp $

# missing defines in this file: LDFLAGS_COMMON

# For CE, the linker by default uses WinMain() as entry point, using this we make it use the standard main()
LDFLAGS_COMMON = $(LDFLAGS_COMMON) /entry:"mainACRTStartup"

!ifndef LDLIBS
LDLIBS =
!endif

LDFLAGS_REL = $(LDFLAGS_REL) $(LDFLAGS_COMMON) $(LDSEARCH)
LDFLAGS_DBG = $(LDFLAGS_DBG) $(LDFLAGS_COMMON) $(LDSEARCH)
LDFLAGS_STLDBG = $(LDFLAGS_STLDBG) $(LDFLAGS_COMMON) $(LDSEARCH)
