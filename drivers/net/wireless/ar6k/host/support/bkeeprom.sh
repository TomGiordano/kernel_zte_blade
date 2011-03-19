#!/bin/sh
# backup EEPROM data to a file,the file name is the option,if the filename not specified,using last part of MAC address as the file name 
#usage bkeeprom filename.epm
export WORKAREA=$PWD/../../
export IMAGEPATH=$WORKAREA/host/.output/$ATH_PLATFORM/image
sudo $WORKAREA/host/support/loadAR6000.sh unloadall
sudo $WORKAREA/host/support/loadAR6000.sh bmi
sudo $IMAGEPATH/eeprom.AR6002 -r -f $1.epm 
if [ $? -eq 0 ] ;then
    if [ $1 -eq ""]; then
       MACADDR1=`sudo xxd -s 0xd -l 8 $1.epm|cut -b 10-11`
       MACADDR2=`sudo xxd -s 0xd -l 8 $1.epm|cut -b 12-13`
       MACADDR3=`sudo xxd -s 0xd -l 8 $1.epm|cut -b 15-16`
       IMAGENAME=ES4.TT.$MACADDR1.$MACADDR2.$MACADDR3
       sudo mv $1.epm $IMAGENAME.epm
       sudo chown arkin:arkin $IMAGENAME.epm
    fi
    echo Board Data backup-ed successfully
else
    echo EEPROM reading failed
fi
sudo $WORKAREA/host/support/loadAR6000.sh unloadall
