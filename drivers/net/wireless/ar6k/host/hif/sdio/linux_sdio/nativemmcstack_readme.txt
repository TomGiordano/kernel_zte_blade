HIF support for native Linux MMC Stack.
paull@atheros.com

4/1/2009  
Added support for Tokyo Electron Ellen I card in Linux 2.6.25-14 (Fedora Core 9)
The patch mmc2.6.25-14.patch supports Ellen I and ENE host controllers.

12/18/2008
Tested on Freescale MX27 and OMAP3530 Beageleboard Linux ver 2.6.28
adds DMA bounce buffer support
hif.c ver 5 and hif.h ver 4 are for the old HTC/HIF interface and shouold be useable with 2.1 drivers
ver 6 and 5 are for the new HTC/HIF interface
For older Linux MMC stack versions, comment out in hif.c hifDeviceInserted() the lines:
    /* give us some time to enable, in ms */
    func->enable_timeout = 100;
it is only required on some platforms, eg Beagleboard.

7/18/2008

a. tested on Fedora Core 9 kernel 2.6.25.6, x86 with ENE standrad host controller,using the Olca 2.1.1RC.15
b. requires applying the linux2.6.25.6mmc.patch to the kernel drivers/mmc directory
c. through put is 20-22mbs up/down link
d. new platform type is:
	ATH_PLATFORM=LOCAL_i686_NATIVEMMC-SDIO
	TARGET_TYPE=AR6002
e. known issues: unloading the driver on Fedora Core 9 after conecting to an AP seems to not be complete.



