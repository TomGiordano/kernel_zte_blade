#Android makefile to build kernel as a part of Android Build
#zenghuipeng	add oprofile.ko driver.  ZHP_OPROFILE_20101109
#ouyanghuiqin copy dhd.ko for compile problem. ZTE_WIFI_OYHQ_20110106
ifeq ($(TARGET_PREBUILT_KERNEL),)

KERNEL_OUT := $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ
KERNEL_CONFIG := $(KERNEL_OUT)/.config
TARGET_PREBUILT_INT_KERNEL := $(KERNEL_OUT)/arch/arm/boot/zImage
ifeq ($(TARGET_USES_UNCOMPRESSED_KERNEL),true)
$(info Using uncompressed kernel)
TARGET_PREBUILT_KERNEL := $(KERNEL_OUT)/piggy
else
TARGET_PREBUILT_KERNEL := $(TARGET_PREBUILT_INT_KERNEL)
endif

$(KERNEL_OUT):
	mkdir -p $(KERNEL_OUT)

$(KERNEL_CONFIG): $(KERNEL_OUT)
	$(MAKE) -C kernel O=../$(KERNEL_OUT) ARCH=arm CROSS_COMPILE=arm-eabi- $(KERNEL_DEFCONFIG)

$(KERNEL_OUT)/piggy : $(TARGET_PREBUILT_INT_KERNEL)
	$(hide) gunzip -c $(KERNEL_OUT)/arch/arm/boot/compressed/piggy > $(KERNEL_OUT)/piggy

$(TARGET_PREBUILT_INT_KERNEL): $(KERNEL_OUT) $(KERNEL_CONFIG)
	$(MAKE) -C kernel O=../$(KERNEL_OUT) ARCH=arm CROSS_COMPILE=arm-eabi-
	$(MAKE) -C kernel O=../$(KERNEL_OUT) ARCH=arm CROSS_COMPILE=arm-eabi- modules
	#ZHP_OPROFILE_20101109 add start
	mkdir -p ./$(KERNEL_OUT)/../../system/lib
	cp -f ./$(KERNEL_OUT)/arch/arm/oprofile/oprofile.ko ./$(KERNEL_OUT)/../../system/lib
	
	#ZTE_WIFI_OYHQ_20110106
	#$(shell if [ -e $(./$(KERNEL_OUT)/drivers/net/wireless/bcm4319/dhd.ko) ]; then cp -f ./$(KERNEL_OUT)/drivers/net/wireless/bcm4319/dhd.ko ./$(KERNEL_OUT)/../../system/lib;  fi;)
ifeq ($(BOARD_USES_BCM_WIFI),true)
	cp -f ./$(KERNEL_OUT)/drivers/net/wireless/bcm4319/dhd.ko ./$(KERNEL_OUT)/../../system/lib
endif
	
kerneltags: $(KERNEL_OUT) $(KERNEL_CONFIG)
	#ZHP_OPROFILE_20101109 add end
	$(MAKE) -C kernel O=../$(KERNEL_OUT) ARCH=arm CROSS_COMPILE=arm-eabi- tags

kernelconfig: $(KERNEL_OUT) $(KERNEL_CONFIG)
	env KCONFIG_NOTIMESTAMP=true \
	     $(MAKE) -C kernel O=../$(KERNEL_OUT) ARCH=arm CROSS_COMPILE=arm-eabi- menuconfig
	cp $(KERNEL_OUT)/.config kernel/arch/arm/configs/$(KERNEL_DEFCONFIG)

endif
