#!/bin/bash

make ARCH=arm64 LLVM=1 -j12 O=.output

#RAMDISK_PATH=../magisk/ramdisk.cpio KERNEL_PATH=../.output/arch/arm64/boot/Image DTB_PATH=../.output/arch/arm64/boot/dts/exynos/exynos9810-star.dtb make -C minimal_sboot_wrapper/

cp .output/arch/arm64/boot/Image uniLoader/blob/Image
cp .output/arch/arm64/boot/dts/exynos/exynos9810-star.dtb uniLoader/blob/dtb

make -C uniLoader/ clean
make CROSS_COMPILE=aarch64-linux-gnu- -C uniLoader/

mkbootimg-osm0sis \
	--kernel uniLoader/uniLoader \
	--ramdisk newfs \
	--base 0x10000000 \
	--second_offset 0x00f00000 \
	--cmdline buildvariant=eng \
	--kernel_offset 0x00008000 \
	--ramdisk_offset 0x01000000 \
	--tags_offset 0x00000100 \
	--pagesize 2048 \
	--header_version 1 \
	--os_version 12.0.0 \
	--os_patch_level 2021-12 \
	--dt dt.img \
	-o boot.img
adb wait-for-recovery

adb push boot.img /sdcard
