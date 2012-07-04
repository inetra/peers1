# -*- makefile -*- Time-stamp: <04/05/01 00:34:42 ptr>
# $Id: evc3.mak,v 1.1.1.1 2007/09/27 13:21:27 alexey Exp $

LDFLAGS_COMMON = $(LDFLAGS_COMMON) /base:"0x00100000"

!include evc-common.mak
