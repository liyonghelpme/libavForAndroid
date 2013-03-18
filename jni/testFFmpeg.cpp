#include <jni.h>
#include <android/log.h>
#include <stdio.h>
#include <dlfcn.h>

extern "C" {
    #include "libavcodec/avcodec.h"
    #include "libavformat/avformat.h"
    #include "libavutil/colorspace.h"
};

#define  LOG_TAG    "libgl2jni"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

JNIEnv *jniEnv;
int testLib();
extern "C" {
    JNIEXPORT void JNICALL Java_com_liyong_libav_GL2JNILib_init(JNIEnv * env, jobject obj,  jint width, jint height, jbyteArray str);
    JNIEXPORT void JNICALL Java_com_liyong_libav_GL2JNILib_step(JNIEnv * env, jobject obj);


};


JNIEXPORT void JNICALL Java_com_android_gl2jni_GL2JNILib_init(JNIEnv * env, jobject obj,  jint width, jint height, jbyteArray str)
{
    jniEnv = env;
    testLib();
}

JNIEXPORT void JNICALL Java_com_android_gl2jni_GL2JNILib_step(JNIEnv * env, jobject obj)
{
}


int testLib()
{
    /*
    const char *err = NULL;
    char *fileName = "/data/data/com.android.gl2jni/lib/libffmpeg.so";
    LOGI("clearing errors:");
    err = dlerror();
    LOGI("%s\n", (err == NULL) ? "OK": err);

    LOGI("Loading Library\n");
    void *handle = dlopen(fileName, RTLD_LAZY);
    err = dlerror();
    LOGI("%s\n", (err == NULL) ? "OK": err);

    if(handle != NULL)
    {
        LOGI("Loading symbol \n");
        dlsym(handle, "avcodec_encode_video");
        err = dlerror();
        LOGI("%s\n", (err == NULL) ? "OK": err);
    }
    */
}
