# AR6K Host driver makefile
# Copyright 2006 Atheros Communications, Inc.
# 
# Minimal build invocation:
#   
#     make (all | clean | clobber)  
#
# Extended Make invocation:
#
#     make ATH_BUILD_TYPE=<build type> ATH_SDIO_STACK_BASE=<sdio stack install path> (all | clean | clobber)
#
# Notes:
#     1. This makefile must be invoked from the host/ directory
#     2. The <build type> must match an entry in localmake.linux.inc.
#     3. The localmake.linux.inc can be overridden using an include file outside the build tree.
#        This file (name and path) can be set via the ATH_MAKE_INCLUDE_OVERRIDE variable.
#        ** If ATH_MAKE_INCLUDE_OVERRIDE is used, you can define all build variables in that file 
#        instead of using command line arguments **.  This feature is provided for developers
#        that may want to customize the build using a single include file.
#
#        For example :
#             
#            " make ATH_MAKE_INCLUDE_OVERRIDE=$HOME/mymake.inc "
#        
#             could be used, as long as "mymake.inc" defines all the required variables (see below)
#
#  Required Variables:  
#      
#      ATH_OS_SUB_TYPE - on linux, this must be "linux_2_4" for 2.4 kernels or left blank for 2.6 kernels.
#      ATH_LINUXPATH - linux kernel source path
#      ATH_CROSS_COMPILE_TYPE - optional cross compiler path , leave blank for local gcc compilation
#      ATH_ARCH_CPU_TYPE  - CPU architecture type, leave blank for local gcc compilation
#      ATH_SDIO_STACK_BASE - SDIO Stack installation path to compile the SDIO HIF layer against an externally supplied
#                            SDIO stack source.
#
#  Override variables:
#
#    ATH_MAKE_INCLUDE_OVERRIDE - full path to include file which overrides the default (localmake.linux.inc)
#                                this file can contain other overrides specific to a developers
#                                workspace environment.
#    ATH_BUILD_OUTPUT_OVERRIDE - output path override for compiled executable and
#                                database image
#

#
#history   
# when        who              what                         tag 
# 2010-01-06  hp             for CQ430643 and CQ431832   ZTE_WIFI_HP_007 
#
#
# Include local variables
ifdef ATH_MAKE_INCLUDE_OVERRIDE
_LOCALMAKE_INCLUDE = $(ATH_MAKE_INCLUDE_OVERRIDE)
else
_LOCALMAKE_INCLUDE = localmake.linux.inc
endif

-include $(_LOCALMAKE_INCLUDE)

export  ATH_SRC_BASE
export  ATH_BUILD_TYPE
export  ATH_OS_SUB_TYPE
export  ATH_LINUXPATH
export  ATH_CROSS_COMPILE_TYPE
export  ATH_ARCH_CPU_TYPE
export  ATH_SDIO_STACK_BASE
export  ATH_BUS_TYPE
export  ATH_BUS_SUBTYPE
export  ATH_HC_DRIVERS
export  ATH_HTC_RAW_INT_ENV
# ATHENV
export  ATH_ANDROID_ENV
export  ATH_EEPROM_FILE_USED
export  ATH_FORCE_TCMD_MODE
export  ATH_FORCE_ART_MODE
# ATHENV

#ZTE_WIFI_HP_007 
export ATH_SAVE_EEPROM_TO_FILE 
#ZTE_WIFI_HP_007 end 
ATH_SRC_BASE :=$(shell pwd)
MAKE :=make
CC :=$(ATH_CROSS_COMPILE_TYPE)gcc
LD :=$(ATH_CROSS_COMPILE_TYPE)ld
STRIP :=$(ATH_CROSS_COMPILE_TYPE)strip
#ATH_HIF_TYPE :=`echo $(ATH_BUS_TYPE) | tr [:upper:] [:lower:]`
#ATH_HIF_TYPE := sdio
@ATH_LINUXPATH := ../../kernel/zte-kernel/

export  STRIP
#export compiler variables for 3rd party applications (like the WPA supplicant)
export  CC
export  LD
export  STRIP

