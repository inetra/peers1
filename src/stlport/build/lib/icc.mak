# -*- Makefile -*- Time-stamp: <03/10/12 20:35:49 ptr>
# $Id: icc.mak,v 1.1.1.1 2007/09/27 13:21:26 alexey Exp $

SRCROOT := ..
COMPILER_NAME := icc

STLPORT_INCLUDE_DIR = ../../stlport
include Makefile.inc
include ${SRCROOT}/Makefiles/top.mak


INCLUDES += -I$(STLPORT_INCLUDE_DIR)

ifeq ($(OSNAME),linux)
DEFS += -D_STLP_REAL_LOCALE_IMPLEMENTED -D_GNU_SOURCE
endif

# options for build with boost support
ifdef STLP_BUILD_BOOST_PATH
INCLUDES += -I$(STLP_BUILD_BOOST_PATH)
endif

