# -*- makefile -*- Time-stamp: <03/10/17 14:41:05 ptr>
# $Id: rules-a.mak,v 1.1.1.1 2007/09/27 13:21:27 alexey Exp $

# Shared libraries tags

release-static:	$(OUTPUT_DIR_A) $(A_NAME_OUT)

dbg-static:	$(OUTPUT_DIR_A_DBG) $(A_NAME_OUT_DBG)

stldbg-static:	$(OUTPUT_DIR_A_STLDBG) $(A_NAME_OUT_STLDBG)

$(A_NAME_OUT):	$(OBJ_A) 
	$(AR) $(AR_INS_R) $(AR_OUT) $(OBJ_A)

$(A_NAME_OUT_DBG):	$(OBJ_A_DBG)
	$(AR) $(AR_INS_R) $(AR_OUT) $(OBJ_A_DBG)

$(A_NAME_OUT_STLDBG):	$(OBJ_A_STLDBG)
	$(AR) $(AR_INS_R) $(AR_OUT) $(OBJ_A_STLDBG)

