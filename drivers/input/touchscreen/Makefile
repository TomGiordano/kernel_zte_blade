
#========================================================================================
#when         who        what, where, why                         comment tag
#--------     ----       -------------------------------------    --------------------------
#2011-07-15   xym        add cypress CY8CTST241 touchscreen       ZTE_TS_XYM_20110715
#2011-07-11   xym        add FocalTech new drvier and head file   ZTE_TS_XYM_20110711
#2011-01-24   zfj        add FocalTech drvier and head file       ZTE_TS_ZFJ_20110124
#2010-07-03   wly        update firmware                          ZTE_WLY_CRDB00517999
#2010-03-29   wly        add cypress driver                       ZTE_TOUCH_WLY_001
#========================================================================================
#
# Makefile for the touchscreen drivers.
#

# Each configuration option enables a list of files.

wm97xx-ts-y := wm97xx-core.o

obj-$(CONFIG_TOUCHSCREEN_88PM860X)	+= 88pm860x-ts.o
obj-$(CONFIG_TOUCHSCREEN_AD7877)	+= ad7877.o
obj-$(CONFIG_TOUCHSCREEN_AD7879)	+= ad7879.o
obj-$(CONFIG_TOUCHSCREEN_ADS7846)	+= ads7846.o
obj-$(CONFIG_TOUCHSCREEN_ATMEL_TSADCC)	+= atmel_tsadcc.o
obj-$(CONFIG_TOUCHSCREEN_BITSY)		+= h3600_ts_input.o
obj-$(CONFIG_TOUCHSCREEN_CYPRESS_TMG)	+= cy8c_tmg_ts.o
obj-$(CONFIG_TOUCHSCREEN_DYNAPRO)	+= dynapro.o
obj-$(CONFIG_TOUCHSCREEN_CYPRESS_TMG)	+= cy8c_tmg_ts.o
obj-$(CONFIG_TOUCHSCREEN_HAMPSHIRE)	+= hampshire.o
obj-$(CONFIG_TOUCHSCREEN_GUNZE)		+= gunze.o
obj-$(CONFIG_TOUCHSCREEN_EETI)		+= eeti_ts.o
obj-$(CONFIG_TOUCHSCREEN_ELAN_I2C_8232) += elan8232_i2c.o
obj-$(CONFIG_TOUCHSCREEN_ELO)		+= elo.o
obj-$(CONFIG_TOUCHSCREEN_FUJITSU)	+= fujitsu_ts.o
obj-$(CONFIG_TOUCHSCREEN_INEXIO)	+= inexio.o
obj-$(CONFIG_TOUCHSCREEN_MC13783)	+= mc13783_ts.o
obj-$(CONFIG_TOUCHSCREEN_MCS5000)	+= mcs5000_ts.o
obj-$(CONFIG_TOUCHSCREEN_MIGOR)		+= migor_ts.o
obj-$(CONFIG_TOUCHSCREEN_MTOUCH)	+= mtouch.o
obj-$(CONFIG_TOUCHSCREEN_MK712)		+= mk712.o
obj-$(CONFIG_TOUCHSCREEN_MSM)		+= msm_ts.o
obj-$(CONFIG_TOUCHSCREEN_HP600)		+= hp680_ts_input.o
obj-$(CONFIG_TOUCHSCREEN_HP7XX)		+= jornada720_ts.o
obj-$(CONFIG_TOUCHSCREEN_HTCPEN)	+= htcpen.o
obj-$(CONFIG_TOUCHSCREEN_USB_COMPOSITE)	+= usbtouchscreen.o
obj-$(CONFIG_TOUCHSCREEN_PCAP)		+= pcap_ts.o
obj-$(CONFIG_TOUCHSCREEN_PENMOUNT)	+= penmount.o
obj-$(CONFIG_TOUCHSCREEN_QT602240)	+= qt602240_ts.o
obj-$(CONFIG_TOUCHSCREEN_S3C2410)	+= s3c2410_ts.o
obj-$(CONFIG_TOUCHSCREEN_MSM)		+= msm_ts.o
obj-$(CONFIG_TOUCHSCREEN_SYNAPTICS_I2C_RMI)	+= synaptics_i2c_rmi.o
obj-$(CONFIG_TOUCHSCREEN_SYNAPTICS_3K)          += synaptics_3k.o
obj-$(CONFIG_TOUCHSCREEN_SYNAPTICS_I2C_RMI_UPDATE)	+= SynaUpgrade.o
obj-$(CONFIG_TOUCHSCREEN_TOUCHIT213)	+= touchit213.o
obj-$(CONFIG_TOUCHSCREEN_TOUCHRIGHT)	+= touchright.o
obj-$(CONFIG_TOUCHSCREEN_TOUCHWIN)	+= touchwin.o
obj-$(CONFIG_TOUCHSCREEN_TSC2007)	+= tsc2007.o
obj-$(CONFIG_TOUCHSCREEN_UCB1400)	+= ucb1400_ts.o
obj-$(CONFIG_TOUCHSCREEN_WACOM_W8001)	+= wacom_w8001.o
obj-$(CONFIG_TOUCHSCREEN_WM97XX)	+= wm97xx-ts.o
obj-$(CONFIG_TOUCHSCREEN_DA9034)	+= da9034-ts.o
wm97xx-ts-$(CONFIG_TOUCHSCREEN_WM9705)	+= wm9705.o
wm97xx-ts-$(CONFIG_TOUCHSCREEN_WM9712)	+= wm9712.o
wm97xx-ts-$(CONFIG_TOUCHSCREEN_WM9713)	+= wm9713.o
obj-$(CONFIG_TOUCHSCREEN_WM97XX_ATMEL)	+= atmel-wm97xx.o
obj-$(CONFIG_TOUCHSCREEN_WM97XX_MAINSTONE)	+= mainstone-wm97xx.o
obj-$(CONFIG_TOUCHSCREEN_WM97XX_ZYLONITE)	+= zylonite-wm97xx.o
obj-$(CONFIG_TOUCHSCREEN_W90X900)	+= w90p910_ts.o
obj-$(CONFIG_TOUCHSCREEN_TPS6507X)	+= tps6507x-ts.o
obj-$(CONFIG_TOUCHSCREEN_MSM_LEGACY)		+= msm_touch.o
obj-$(CONFIG_TOUCHSCREEN_CY8C_TS)	+= cy8c_ts.o
#ZTE_TOUCH_WLY_001,@2010-03-29,begin
#ZTE_WLY_CRDB00517999, begin
obj-$(CONFIG_DOWNLOAD_FIRMWARE)		+= issp-firmware.o
issp-firmware-objs := issp_driver_routines.o issp_routines.o download_main.o
#ZTE_WLY_CRDB00517999, end
obj-$(CONFIG_TOUCHSCREEN_CYPRESS_I2C_RMI)	+= cypress_i2c_rmi.o
#ZTE_TOUCH_WLY_001,@2010-03-29,end
obj-$(CONFIG_TOUCHSCREEN_MXT224)       += mXT224_atmel.o atmel_config.o
obj-$(CONFIG_TOUCHSCREEN_CYTTSP_I2C)       += cyttsp-i2c.o
#ZTE_TS_ZFJ_20110124, begin
obj-$(CONFIG_TOUCHSCREEN_FOCALTECH)	+= ft5x0x_ts.o
#ZTE_TS_ZFJ_20110124, end
#ZTE_TS_XYM_20110711 begin
obj-$(CONFIG_TOUCHSCREEN_FOCALTECH_NEW)	+= ft5x0x_ts_new.o
#ZTE_TS_XYM_20110711 end
#ZTE_TS_XYM_20110715 begin
obj-$(CONFIG_TOUCHSCREEN_CY8CTST241)		+= cy8ctst241.o
cy8ctst241-objs := cyttsp_core.o cyttsp_i2c.o
#ZTE_TS_XYM_20110715 end