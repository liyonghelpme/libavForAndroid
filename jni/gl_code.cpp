/*
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// OpenGL ES 2.0 code

#include <jni.h>
#include <android/log.h>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <asm-generic/errno-base.h>
#include <errno.h>

#include <sys/time.h>
extern "C" {
    #include "libavcodec/avcodec.h"
    #include "libavformat/avformat.h"
    #include "libavutil/colorspace.h"
};

#define  LOG_TAG    "libgl2jni"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)




jclass GL2JNIView;
std::string fileName;
JNIEnv * jniEnv;
void saveBmp(char *buffer);
static void initVideo();
static void deleteVideo();

static void printGLString(const char *name, GLenum s) {
    const char *v = (const char *) glGetString(s);
    LOGI("GL %s = %s\n", name, v);
}

static void checkGlError(const char* op) {
    for (GLint error = glGetError(); error; error
            = glGetError()) {
        LOGI("after %s() glError (0x%x)\n", op, error);
    }
}

static const char gVertexShader[] = 
    "attribute vec4 vPosition;\n"
    "void main() {\n"
    "  gl_Position = vPosition;\n"
    "}\n";

static const char gFragmentShader[] = 
    "precision mediump float;\n"
    "void main() {\n"
    "  gl_FragColor = vec4(0, 1.0, 0.0, 1.0);\n"
    "}\n";

GLuint loadShader(GLenum shaderType, const char* pSource) {
    GLuint shader = glCreateShader(shaderType);
    if (shader) {
        glShaderSource(shader, 1, &pSource, NULL);
        glCompileShader(shader);
        GLint compiled = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (!compiled) {
            GLint infoLen = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
            if (infoLen) {
                char* buf = (char*) malloc(infoLen);
                if (buf) {
                    glGetShaderInfoLog(shader, infoLen, NULL, buf);
                    LOGE("Could not compile shader %d:\n%s\n",
                            shaderType, buf);
                    free(buf);
                }
                glDeleteShader(shader);
                shader = 0;
            }
        }
    }
    return shader;
}

GLuint createProgram(const char* pVertexSource, const char* pFragmentSource) {

    char buff[100];
    int len;
    GLuint vertexShader = loadShader(GL_VERTEX_SHADER, pVertexSource);
    if (!vertexShader) {
        return 0;
    }
    checkGlError("loadShader");

    glGetShaderInfoLog(vertexShader, 100, &len, buff);
    LOGI("shader log %s", buff);

    GLuint pixelShader = loadShader(GL_FRAGMENT_SHADER, pFragmentSource);
    if (!pixelShader) {
        return 0;
    }
    glGetShaderInfoLog(pixelShader, 100, &len, buff);
    LOGI("shader log %s", buff);

    GLuint program = glCreateProgram();


    if (program) {
        glAttachShader(program, vertexShader);
        LOGI("glAttachShader %d %s", program, pVertexSource);
        checkGlError("glAttachShader");
        glAttachShader(program, pixelShader);
        LOGI("glAttachShader %d %s", program, pFragmentSource);
        checkGlError("glAttachShader");
        glLinkProgram(program);
        GLint linkStatus = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
        if (linkStatus != GL_TRUE) {
            GLint bufLength = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &bufLength);
            if (bufLength) {
                char* buf = (char*) malloc(bufLength);
                if (buf) {
                    glGetProgramInfoLog(program, bufLength, NULL, buf);
                    LOGE("Could not link program:\n%s\n", buf);
                    free(buf);
                }
            }
            glDeleteProgram(program);
            program = 0;
        }
    }

    glGetProgramInfoLog(program, 100, &len, buff);
    LOGI("program log %s", buff);


    return program;
}

GLuint gProgram;
GLuint gvPositionHandle;
GLuint framebuffer;
GLuint colorRender;
GLuint depthRender;
GLuint texture;
GLuint depthTexture;

GLuint width;
GLuint height;

GLuint textureProgram;
GLuint tvpos;
GLuint tex;

static const char tv[] = 
    "attribute vec4 vPosition;\n"
    "varying vec2 uv;\n"
    "void main() {\n"
    "  gl_Position = vPosition;\n"
    "  uv = vPosition.xy;\n"
    "}\n";

static const char tf[] = 
    "precision mediump float;\n"
    "uniform sampler2D tex;\n"
    "varying vec2 uv;\n"
    "void main() {\n"
    "  vec4 t = texture2D(tex, uv);\n"
    "  gl_FragColor = vec4(1.0, 0, 0, 1);\n"
    "}\n";


jmethodID jniSaveBmp;
jmethodID jniPrintHello;


bool setupGraphics(int w, int h) {
    //initVideo();

    GL2JNIView = jniEnv->FindClass("com/android/gl2jni/GL2JNIView");
    LOGI("GL2JNIView %d", GL2JNIView);
    //jniSaveBmp = jniEnv->GetStaticMethodID(GL2JNIView, "saveBmp", "([C)V");
    //LOGI("saveBmp %d", jniSaveBmp);
    //jniPrintHello = jniEnv->GetStaticMethodID(GL2JNIView, "printHello", "()V");

	width = w;
    height = h;

    glGenFramebuffers(1, &framebuffer);
    checkGlError("glGenFramebuffers");


	glGenTextures(1, &texture);
    checkGlError("glGenTextures");
	glBindTexture(GL_TEXTURE_2D, texture);
    checkGlError("glBindTexture");
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    checkGlError("glTexImage2D");

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    checkGlError("glTexParameteri");
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    checkGlError("glTexParameteri");
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    checkGlError("glTexParameteri");
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    checkGlError("glTexParameteri");

	glBindTexture(GL_TEXTURE_2D, 0);


    /*
	glGenTextures(1, &depthTexture);
    checkGlError("glGenTextures");
	glBindTexture(GL_TEXTURE_2D, depthTexture);
    checkGlError("glBindTexture");
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
	            512, 1024,
	            0, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, NULL);
    checkGlError("glTexImage2D");

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    checkGlError("glTexParameteri");
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    checkGlError("glTexParameteri");
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    checkGlError("glTexParameteri");
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    checkGlError("glTexParameteri");

	glBindTexture(GL_TEXTURE_2D, 0);
    checkGlError("glBindTexture");

    */

	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    checkGlError("glBindFramebuffer");
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
    checkGlError("glFramebufferTexture2D  0");

	//glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture, 0);
    //checkGlError("glFramebufferTexture2D  1");


	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

    LOGI("framebuffer %d %d %d %d %d %d %d", framebuffer, texture, depthTexture, status, GL_FRAMEBUFFER_COMPLETE, width, height);

	if(status != GL_FRAMEBUFFER_COMPLETE)
	{
		LOGI("error init framebuffer");
	}

    printGLString("Version", GL_VERSION);
    printGLString("Vendor", GL_VENDOR);
    printGLString("Renderer", GL_RENDERER);
    printGLString("Extensions", GL_EXTENSIONS);

    LOGI("setupGraphics(%d, %d)", w, h);
    gProgram = createProgram(gVertexShader, gFragmentShader);

    if (!gProgram) {
        LOGE("Could not create program.");
        return false;
    }



    gvPositionHandle = glGetAttribLocation(gProgram, "vPosition");
    checkGlError("glGetAttribLocation");
    LOGI("glGetAttribLocation(\"vPosition\") = %d\n",
            gvPositionHandle);

    textureProgram = createProgram(tv, tf);
    if(!textureProgram)
    {
        LOGE("textureProgram error");
        return false;
    }

    LOGI("createBothProgram %d %d", gProgram, textureProgram);

    tvpos = glGetAttribLocation(textureProgram, "vPosition");
    LOGI("glGetAttribLocation(\"tvpos\") = %d\n", tvpos);
    tex = glGetUniformLocation(textureProgram, "tex");
    LOGI("glGetUniformLocation(\"tex\") = %d\n", tex);


    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glViewport(0, 0, w, h);
    checkGlError("glViewport");
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return true;
}

