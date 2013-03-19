#!/bin/bash
#PREFIX=./myAndroid
#NDK=~/android-ndk-r8
#PLATFORM=$NDK/mytool
#PREBUILD=$NDK/mytool

PREFIX=./myIOS

IOS=/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/usr
#include and lib
IOSLIB=/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS6.1.sdk
PREBUILD=$IOS

$PREBUILD/bin/ar d libavcodec/libavcodec.a inverse.o
$PREBUILD/bin/ld -arch armv7  -syslibroot $IOSLIB  -install_name libffmpeg.dylib -dylib  -o $PREFIX/libffmpeg.dylib libavcodec/libavcodec.a libavformat/libavformat.a libavutil/libavutil.a libswscale/libswscale.a  
