#!/bin/bash
PREFIX=./myAndroid
NDK=~/android-ndk-r8
PLATFORM=$NDK/mytool
PREBUILD=$NDK/mytool

$PREBUILD/bin/arm-linux-androideabi-ar d libavcodec/libavcodec.a inverse.o
$PREBUILD/bin/arm-linux-androideabi-ld -rpath-link=$PLATFORM/sysroot/usr/lib -L$PLATFORM/sysroot/usr/lib  -soname libffmpeg.so -shared -nostdlib  -z,noexecstack -Bsymbolic --whole-archive --no-undefined -o $PREFIX/libffmpeg.so libavcodec/libavcodec.a libavformat/libavformat.a libavutil/libavutil.a libswscale/libswscale.a  -lc -lm -lz -ldl -llog  --warn-once  --dynamic-linker=/system/bin/linker $PREBUILD/lib/gcc/arm-linux-androideabi/4.4.3/libgcc.a
cp $PREFIX/libffmpeg.so ../