# Set cross compile type (if any)
ifdef ATH_CROSS_COMPILE_TYPE
_CROSS_COMPILE_LINE := ARCH=$(ATH_ARCH_CPU_TYPE) CROSS_COMPILE=$(ATH_CROSS_COMPILE_TYPE)
endif

# Figure out module extension
ifneq ($(ATH_OS_SUB_TYPE),linux_2_4)
KMOD_EXTENSION :=ko
endif
ifeq ($(ATH_OS_SUB_TYPE),linux_2_4)
KMOD_EXTENSION :=o
endif

# Set up object output areas
ifdef ATH_BUILD_OUTPUT_OVERRIDE
_COMPILED_OBJECTS_PATH :=$(ATH_BUILD_OUTPUT_OVERRIDE)
COMPILED_BIN_OBJECTS_PATH :=$(ATH_BUILD_OUTPUT_OVERRIDE)
COMPILED_IMAGE_OBJECTS_PATH :=$(ATH_BUILD_OUTPUT_OVERRIDE)
_MAKE_IMAGE_OUTPUT_DIR :=
_MAKE_BIN_OUTPUT_DIR :=
_CLEAN_IMAGE_OUTPUT_DIR :=
_CLEAN_BIN_OUTPUT_DIR :=
else
_COMPILED_OBJECTS_PATH := $(ATH_SRC_BASE)/.output/$(ATH_BUILD_TYPE)-$(ATH_BUS_TYPE)
COMPILED_BIN_OBJECTS_PATH := $(ATH_SRC_BASE)/.output/bin
COMPILED_IMAGE_OBJECTS_PATH :=$(_COMPILED_OBJECTS_PATH)/$(COMPILED_OBJECTS_PREFIX)image
_MAKE_IMAGE_OUTPUT_DIR := mkdir --parents $(COMPILED_IMAGE_OBJECTS_PATH)
_MAKE_BIN_OUTPUT_DIR := mkdir --parents $(COMPILED_BIN_OBJECTS_PATH)
_CLEAN_IMAGE_OUTPUT_DIR := rm -R -f $(COMPILED_IMAGE_OBJECTS_PATH)
_CLEAN_BIN_OUTPUT_DIR := rm -R -f $(COMPILED_BIN_OBJECTS_PATH)
endif

# Export the required variables to other Makefiles in the system
export COMPILED_IMAGE_OBJECTS_PATH
export COMPILED_BIN_OBJECTS_PATH

# control variables for conditionally compiling some tools
ATH_BUILD_SYSTEM_TOOLS ?=no
ATH_BUILD_BTFILTER ?=no

all: modules

modules:
ifndef ATH_BUILD_TYPE 
	@echo "Please edit the $(_LOCALMAKE_INCLUDE) file"
	exit 1
endif
	$(_MAKE_IMAGE_OUTPUT_DIR)
	$(_MAKE_BIN_OUTPUT_DIR)
	$(MAKE)  $(_CROSS_COMPILE_LINE) -C $(ATH_LINUXPATH) ATH_HIF_TYPE=sdio SUBDIRS=$(ATH_SRC_BASE)/os/linux modules
	$(MAKE) -C hif/sdio
	$(STRIP) --strip-unneeded  $(ATH_SRC_BASE)/os/linux/ar6000.$(KMOD_EXTENSION)
	cp -f $(ATH_SRC_BASE)/os/linux/ar6000.$(KMOD_EXTENSION) $(COMPILED_IMAGE_OBJECTS_PATH)
	
clean:
	rm -f os/linux/ar6000.mod.c os/linux/*.o os/linux/*.ko wmi/*.o htc/AR6000/src/*.o htc/*.o \
	bmi/src/*.o wlan/src/*.o driver/* apps/wpa_supplicant/*.o 

# The kernel module build process leaves some intermediate files, this will clean up all these files
	find $(ATH_SRC_BASE) \( -not -path "*.output*" -a -name '*.[oas]' -o -name core -o -name '.*.flags' -o -name '.ko' -o -name '.*.cmd' \) -type f -print \
		| grep -v lxdialog/ | xargs rm -f
	$(MAKE) -C hif/sdio clean

clobber:clean
	rm -rf .output


