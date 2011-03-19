#history   
# when        who              what                         tag 
# 2010-01-06  hp             for CQ430643 and CQ431832   ZTE_WIFI_HP_007 
#
#
#
#  Local Makefile includes for tool and kernel source paths
#  ***** This is only a sample, modify the paths for your specific build environment *****
# 
#  Copyright 2004-2006 Atheros Communications, Inc.
#
# When creating a new build type, use the following template:
#  	ifeq ($(ATH_BUILD_TYPE),<platform name>) 
#       ATH_ARCH_CPU_TYPE :=  < cpu architecture >
#       ATH_CROSS_COMPILE_TYPE := < cross compiler path >
#       ATH_LINUXPATH := < kernel source path >

#ATH_BUILD_TYPE := <Platform (Eg: LOCAL_i686)>
#ATH_BUS_TYPE := <Interconnect type (Eg: SDIO, SPI)>
#ATH_LINUXPATH := <Pointer to kernel source (Eg: /<path>/fc3-i686-2.6.9)>
#ATH_ARCH_CPU_TYPE := <Type of architecture (Eg: arm)>
#ATH_CROSS_COMPILE_TYPE := <Path to the compiler toolchain - Eg: (/<path>/arm_v4t_le-)
#ATH_OS_SUB_TYPE := <Kernel version - (Eg: linux_2_4, linux_2_6)>

#ATHENV
ATH_BUILD_TYPE := QUALCOMM_ARM_NATIVEMMC
ATH_BSP_TYPE := MSM7627_BSP4715
# x86 case
ifeq ($(ATH_BUILD_TYPE), LOCAL_i686_NATIVEMMC)

  $(info *************************)
  $(info PLATFORM: $(ATH_BUILD_TYPE))
  $(info *************************)

  export ATH_BUS_SUBTYPE=linux_sdio
  ATH_BUS_TYPE := SDIO
  ATH_ANDROID_ENV := yes
#  ATH_FORCE_TCMD_MODE := yes
#  ATH_FORCE_ART_MODE := yes
#  ATH_EEPROM_FILE_USED := yes
  ATH_LINUXPATH := /usr/src/linux-2.6.29-rc8
  ATH_OS_SUB_TYPE := linux_2_6

  # SDIO Host Driver
  ifeq ($(ATH_BUS_TYPE), SDIO)
    ATH_HC_DRIVERS :='pci_ellen/ pci_std/'
  endif

endif

# Qualcomm platform case with Atheros private SD stack
ifeq ($(ATH_BUILD_TYPE), QUALCOMM_ARM)

  $(info *************************)
  $(info PLATFORM: $(ATH_BUILD_TYPE))
  $(info BSP:      $(ATH_BSP_TYPE))
  $(info *************************)

  # Some environment settings
  ATH_ANDROID_ENV := yes
#  ATH_FORCE_TCMD_MODE := yes
#  ATH_FORCE_ART_MODE := yes
#  ATH_EEPROM_FILE_USED := yes

  # Kernel path
  ifeq ($(ATH_BSP_TYPE), QSD8K_BSP2090)
    ATH_LINUXPATH := /na_test2/android/android_8k_2090/LINUX/kernel
  endif
  ifeq ($(ATH_BSP_TYPE), QSD8K_BSP2070)
    ATH_LINUXPATH := /na_test2/qsd8k/bsp2070/LINUX/project/platform-qsd8650/build-target/kernel
    ATH_ANDROID_ENV := no
  endif
  ifeq ($(ATH_BSP_TYPE), MSM7201A_BSP6360)
    ATH_LINUXPATH := /na_test2/android/android_7k_6360/LINUX/kernel
  endif
  ifeq ($(ATH_BSP_TYPE), MSM7201A_BSP6350)
    ATH_LINUXPATH := /na_test2/android/android_7k_6350/LINUX/kernel
  endif

  # Some common variable
  ATH_ARCH_CPU_TYPE := arm 
  ATH_CROSS_COMPILE_TYPE := arm-eabi-
  ATH_BUS_TYPE := SDIO

  # SDIO host driver
  ifeq ($(ATH_BUS_TYPE), SDIO)
    ATH_HC_DRIVERS := 'qualcomm/'
  endif

endif

# Qualcomm platform case
ifeq ($(ATH_BUILD_TYPE), QUALCOMM_ARM_NATIVEMMC)

  $(info *************************)
  $(info PLATFORM: $(ATH_BUILD_TYPE))
  $(info BSP:      $(ATH_BSP_TYPE))
  $(info *************************)

  # Some environment settings
  ATH_ANDROID_ENV := yes
#  ATH_FORCE_TCMD_MODE := yes
#  ATH_FORCE_ART_MODE := yes
#  ATH_EEPROM_FILE_USED := yes

  # Kernel path
  ifeq ($(ATH_BSP_TYPE), QSD8K_BSP3180)
    ATH_LINUXPATH := /home/neo/Android/Qualcomm/8K/3180/LINUX/android/out/target/product/qsd8250_surf/obj/KERNEL_OBJ
  endif
  ifeq ($(ATH_BSP_TYPE), QSD8K_BSP3170)
    ATH_LINUXPATH := /home/neo/Android/Qualcomm/8K/3170/LINUX/android/out/target/product/qsd8250_surf/obj/KERNEL_OBJ
  endif
  ifeq ($(ATH_BSP_TYPE), MSM7201A_BSP6370)
    ATH_LINUXPATH := /na_test2/android/android_7k_6370/LINUX/android/out/target/product/msm7201a_surf/obj/KERNEL_OBJ
  endif
  ifeq ($(ATH_BSP_TYPE), MSM7227_BSP4715)
    ATH_LINUXPATH := /home/tom/Android/source/kernel/blade/2.6.32/kernel/kernel
  endif
  ifeq ($(ATH_BSP_TYPE), MSM7227_BSP4515)
    ATH_LINUXPATH := /home/tom/Android/source/kernel/blade/2.6.32/kernel/kernel
  endif
  ifeq ($(ATH_BSP_TYPE), MSM7627_BSP4715)
    #ZTE_WIFI_HP_007 
    ATH_SAVE_EEPROM_TO_FILE := yes
    #ZTE_WIFI_HP_007 end 
    ATH_LINUXPATH := /home/tom/Android/source/kernel/blade/2.6.32/kernel/kernel
  endif
  # Some common variable
  export ATH_BUS_SUBTYPE=linux_sdio
  ATH_ARCH_CPU_TYPE := arm 
  ATH_CROSS_COMPILE_TYPE := arm-eabi-
  ATH_BUS_TYPE := SDIO

endif
#ATHENV
