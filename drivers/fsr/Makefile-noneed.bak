#
#   Makefile for eXtended Sector Remapper (FSR) driver
#
#   Copyright(c) 2004-2009, Samsung Electronics, Co., Ltd.
#

EXTRA_CFLAGS	+= -I$(srctree)/drivers/fsr/Inc \
		   -I$(srctree)/drivers/fsr

# For FSR 1.5 RC5 and below version
EXTRA_CFLAGS	+= -DFSR_DUP_BUFFER

EXTRA_CFLAGS	+= -DFSR_LINUX_OAM
#EXTRA_CFLAGS	+= -DFSR_OAM_RTLMSG_DISABLE
EXTRA_CFLAGS	+= -DFSR_OAM_DBGMSG_ENABLE #-DFSR_OAM_ALL_DBGMSG 
#EXTRA_CFLAGS	+= -DFSR_ASSERT
#EXTRA_CFLAGS	+= -DFSR_MAMMOTH_POWEROFF
EXTRA_CFLAGS	+= -DFSR_BML_DEVICE_AUTHENTICATION
EXTRA_CFLAGS	+= -DFSR_BML_SUPPORT_SUSPEND_RESUME

ifeq ($(CONFIG_TINY_FSR),y)
EXTRA_CFLAGS	+= -DWITH_TINY_FSR
endif

ifeq ($(CONFIG_ARM),y)
EXTRA_CFLAGS    += -D__CC_ARM
endif

ifeq ($(CONFIG_LINUSTOREIII_DEBUG),y)
#EXTRA_CFLAGS	+= -D_RFS_INTERNAL_RESET
#EXTRA_CFLAGS	+= -D_RFS_INTERNAL_STAT_BH
endif

# Note: The following options are only used for development purpose
#	We don't guarantee these options on production
EXTRA_CFLAGS	+= -D__LINUSTORE_INTERNAL_DEBUG_SYNC__
EXTRA_CFLAGS	+= -D__LINUSTORE_INTERNAL_IO_PARTTERN__
EXTRA_CFLAGS	+= -D__LINUSTOREIII_INTERNAL_PM__
EXTRA_CFLAGS	+= -D__LINUSTORE_INTERNAL_PLATFORM_DRIVER__

# Kernel gcov
ifeq ($(CONFIG_GCOV_PROFILE),y)
ifeq ($(PATCHLEVEL),4)
include Makefile.gcov
else
include $(srctree)/drivers/fsr/Makefile.gcov
endif
endif
ifeq ($(PATCHLEVEL),4)
include Makefile.24
else
include $(srctree)/drivers/fsr/Makefile.26
endif
