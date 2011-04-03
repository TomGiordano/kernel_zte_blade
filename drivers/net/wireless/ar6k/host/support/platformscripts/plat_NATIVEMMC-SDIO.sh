#!/bin/sh
#
# Linux Native MMC-SDIO stack platform setup script
#
#

case $1 in
	loadbus)
	# make sure this platform actually has the native MMC stack loaded
	lsmod | grep mmc_core > /dev/null   
	if [ $? -ne 0 ]; then
		echo "*** Native Linux MMC stack not loaded!"
		exit -1
	fi
	;;
	
	unloadbus)
	# nothing to do for native MMC stack
	;;
	
	loadAR6K)
	$IMAGEPATH/recEvent $AR6K_TGT_LOGFILE /dev/null 2>&1 &
	/sbin/insmod $IMAGEPATH/$AR6K_MODULE_NAME.ko $AR6K_MODULE_ARGS
	if [ $? -ne 0 ]; then
		echo "*** Failed to install AR6K Module"
		exit -1
	fi
	;;
	
	unloadAR6K)
	/sbin/rmmod -w $AR6K_MODULE_NAME.ko
 	killall recEvent
	;;
	*)
		echo "Unknown option : $1"
	
esac
