#
# Makefile for miscellaneous I2C chip drivers.
#
# Do not add new drivers to this directory! It is DEPRECATED.
#
# Device drivers are better grouped according to the functionality they
# implement rather than to the bus they are connected to. In particular:
# * Hardware monitoring chip drivers go to drivers/hwmon
# * RTC chip drivers go to drivers/rtc
# * I/O expander drivers go to drivers/gpio
#

obj-$(CONFIG_SENSORS_AK8973)		+= akm8973.o 
obj-$(CONFIG_SENSORS_AKM8976)	+= akm8976.o
obj-$(CONFIG_SENSORS_AKM8962)   	+= akm8962.o
obj-$(CONFIG_DS1682)		+= ds1682.o
obj-$(CONFIG_SENSORS_PCA963X)	+= pca963x.o
obj-$(CONFIG_SENSORS_AK8973)   += akm8973.o
obj-$(CONFIG_SENSORS_TSL2550)	+= tsl2550.o
obj-$(CONFIG_SENSORS_MT9T013)	+= mt9t013.o
obj-$(CONFIG_VP_A1026)		+= a1026.o
obj-$(CONFIG_SENSOR_ADXL34X) 		+= adxl34x.o
obj-$(CONFIG_SENSOR_ACCELERATOR) 		+= accelerator.o
obj-$(CONFIG_SENSORS_BH1721)		+= bh1721.o
obj-$(CONFIG_SENSORS_TSL2771)	+= taos.o
obj-$(CONFIG_SENSOR_LIS302DL) 		+= lis302dl.o
obj-$(CONFIG_NFC_PN544)         += pn544.o 
ifeq ($(CONFIG_I2C_DEBUG_CHIP),y)
EXTRA_CFLAGS += -DDEBUG
endif

