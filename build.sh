#!/bin/bash

export PATH=$PATH:/opt/gcc-4.4.4-glibc-2.11.1-multilib-1.0/arm-fsl-linux-gnueabi/bin


make ARCH=arm CROSS_COMPILE=arm-fsl-linux-gnueabi- mx28evk_defconfig
make ARCH=arm CROSS_COMPILE=arm-fsl-linux-gnueabi-

#tools/mkimage -A arm -O u-boot -T mxsimage -n './arch/arm/cpu/arm926ejs/mxs/mxsimage-spl.mx28.cfg' -d ./spl/u-boot-spl ./spl/u-boot-spl.sb
#tools/mxsboot sd spl/u-boot-spl.sb spl/u-boot-spl.sd

cp u-boot imx-bootlets-src-10.12.01
cd imx-bootlets-src-10.12.01
./build
cd ..
cp imx-bootlets-src-10.12.01/imx28_ivt_uboot.sb /home/matsumotogi/Share/mfgtool/Profiles/MX28\ Linux\ Update/OS\ Firmware/files

