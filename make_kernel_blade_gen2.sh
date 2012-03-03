make cm_blade_gen2_defconfig
schedtool -B -n 1 -e ionice -n 1 make -j `cat /proc/cpuinfo | grep "^processor" | wc -l`
mkdir ./output
make >&1 | tee ./output/log.txt
cp ./arch/arm/boot/zImage ./output
find ./ -name "*.ko" -exec cp {} ./output \;
