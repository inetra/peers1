# -*- makefile -*- Time-stamp: <04/03/16 17:23:52 ptr>
# $Id: rules-install-a.mak,v 1.1.1.1 2007/09/27 13:21:26 alexey Exp $

PHONY += install-release-static install-dbg-static install-stldbg-static

ifneq (windows, $(OSNAME))
install-release-static: release-static $(INSTALL_LIB_DIR)
	$(INSTALL_A) ${A_NAME_OUT} $(INSTALL_LIB_DIR)

install-dbg-static: dbg-static $(INSTALL_LIB_DIR_DBG)
	$(INSTALL_A) ${A_NAME_OUT_DBG} $(INSTALL_LIB_DIR_DBG)

install-stldbg-static: stldbg-static $(INSTALL_LIB_DIR_STLDBG)
	$(INSTALL_A) ${A_NAME_OUT_STLDBG} $(INSTALL_LIB_DIR_STLDBG)
else
install-release-static: release-static $(INSTALL_LIB_DIR)
	$(INSTALL_A) $(subst /,\,$(A_NAME_OUT) $(INSTALL_LIB_DIR)/)

install-dbg-static: dbg-static $(INSTALL_LIB_DIR_DBG)
	$(INSTALL_A) $(subst /,\,$(A_NAME_OUT_DBG) $(INSTALL_LIB_DIR_DBG)/)

install-stldbg-static: stldbg-static $(INSTALL_LIB_DIR_STLDBG)
	$(INSTALL_A) $(subst /,\,$(A_NAME_OUT_STLDBG) $(INSTALL_LIB_DIR_STLDBG)/)
endif
