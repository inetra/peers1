# -*- makefile -*- Time-stamp: <04/03/31 08:08:12 ptr>
# $Id: evc4.mak,v 1.1.1.1 2007/09/27 13:21:27 alexey Exp $

LDFLAGS_COMMON = $(LDFLAGS_COMMON) /base:"0x00010000"

!include evc-common.mak