const GLfloat gTriangleVertices[] = { 
        -1.0f, -1.0f,
        1, -1,
        1, 1,
        -1, -1,
        1, 1, 
        -1, 1,
        };
const GLfloat gRect[] = {
    0, 0, 
    1, 1,
    0, 1,
    0, 0,
    1, 0,
    1, 1,
}; 

int bufferSize;
//RGB
char *fetchPixel()
{
    bufferSize = sizeof(int)*width*height;
    char *buff = (char*)malloc(bufferSize);//RGBA  只要RGB即可   
    LOGI("buffSize %d", bufferSize);
    //glPixelStorei();
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, buff);    
    checkGlError("glReadPixels");
    return buff;
}
void realDraw()
{
    glViewport(0, 0, width, height);
	static float grey;
    grey += 0.01f;
    if (grey > 1.0f) {
        grey = 0.0f;
    }
    glUseProgram(gProgram);
    checkGlError("glUseProgram");

    glVertexAttribPointer(gvPositionHandle, 2, GL_FLOAT, GL_FALSE, 0, gTriangleVertices);
    checkGlError("glVertexAttribPointer");
    glEnableVertexAttribArray(gvPositionHandle);
    checkGlError("glEnableVertexAttribArray");
    glDrawArrays(GL_TRIANGLES, 0, 6);
    checkGlError("glDrawArrays");


}
void drawSmallView()
{
    glViewport(0, 0, width/2, height/2);
    glUseProgram(textureProgram);
    checkGlError("glUseProgram1");

    glVertexAttribPointer(tvpos, 2, GL_FLOAT, GL_FALSE, 0, gRect);
    checkGlError("glVertexAttribPointer1");

    glEnableVertexAttribArray(tvpos);

    glActiveTexture(GL_TEXTURE0);
    checkGlError("glActiveTexture");
    glBindTexture(GL_TEXTURE_2D, texture);
    checkGlError("glBindTexture");
    glUniform1i(tex, 0);
    checkGlError("glUniform1i");
    glDrawArrays(GL_TRIANGLES, 0, 6);
    checkGlError("glDrawArrays");
}
//绘制大三角到 主屏幕中
//读取屏幕像素
//写入到纹理里面
//绘制纹理 到 一个 小平面上
//退出 程序需要 
void readAndStorePixel();
int begin = 0;
struct timeval curTime;
long passTime = 0;
void renderFrame() {

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    checkGlError("glBindFramebuffer");

    glClearColor(0.0, 0.0, 0.0, 0.0f);
    checkGlError("glClearColor");
    glClear( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    checkGlError("glClear");

    //直接绘制到屏幕上每有framebuffer 不能显示
    //如何 framebuffer 中的texture 绘制到screen 上面？
	realDraw();//绘制到当前显示屏幕上面

    //将当前framebuffer 的纹理 读取出来 写入到默认纹理里面
    //写入显卡的texture中
    drawSmallView();
    long diff = 0;
    if(begin == 0)
    {
        begin = 1;
        gettimeofday(&curTime, NULL);
    }
    else
    {
        struct timeval now;
        gettimeofday(&now, NULL);
        long diffsec = now.tv_sec-curTime.tv_sec;
        long diffusec = now.tv_usec-curTime.tv_usec;
        diff = diffsec*1000+diffusec/1000;

        curTime.tv_sec = now.tv_sec;
        curTime.tv_usec = now.tv_usec;
    }
    passTime += diff;
    /*
    if(passTime >= 40)
    {
        passTime -= 40;
        readAndStorePixel();
    }
    */

}
int frameCount = 0;
int MAX_FRAME = 40;
void readAndStorePixel()
{
    if(frameCount >= MAX_FRAME)
        return;
    frameCount++;
    glViewport(0, 0, width, height);

    char *buffer = fetchPixel();
    LOGI("bufferSize %d", buffer[0]);

    int i;
    int countRed = 0;
    int countGreen = 0;
    int newBufSize = width*height;
    LOGI("bufferSize 2 %d", newBufSize);
    for(i = 0; i < newBufSize; i++)
    {
        int r = buffer[i*4+0];
        int g = buffer[i*4+1];
        int b = buffer[i*4+2];
        if(r > 0)
            countRed++;
        if(g > 0)
            countGreen++;
    }
    LOGI("red green %d %d", countRed, countGreen);

    saveBmp(buffer);
    free(buffer);
    if(frameCount == MAX_FRAME)
        deleteVideo();
}



extern "C" {
    JNIEXPORT void JNICALL Java_com_liyong_libav_GL2JNILib_init(JNIEnv * env, jobject obj,  jint width, jint height, jbyteArray str);
    JNIEXPORT void JNICALL Java_com_liyong_libav_GL2JNILib_step(JNIEnv * env, jobject obj);
};

JNIEXPORT void JNICALL Java_com_liyong_libav_GL2JNILib_init(JNIEnv * env, jobject obj,  jint width, jint height, jbyteArray str)
{
    jniEnv = env;
    jbyte *arraybody = env->GetByteArrayElements(str, 0);
    jsize len = env->GetArrayLength(str);
    
    fileName = std::string((char*)arraybody, len);
    LOGI("curFileName %s", fileName.c_str());

    setupGraphics(width, height);
}

struct VideoContext {
    AVCodec *codec;
    AVCodecContext *c;
    FILE *f;
    int outbuf_size;
    uint8_t *outbuf;
    AVFrame *picture;
    uint8_t *picture_buf;
} videoCtx;

static void initVideo()
{
    printf("Video encoding\n");
    av_register_all();
    videoCtx.codec = avcodec_find_encoder(CODEC_ID_MPEG4);
    /* find the mpeg1 video encoder */
    if (!videoCtx.codec) {
        fprintf(stderr, "codec not found\n");
        return;
    }
    videoCtx.c = avcodec_alloc_context3(videoCtx.codec);

    /* put sample parameters */
    videoCtx.c->bit_rate = 400000;
    /* resolution must be a multiple of two */
    videoCtx.c->width = 640;
    videoCtx.c->height = 960;
    /* frames per second */
    videoCtx.c->time_base= (AVRational){1, 25};
    videoCtx.c->gop_size = 10; /* emit one intra frame every ten frames */
    videoCtx.c->max_b_frames=1;
    videoCtx.c->pix_fmt = PIX_FMT_YUV420P;

    /* open it */
    if (avcodec_open2(videoCtx.c, videoCtx.codec, NULL) < 0) {
        fprintf(stderr, "could not open codec\n");
        return;
    }

    LOGI("saveBmp %d %d", bufferSize);

    const char *fn = "/sdcard/";
    struct stat fs;
    int ret = stat(fn, &fs);
    LOGI("sdcard exit %d", ret);
    if(ret != ENOENT)
    {
        ret = stat("/sdcard/gl2", &fs);
        LOGI("mkdir %d", ret);
        if(ret != ENOENT)
        {
            int errorCode = mkdir("/sdcard/gl2", 0775);
            LOGI("mkdir error code %d ", errorCode);
            if(errorCode == -1)
            {
                LOGI("error %d %d %d", errno, EACCES, EEXIST);
            }
        }
        videoCtx.f = fopen("/sdcard/gl2/rgb.mp4", "wb");
        if (!videoCtx.f) {
            fprintf(stderr, "could not open %s\n", "/sdcard/gl2/rgb.mp4");
            exit(1);
        }

        videoCtx.outbuf_size = 500000;
        videoCtx.outbuf = (uint8_t*)malloc(videoCtx.outbuf_size);
        videoCtx.picture= avcodec_alloc_frame();

        int size = videoCtx.c->width*videoCtx.c->height;
        videoCtx.picture_buf = (uint8_t*)malloc((size * 3) / 2); /* size for YUV 420 */

        videoCtx.picture->data[0] = videoCtx.picture_buf;
        videoCtx.picture->data[1] = videoCtx.picture->data[0] + size;
        videoCtx.picture->data[2] = videoCtx.picture->data[1] + size / 4;
        //Y 解析度 是 U V 的两倍
        videoCtx.picture->linesize[0] = videoCtx.c->width;
        videoCtx.picture->linesize[1] = videoCtx.c->width / 2;
        videoCtx.picture->linesize[2] = videoCtx.c->width / 2;
    }
}
static void deleteVideo()
{
    /* get the delayed frames */
    /* add sequence end code to have a real mpeg file */
    videoCtx.outbuf[0] = 0x00;
    videoCtx.outbuf[1] = 0x00;
    videoCtx.outbuf[2] = 0x01;
    videoCtx.outbuf[3] = 0xb7;
    fwrite(videoCtx.outbuf, 1, 4, videoCtx.f);

    fclose(videoCtx.f);
    free(videoCtx.outbuf);
    avcodec_close(videoCtx.c);
    av_free(videoCtx.c);
    free(videoCtx.picture_buf);
    av_free(videoCtx.picture);
}
//每秒20帧
static void videoEncode(char *rgbbuffer)
{
    AVCodec *codec = videoCtx.codec;
    AVCodecContext *c= videoCtx.c;
    int i, out_size, size, x, y, outbuf_size;
    FILE *f;
    AVFrame *picture;
    uint8_t *outbuf, *picture_buf;


    /* alloc image and output buffer */
    outbuf_size = videoCtx.outbuf_size;
    outbuf = videoCtx.outbuf;
    size = c->width * c->height;
    
    int totalLen = c->width*c->height;
    int countGreen = 0;
    int redCount = 0;
    printf("find Red\n");
    for(i = 0; i < totalLen; i++)
    {
        int r, g, b;
        r = rgbbuffer[i*4+0];
        g = rgbbuffer[i*4+1];
        b = rgbbuffer[i*4+2];
        if(r > 0)
            redCount++;
        if(g > 0)
            countGreen++;
    }
    printf("read Green %d %d\n",  redCount, countGreen);

    picture = videoCtx.picture;
    picture_buf = videoCtx.picture_buf;
    //Y 4
    //U 2
    //V 2
    
    //transfer rgb -> yuv
    /* encode 1 second of video */
    //420 格式
    fflush(stdout);
    /* prepare a dummy image */
    /* Y */
    for(y=0;y<c->height;y++) {
        for(x=0;x<c->width;x++) {
            int r, g, b;
            r = rgbbuffer[4*(y*c->width+x)+0];
            g = rgbbuffer[4*(y*c->width+x)+1];
            b = rgbbuffer[4*(y*c->width+x)+2];
            picture->data[0][y*picture->linesize[0]+x] = RGB_TO_Y_CCIR(r, g, b);
        }
    }

    //2 * 2平均值0 1
    //           2 3
    /* Cb and Cr */
    for(y=0;y<c->height/2;y++) {
        for(x=0;x<c->width/2;x++) {
            int r, g, b;
            r = rgbbuffer[4*(y*2*c->width+x*2)+0];
            g = rgbbuffer[4*(y*2*c->width+x*2)+1];
            b = rgbbuffer[4*(y*2*c->width+x*2)+2];

            picture->data[1][y * picture->linesize[1] + x] = RGB_TO_U_CCIR(r, g, b, 0);
            picture->data[2][y * picture->linesize[2] + x] = RGB_TO_V_CCIR(r, g, b, 0);
        }
    }

    /* encode the image */
    out_size = avcodec_encode_video(c, outbuf, outbuf_size, picture);
    printf("encoding frame %3d (size=%5d)\n", i, out_size);
    fwrite(outbuf, 1, out_size, f);

}

/*
保存数据到本地的一个二进制文件中
*/
void saveBmp(char *buffer)
{
    //videoEncode(buffer);
    /*
    LOGI("saveBmp %d %d", bufferSize);

    const char *fn = "/sdcard/";
    struct stat fs;
    int ret = stat(fn, &fs);
    LOGI("sdcard exit %d", ret);
    if(ret != ENOENT)
    {
        ret = stat("/sdcard/gl2", &fs);
        LOGI("mkdir %d", ret);
        if(ret != ENOENT)
        {
            int errorCode = mkdir("/sdcard/gl2", 0775);
            LOGI("mkdir error code %d ", errorCode);
            if(errorCode == -1)
            {
                LOGI("error %d %d %d", errno, EACCES, EEXIST);
            }
        }
        FILE *f = fopen("/sdcard/gl2/save.bin", "w");
        LOGI("save bin %x", f);
        if(f != NULL)
        {
            fwrite(buffer, sizeof(char), bufferSize, f);
            fclose(f);
        }
    }
    */
}   

int renderYet = 0;
JNIEXPORT void JNICALL Java_com_liyong_libav_GL2JNILib_step(JNIEnv * env, jobject obj)
{
    //if(!renderYet)
    {
        //LOGI("renderYet %d", renderYet);
        renderFrame();
        renderYet = 1;
    }
}
