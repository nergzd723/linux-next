#!/bin/bash

make ARCH=arm64 LLVM=1 -j12 O=.output

KERNEL_PATH=../.output/arch/arm64/boot/Image DTB_PATH=../.output/arch/arm64/boot/dts/exynos/exynos9810-star.dtb make -C minimal_sboot_wrapper/

cp minimal_sboot_wrapper/wrapped-kernel magisk/kernel

cd magisk

./magiskboot repack twrp-3.6.2_9-0-starlte.img ../boot.img

cd ..

adb wait-for-recovery

adb push boot.img /sdcard
