# Time-stamp: <03/10/26 16:42:14 ptr>
# $Id: rules-install-so.mak,v 1.1.1.1 2007/09/27 13:21:27 alexey Exp $

!ifndef INSTALL_TAGS
INSTALL_TAGS= install-shared install-static
!endif

install:	$(INSTALL_TAGS)

install-release-shared: release-shared $(INSTALL_BIN_DIR)
	$(INSTALL_SO) $(PRG) $(INSTALL_BIN_DIR)
	$(INSTALL_SO) $(PDB_NAME_OUT) $(INSTALL_BIN_DIR)
	@if exist $(MANIFEST_NAME_OUT) $(INSTALL_SO) $(MANIFEST_NAME_OUT) $(INSTALL_BIN_DIR)

install-dbg-shared: dbg-shared $(INSTALL_BIN_DIR_DBG)
	$(INSTALL_SO) $(PRG_DBG) $(INSTALL_BIN_DIR_DBG)
	$(INSTALL_SO) $(PDB_NAME_OUT_DBG) $(INSTALL_BIN_DIR_DBG)
	@if exist $(MANIFEST_NAME_OUT_DBG) $(INSTALL_SO) $(MANIFEST_NAME_OUT_DBG) $(INSTALL_BIN_DIR_DBG)

install-stldbg-shared: stldbg-shared $(INSTALL_BIN_DIR_STLDBG)
	$(INSTALL_SO) $(PRG_STLDBG) $(INSTALL_BIN_DIR_STLDBG)
	$(INSTALL_SO) $(PDB_NAME_OUT_STLDBG) $(INSTALL_BIN_DIR_STLDBG)
	@if exist $(MANIFEST_NAME_OUT_STLDBG) $(INSTALL_SO) $(MANIFEST_NAME_OUT_STLDBG) $(INSTALL_BIN_DIR_STLDBG)
