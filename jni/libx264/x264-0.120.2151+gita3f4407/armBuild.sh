#!/bin/bash
PREFIX=./myAndroid
NDK=~/android-ndk-r8
PLATFORM=$NDK/mytool
PREBUILD=$NDK/mytool
./configure --host=arm-linux \
    --prefix=$PREFIX \
    --exec-prefix=$PREFIX \
    --extra-cflags=" -fPIC -DANDROID" \
    --extra-ldflags="-Wl,-rpath-link=$PLATFORM/lib -L $PLATFORM/lib -nostdlib -lc -lm -ldl -llog" \
    --cross-prefix=$PREBUILD/bin/arm-linux-androideabi- \
    --sysroot=$PLATFORM/sysroot \
    --disable-cli \
    --enable-shared \
    --enable-static \
    --enable-pic \
    --disable-asm 
    



