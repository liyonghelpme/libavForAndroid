#!/bin/bash
PREFIX=./myAndroid
NDK=~/android-ndk-r8
PLATFORM=$NDK/mytool
PREBUILD=$NDK/mytool


$PREBUILD/bin/arm-linux-androideabi-ld -rpath-link=$PLATFORM/sysroot/usr/lib -L$PLATFORM/sysroot/usr/lib  -soname libx264.so -shared -nostdlib  -z,noexecstack -Bsymbolic --whole-archive --no-undefined -o $PREFIX/libx264.so libx264.a  -lc -lm -lz -ldl -llog  --warn-once  --dynamic-linker=/system/bin/linker $PREBUILD/lib/gcc/arm-linux-androideabi/4.4.3/libgcc.a
