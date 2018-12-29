############################################################################
#
#             Copyright Mentor Graphics Corporation 2004
#                All Rights Reserved.
#
#    THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION
#  WHICH IS THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS
#  LICENSORS AND IS SUBJECT TO LICENSE TERMS.
#
# Build example peripherals for a given environment
#
# $Revision: 1.1 $
#############################################################################

include $(MUSB_DIR)/config.mak

###############################################################################
# Variables
###############################################################################
FUNCTIONS_BIN_DIR = $(OS_BIN_DIR)
FUNCTIONS_OBJ_DIR = $(OS_OBJ_DIR)
FUNCTIONS_SRC_DIR = $(MUSB_DIR)/src/functions

SYSTEMS_SRC_DIR = $(MUSB_DIR)/src/systems
OS_SRC_DIR = $(SYSTEMS_SRC_DIR)/$(TARGET_OS)
BOARD_SRC_DIR = $(OS_SRC_DIR)/$(TARGET_TYPE)
FUNCTION_CINCLUDE = $(CINCLUDE)$(MUSB_DIR)/include $(CINCLUDE)$(OS_SRC_DIR) $(CINCLUDE)$(BOARD_SRC_DIR)

###############################################################################
# needs to add anything necessary to A2OBJ, C2OBJ, OBJS2EXE, etc.
include $(OS_SRC_DIR)/$(TARGET_TYPE)/$(TARGET_OS).mak
###############################################################################

#
# TODO: add other functions names and dirs
#

TRANS_FN_NAME = $(FUNCTION_PREFIX)trans
TRANS_FN_BIN_DIR = $(FUNCTIONS_BIN_DIR)
TRANS_FN_TARGET = $(TRANS_FN_BIN_DIR)/$(TRANS_FN_NAME)$(LIB)
TRANS_FN_OBJ_DIR = $(FUNCTIONS_OBJ_DIR)/$(TRANS_FN_NAME)
TRANS_FN_SRC_DIR = $(FUNCTIONS_SRC_DIR)/trans_fn


TRANS_FN_OBJ = \
	$(TRANS_FN_OBJ_DIR)/mu_tfunc$(OBJ)

###############################################################################
# Rules
###############################################################################

#
# TODO: add other driver rules
#

$(TRANS_FN_OBJ_DIR)/%$(OBJ): $(TRANS_FN_SRC_DIR)/%.c
	$(CREATE_FILE_DIR)
	$(C2OBJ) $(FUNCTION_CINCLUDE) $(CINCLUDE)$(TRANS_FN_SRC_DIR) $(C2OBJ_TAIL)

###############################################################################
# Targets
###############################################################################

.PHONY: all

#
# TODO: add other driver targets
#
all: trans_fn  

.PHONY: trans_fn

trans_fn: $(TRANS_FN_TARGET)

$(TRANS_FN_TARGET): $(TRANS_FN_OBJ)
	$(CREATE_FILE_DIR)
	$(OBJS2LIB) $(TRANS_FN_OBJ)



#############################################################################
# Cleaning targets
###############################################################################
.PHONY: clean
clean:
	$(DELETE_FILE) $(TRANS_FN_TARGET)
	$(DELETE_FILE) $(TRANS_FN_OBJ)

.PHONY: distclean
distclean: clean
	$(DELETE_DIR)  $(TRANS_FN_OBJ_DIR)
	
###############################################################################
# Dependencies
###############################################################################

