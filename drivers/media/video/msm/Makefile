GCC_VERSION      := $(shell $(CONFIG_SHELL) $(PWD)/scripts/gcc-version.sh $(CROSS_COMPILE)gcc)
ifeq ($(GCC_VERSION),0404)
CFLAGS_REMOVE_msm_vfe8x.o = -Wframe-larger-than=1024
endif

obj-$(CONFIG_MSM_CAMERA) += msm_camera.o msm_v4l2.o msm_axi_qos.o
obj-$(CONFIG_SENSOR_INFO) += msm_sensorinfo.o
obj-$(CONFIG_MSM_CAMERA_FLASH) += flash.o
obj-$(CONFIG_ARCH_MSM_ARM11) += msm_vfe7x.o msm_io7x.o
obj-$(CONFIG_ARCH_MSM7X30) += msm_vfe31.o msm_io_vfe31.o msm_vpe1.o
obj-$(CONFIG_ARCH_QSD8X50) += msm_vfe8x.o msm_vfe8x_proc.o msm_io8x.o
obj-$(CONFIG_ARCH_MSM8X60) += msm_vfe31.o msm_io_8x60.o msm_vpe1.o
obj-$(CONFIG_MT9T013) += mt9t013.o mt9t013_reg.o
obj-$(CONFIG_MT9D112) += mt9d112.o mt9d112_reg.o
obj-$(CONFIG_SN12M0PZ) += sn12m0pz.o sn12m0pz_reg.o
obj-$(CONFIG_MT9P111_MCNEX) += mt9p111_mcnex.o mt9p111_reg_mcnex.o
obj-$(CONFIG_MT9P111_SAMSUNG_FPC) += mt9p111_samsung_fpc.o mt9p111_reg_samsung_fpc.o
obj-$(CONFIG_MT9P111_SAMSUNG_SOCKET) += mt9p111_samsung_socket.o mt9p111_reg_samsung_socket.o
obj-$(CONFIG_MT9V113) += mt9v113.o mt9v113_reg.o
obj-$(CONFIG_MT9T11X_QTECH_MCNEX_FPC) += mt9t11x_qtech_mcnex_fpc.o mt9t112_reg_qtech_mcnex_fpc.o mt9t111_reg_qtech_mcnex_fpc.o
obj-$(CONFIG_MT9T11X_QTECH_MCNEX_SOCKET) += mt9t11x_qtech_mcnex_socket.o mt9t111_reg_qtech_mcnex_socket.o mt9t112_reg_qtech_mcnex_socket.o
obj-$(CONFIG_MT9D115_QTECH_SUNNY_SOCKET)  += mt9d115_qtech_sunny_socket.o mt9d115_reg_qtech_sunny_socket.o
obj-$(CONFIG_MT9D115_QTECH_SUNNY_FPC)  += mt9d115_qtech_sunny_fpc.o mt9d115_reg_qtech_sunny_fpc.o
obj-$(CONFIG_MT9D113_QTECH_SUNNY_SOCKET)  += mt9d113_qtech_sunny_socket.o mt9d113_reg_qtech_sunny_socket.o
obj-$(CONFIG_OV5642_GLOBALOPTICS)  += ov5642_globaloptics.o ov5642_reg_globaloptics.o
obj-$(CONFIG_HI704_KERR)  += hi704_kerr_fpc.o hi704_reg_kerr_fpc.o
obj-$(CONFIG_OV5640_GLOBALOPTICS)  += ov5640_globaloptics.o ov5640_reg_globaloptics.o
obj-$(CONFIG_S5K5CAGX_SAMSUNG_FPC)  += s5k5cagx_samsung_fpc.o s5k5cagx_reg_samsung_fpc.o
obj-$(CONFIG_S5K5CAGX_MCNEX_QTECH_FPC)  += s5k5cagx_mcnex_qtech_fpc.o s5k5cagx_reg_mcnex_qtech_fpc.o
obj-$(CONFIG_MT9P012) += mt9p012_reg.o
obj-$(CONFIG_MSM_CAMERA_AF_FOXCONN) += mt9p012_fox.o
obj-$(CONFIG_MSM_CAMERA_AF_BAM) += mt9p012_bam.o
obj-$(CONFIG_MT9P012_KM) += mt9p012_km.o mt9p012_km_reg.o
obj-$(CONFIG_S5K3E2FX) += s5k3e2fx.o
obj-$(CONFIG_VX6953) += vx6953.o vx6953_reg.o
obj-$(CONFIG_VB6801) += vb6801.o
obj-$(CONFIG_IMX074) += imx074.o imx074_reg.o
obj-$(CONFIG_WEBCAM_OV7692) += ov7692.o
obj-$(CONFIG_MSM_GEMINI) += msm_gemini_dev.o msm_gemini_sync.o msm_gemini_core.o msm_gemini_hw.o msm_gemini_platform.o
