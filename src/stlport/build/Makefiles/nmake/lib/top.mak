# -*- makefile -*- Time-stamp: <03/10/26 16:04:46 ptr>
# $Id: top.mak,v 1.1.1.1 2007/09/27 13:21:27 alexey Exp $

!ifndef LDFLAGS
LDFLAGS = 
!endif

!include $(RULESBASE)/$(USE_MAKE)/lib/macro.mak
!include $(RULESBASE)/$(USE_MAKE)/lib/$(COMPILER_NAME).mak
!include $(RULESBASE)/$(USE_MAKE)/lib/rules-so.mak
!include $(RULESBASE)/$(USE_MAKE)/lib/rules-a.mak
!include $(RULESBASE)/$(USE_MAKE)/lib/rules-install-so.mak
!include $(RULESBASE)/$(USE_MAKE)/lib/rules-install-a.mak
!include $(RULESBASE)/$(USE_MAKE)/lib/clean.mak
