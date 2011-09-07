#!/bin/sh

set -e
npages=15000
disksize="$((npages*4300))"
[ -d tmpmnt ] || mkdir tmpmnt
[ -d zram0mnt ] || mkdir zram0mnt
i=0
rm -f tmpmnt/tmpfile*
grep -q " $PWD/tmpmnt " /proc/mounts && umount tmpmnt
grep -q " $PWD/zram0mnt " /proc/mounts && umount zram0mnt
cat /sys/block/zram0/mem_used_total
mount -t tmpfs -o size=10G tmpfs tmpmnt
du tmpmnt

echo 1 >/sys/block/zram0/reset
sleep 2
echo "$disksize" >/sys/block/zram0/disksize
cat /sys/block/zram0/mem_used_total

openssl rand -base64 "$((npages*3000))" >tmpmnt/tmpfile

mke2fs /dev/zram0
mount /dev/zram0 zram0mnt
cmpfile=zram0mnt/f

while [ $i -lt 10 ]
do
	free
	echo "cycle $i:"
	cat tmpmnt/tmpfile >"$cmpfile"
	du tmpmnt
	cat /sys/block/zram0/mem_used_total
        cmp -b tmpmnt/tmpfile "$cmpfile"
	cat tmpmnt/tmpfile >tmpmnt/tmpfile$i
	i="$((i+1))"
done
