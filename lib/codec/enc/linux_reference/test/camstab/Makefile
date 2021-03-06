#-------------------------------------------------------------------------------
#-                                                                            --
#-       This software is confidential and proprietary and may be used        --
#-        only as expressly authorized by a licensing agreement from          --
#-                                                                            --
#-                            Hantro Products Oy.                             --
#-                                                                            --
#-                   (C) COPYRIGHT 2006 HANTRO PRODUCTS OY                    --
#-                            ALL RIGHTS RESERVED                             --
#-                                                                            --
#-                 The entire notice above must be reproduced                 --
#-                  on all copies and should not be removed.                  --
#-                                                                            --
#-------------------------------------------------------------------------------
#-
#--  Abstract : Makefile for encoder camera stabilization testbench
#--
#-------------------------------------------------------------------------------

# Comment/uncomment the following line to disable/enable debugging
DEBUG = y

# set this to 'y' for Electric Fence checking
USE_EFENCE = n

# Add your debugging flag (or not) to CFLAGS
ifeq ($(DEBUG),y)
  DEBFLAGS = -O1 -g -DDEBUG
else
  DEBFLAGS = -O2 -DNDEBUG
endif

ifeq ($(shell uname -m),x86_64)
	ifneq (,$(findstring pclinux,$(MAKECMDGOALS)))
		export ARCH = -m32
	endif
	ifneq (,$(findstring system,$(MAKECMDGOALS)))
		export ARCH = -m32
	endif
	ifneq (,$(findstring testdata,$(MAKECMDGOALS)))
		export ARCH = -m32
	endif
	ifneq (,$(findstring eval,$(MAKECMDGOALS)))
		export ARCH = -m32
	endif
endif

# the path where to find header files
INCFLAGS = -I../../../inc -I../../memalloc
INCFLAGS += -I../../../source/camstab

ifeq ($(USE_EFENCE), y)
        EFENCE= -DUSE_EFENCE -I/afs/hantro.com/projects/adder/users/atna/efence_2_4_13 \
                -L/afs/hantro.com/projects/adder/users/atna/efence_2_4_13 \
                -lefence -lpthread
endif

CC = $(CROSS_COMPILE)gcc

# compiler switches
CFLAGS  = $(ARCH) -Wall -D_GNU_SOURCE -D_REENTRANT -D_THREAD_SAFE \
           $(DEBFLAGS) $(INCFLAGS)

# list of used sourcefiles
SRCS = standalone.c EncGetOption.c

vpath %.c
vpath %.c .

OBJS = $(SRCS:.c=.o)

# name of the input library 
LIB = ../../lib8290enc.a

# System model library
MODELLIB = ../../../../system/models/enc8290_asic_model.a

# System model library that writes test data traces
TESTDATALIB = ../../../../system/models/enc8290_asic_model_trace.a

# name of the output executable
TARGET = videostabtest

# MACRO for cleaning object -files
RM  = rm -f


#Here are rules for building codes and generating executable
all:    tags
	@echo ---------------------------------------
	@echo "Usage: make [ system | testdata | eval | versatile ]"
	@echo "system     - PC system model (==pclinux)"
	@echo "testdata   - PC system model for test data creation"
	@echo "eval       - PC system model for evaluation with frame limit"
	@echo "integrator - ARM integrator with FPGA HW"
	@echo "versatile  - ARM versatile with FPGA HW"
	@echo "NOTE! Make sure to do 'make libclean'"
	@echo "between compiling to different targets!"
	@echo ---------------------------------------

# Use other makefiles to build the libraries
$(MODELLIB):
	$(MAKE) -w -C ../../../../system/models

$(TESTDATALIB):
	$(MAKE) -w -C ../../../../system/models testdata

$(LIB):
	$(MAKE) -w -C ../.. $(TARGETENV) INCLUDE_JPEG=n INCLUDE_MPEG4=n \
                INCLUDE_H264=n INCLUDE_TESTING=n USE_EFENCE=$(USE_EFENCE)


.PHONY: eval
evaluation: eval
eval: TARGET = vs_8290_eval
eval: TARGETENV = eval
eval: $(MODELLIB) $(LIB) $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LIB) $(MODELLIB) -o $(TARGET)

pclinux: system

.PHONY: system
system: TARGETENV = system
system: $(MODELLIB) $(LIB) $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LIB) $(MODELLIB) -o $(TARGET)

.PHONY: testdata
testdata: TARGETENV = testdata
testdata: $(TESTDATALIB) $(LIB) $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LIB) $(TESTDATALIB) -o $(TARGET)

.PHONY: versatile
versatile: TARGETENV = versatile
versatile: CROSS_COMPILE = arm-none-linux-gnueabi-
versatile: ARCH = -mcpu=arm926ej-s -mtune=arm926ej-s
versatile: CFLAGS+= -DMEMALLOC_MODULE_PATH=\"/tmp/dev/memalloc\"
versatile: LIB += -lpthread
versatile: $(OBJS)
	$(MAKE) -w -C ../.. $@ INCLUDE_JPEG=n INCLUDE_MPEG4=n INCLUDE_H264=n \
            INCLUDE_TESTING=n USE_EFENCE=$(USE_EFENCE) \
            CROSS_COMPILE=$(CROSS_COMPILE) ARCH="$(ARCH)"
	$(CC) $(CFLAGS) $(OBJS) $(LIB) $(EFENCE) -o $(TARGET) 

.PHONY: integrator
integrator: TARGETENV = integrator
integrator: CROSS_COMPILE= arm-linux-
integrator: ARCH = -mcpu=arm9tdmi -mtune=arm9tdmi
integrator: CFLAGS+= -DSDRAM_LM_BASE=0x80000000 
integrator: CFLAGS+= -DMEMALLOC_MODULE_PATH=\"/dev/memalloc\"
integrator: LIB += -lpthread
integrator: $(OBJS)
	$(MAKE) -w -C ../.. $@ INCLUDE_JPEG=n INCLUDE_MPEG4=n INCLUDE_H264=n \
            INCLUDE_TESTING=n USE_EFENCE=$(USE_EFENCE) \
            CROSS_COMPILE=$(CROSS_COMPILE) ARCH="$(ARCH)"
	$(CC) $(CFLAGS) $(OBJS) $(LIB) $(EFENCE) -o $(TARGET)

.PHONY: pci
pci: LIB += -lpthread
pci: $(OBJS)
	$(MAKE) -w -C ../.. $@ INCLUDE_JPEG=n INCLUDE_MPEG4=n INCLUDE_H264=n \
            INCLUDE_TESTING=n USE_EFENCE=$(USE_EFENCE) \
            CROSS_COMPILE=$(CROSS_COMPILE) ARCH="$(ARCH)"
	$(CC) $(CFLAGS) $(OBJS) $(LIB) $(EFENCE) -o $(TARGET)

system_cov: CC = covc --retain -t!standalone.c,!EncGetOption.c g++
system_cov: TARGETENV = system_cov
system_cov: $(MODELLIB) $(LIB) $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LIB) $(MODELLIB) -o $(TARGET)

.PHONY: clean
clean:
	$(RM) *.o core* *~ $(TARGET) tags

.PHONY: libclean
libclean: clean
	$(MAKE) -w -C ../.. clean

.PHONY: tags
tags:
	ctags ../../../inc/*.h *.c ../../../source/camstab/*.[ch] \
            ../../../source/common/*.[ch] ../../ewl/*c

