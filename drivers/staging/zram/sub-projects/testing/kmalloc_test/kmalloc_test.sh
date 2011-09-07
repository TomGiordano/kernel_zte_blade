#!/bin/bash
cd ../../swap_replay/sr_parse
./sr_data_histo /tmp/sample_sr_data_2 -raw | grep W | awk '{ print $2 " " $3 }' | while read line
do
	echo $line > /proc/kmalloc_test
done
      
