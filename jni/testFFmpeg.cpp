#include <jni.h>
#include <android/log.h>
#include <stdio.h>
#include <dlfcn.h>

/*
extern "C" {
    #include "libavcodec/avcodec.h"
    #include "libavformat/avformat.h"
    #include "libavutil/colorspace.h"
};
*/

#define  LOG_TAG    "libgl2jni"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

JNIEnv *jniEnv;
extern "C" {
    JNIEXPORT void JNICALL Java_com_liyong_libav_GL2JNILib_init(JNIEnv * env, jobject obj,  jint width, jint height, jbyteArray str);
    JNIEXPORT void JNICALL Java_com_liyong_libav_GL2JNILib_step(JNIEnv * env, jobject obj);
};


JNIEXPORT void JNICALL Java_com_liyong_libav_GL2JNILib_init(JNIEnv * env, jobject obj,  jint width, jint height, jbyteArray str)
{
    LOGI("init testFFmpeg"); 
    jniEnv = env;
}

JNIEXPORT void JNICALL Java_com_liyong_libav_GL2JNILib_step(JNIEnv * env, jobject obj)
{
}


