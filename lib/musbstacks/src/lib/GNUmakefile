############################################################################
#
#             Copyright Mentor Graphics Corporation 2004
#                All Rights Reserved.
#
#    THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION
#  WHICH IS THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS
#  LICENSORS AND IS SUBJECT TO LICENSE TERMS.
#
# Build the Portable USB Software Platform
#
# $Revision: 1.1 $
#############################################################################

include $(MUSB_DIR)/config.mak

LIBC_SRC_DIR = $(MUSB_DIR)/src/lib
OS_SRC_DIR = $(MUSB_DIR)/src/systems/$(TARGET_OS)
SYSTEM_SRC_DIR = $(MUSB_DIR)/src/systems/$(TARGET_OS)
BOARD_SRC_DIR = $(SYSTEM_SRC_DIR)/$(TARGET_TYPE)

# can add to SYSTEM_OBJ, CFLAGS, LDFLAGS, LDFLAGS_TAIL, etc.
include $(SYSTEM_SRC_DIR)/os.mak

###############################################################################

LIBC_OBJ = \
  $(OS_OBJ_DIR)/mu_bits$(OBJ) \
  $(OS_OBJ_DIR)/mu_ramfl$(OBJ) \
  $(OS_OBJ_DIR)/mu_stack$(OBJ) \
  $(OS_OBJ_DIR)/mu_stdio$(OBJ) \
  $(OS_OBJ_DIR)/mu_strng$(OBJ) \

###############################################################################
# Rules
###############################################################################

$(OS_OBJ_DIR)/%$(OBJ): $(LIBC_SRC_DIR)/%.c
	$(CREATE_FILE_DIR)
	$(C2OBJ) $(CINCLUDE)$(MUSB_DIR)/include $(CINCLUDE)$(SYSTEM_SRC_DIR) $(CINCLUDE)$(BOARD_SRC_DIR) $(CINCLUDE)$(TARGET_OS) $(CINCLUDE)$(LIBC_SRC_DIR) $(C2OBJ_TAIL)

###############################################################################
# Targets
###############################################################################

.PHONY: all

all: libc

.PHONY: libc

libc: $(OS_BIN_DIR)/mu_libc$(LIB)

$(OS_BIN_DIR)/mu_libc$(LIB): $(LIBC_OBJ)
	$(CREATE_FILE_DIR)
	$(OBJS2LIB) $(LIBC_OBJ)

#############################################################################
# Cleaning targets
###############################################################################
.PHONY: clean

clean: 
	$(DELETE_FILE) $(LIBC_OBJ) $(OS_BIN_DIR)/mu_libc$(LIB)

.PHONY: distclean

distclean: clean

###############################################################################
# Dependencies
###############################################################################

#include $(LIBC_SRC_DIR)/libc.mak
