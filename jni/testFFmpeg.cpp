#include <jni.h>
#include <android/log.h>
#include <stdio.h>
#include <dlfcn.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include "yuv420.h"
#include <sys/time.h>

float recordTime = 10;
float frameRate = 1./30;
bool startYet = false;
int width;
int height;
float passTime = 1./30;
float totalTime = 0;
uint8_t *pixelBuffer;
AVFrame *picture;
uint8_t *outbuf;
uint8_t *picture_buf;
int outbuf_size;

AVFormatContext *oc;
AVOutputFormat *fmt;
AVStream *video_st;
double video_pts;



extern "C" {
    #include "libavcodec/avcodec.h"
    #include "libavformat/avformat.h"
    #include "libavutil/colorspace.h"
};



JNIEnv *jniEnv;
extern "C" {
    JNIEXPORT void JNICALL Java_com_liyong_libav_GL2JNILib_init(JNIEnv * env, jobject obj,  jint width, jint height, jbyteArray str);
    JNIEXPORT void JNICALL Java_com_liyong_libav_GL2JNILib_step(JNIEnv * env, jobject obj);
};


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
    "  gl_FragColor = vec4(0.0, 1.0, 0.0, 1.0);\n"
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
    GLuint vertexShader = loadShader(GL_VERTEX_SHADER, pVertexSource);
    if (!vertexShader) {
        return 0;
    }

    GLuint pixelShader = loadShader(GL_FRAGMENT_SHADER, pFragmentSource);
    if (!pixelShader) {
        return 0;
    }

    GLuint program = glCreateProgram();
    if (program) {
        glAttachShader(program, vertexShader);
        checkGlError("glAttachShader");
        glAttachShader(program, pixelShader);
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
    return program;
}



GLuint gProgram;
GLuint gvPositionHandle;

bool setupGraphics(int w, int h) {
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

    glViewport(0, 0, w, h);
    checkGlError("glViewport");
    return true;
}

const GLfloat gTriangleVertices[] = { 0.0f, 0.5f, -0.5f, -0.5f,
        0.5f, -0.5f };

void renderFrame() {
    static float grey;
    grey += 0.01f;
    if (grey > 1.0f) {
        grey = 0.0f;
    }
    glClearColor(grey, grey, grey, 1.0f);
    checkGlError("glClearColor");
    glClear( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    checkGlError("glClear");

    glUseProgram(gProgram);
    checkGlError("glUseProgram");

    glVertexAttribPointer(gvPositionHandle, 2, GL_FLOAT, GL_FALSE, 0, gTriangleVertices);
    checkGlError("glVertexAttribPointer");
    glEnableVertexAttribArray(gvPositionHandle);
    checkGlError("glEnableVertexAttribArray");
    glDrawArrays(GL_TRIANGLES, 0, 3);
    checkGlError("glDrawArrays");
}


void startWork(int w, int h) {
    width = w;
    height = h;
    startYet = true;
    frameRate = 1./25;
    passTime = 1./25;
    totalTime = 0;

    av_register_all();

    pixelBuffer = (uint8_t *)malloc(sizeof(int)*width*height);
    if(!pixelBuffer) {
        printf("could not pixelBuffer\n");
        exit(1);
    }

    const char *fileName = "/sdcard/gl2/myGame.mp4";
    
    fmt = av_guess_format(NULL, fileName, NULL);
    if(!fmt) {
        LOGI("guess fmt error %s\n", fileName);
        exit(1);
    }

    printf("alloc context \n");
    oc = avformat_alloc_context();
    oc->oformat = fmt;
    snprintf(oc->filename, sizeof(oc->filename), "%s", fileName);
    printf("add_video_stream \n");
    video_st = add_video_stream(oc, fmt->video_codec, width, height);
    if(av_set_parameters(oc, NULL) < 0) {
        printf("Invalid output format parameters\n");
    }
    av_dump_format(oc, 0, fileName, 1);
    LOGI("open_video");

    open_video(oc, video_st, &outbuf, &outbuf_size, (AVPicture **)&picture);
    //需要输出文件
    if(!(fmt->flags & AVFMT_NOFILE)) {
        if(avio_open(&oc->pb, fileName, AVIO_FLAG_WRITE) < 0) {
            LOGI("could not open %s " , fileName);
        } 
    }

    av_write_header(oc);
}
void stopWork() {

    av_write_trailer(oc);


    LOGI("stopWork");
    avcodec_close(video_st->codec);
    av_free(picture->data[0]);
    av_free(picture);
    av_free(outbuf);

    unsigned int i;
    for(i = 0; i < oc->nb_streams; i++) {
        av_freep(&oc->streams[i]->codec);
        av_freep(&oc->streams[i]);
    }
    if(!(fmt->flags & AVFMT_NOFILE))
        avio_close(oc->pb);
    av_free(oc);


    free(pixelBuffer);

    outbuf = NULL;
    picture = NULL;
    startYet = false;

}
int frameCount = 0;
void compressCurrentFrame() {
    AVCodecContext *c = video_st->codec;
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixelBuffer);    

    SwsContext *sctx = sws_getCachedContext (NULL, width, height, PIX_FMT_RGBA, width, height, PIX_FMT_YUV420P, SWS_FAST_BILINEAR, NULL, NULL, NULL);

    //使用不同的算法进行压缩
    int inLineSize[] = {width*4, 0, 0, 0};
    uint8_t *srcSlice[] = {pixelBuffer, NULL, NULL};

    int retCode = sws_scale (sctx, srcSlice, inLineSize, 0, height, picture->data, picture->linesize);
    LOGI("sws_scale %d", retCode);
    sws_freeContext(sctx);

    video_pts = (double)video_st->pts.val*video_st->time_base.num/video_st->time_base.den;
    LOGI("video_pts %lf", video_pts);

    int out_size = avcodec_encode_video(c, outbuf, outbuf_size, picture);
    
    int ret;
    if(out_size > 0) {
        AVPacket pkt;
        av_init_packet(&pkt);
        //编码器 需要PTS 需要在 codec 的timebase 和 stream的 timebase之间转化
        if(c->coded_frame->pts != AV_NOPTS_VALUE) {
            pkt.pts = av_rescale_q(c->coded_frame->pts, c->time_base, video_st->time_base);
        }
        //当前是关键帧
        if(c->coded_frame->key_frame) {
            pkt.flags |= AV_PKT_FLAG_KEY;
        }
        pkt.stream_index = video_st->index;
        pkt.data = outbuf;
        pkt.size = out_size;
        
        //将包数据写入 输出文件
        ret = av_interleaved_write_frame(oc, &pkt);
    } else {
        ret = 0;
    }
    LOGI("encode %d", ret);

    frameCount++;
}

double now = 0;
JNIEXPORT void JNICALL Java_com_liyong_libav_GL2JNILib_init(JNIEnv * env, jobject obj,  jint w, jint h, jbyteArray str)
{
    LOGI("init testFFmpeg"); 
    jniEnv = env;
    setupGraphics(w, h);
    struct timeval t;
    gettimeofday(&t, NULL);

    LOGI("startWork");
    startWork(w, h);
}

void update() {
    float dt;
    double lastTime = now;
    struct timeval t;
    gettimeofday(&t, NULL);
    if(now == 0) {
        double result = t.tv_usec;
        result *= 0.000001;
        result += (double) t.tv_sec;
        now = result;
        dt = 0;
        
    } else {

        double result = t.tv_usec;
        result *= 0.000001;
        result += (double) t.tv_sec;
        now = result;
        dt = now-lastTime;
    }



    if(startYet) {
        LOGI("passTime %lf %lf %lf %d %d", dt, now, lastTime, t.tv_sec, t.tv_usec);
        if(totalTime < recordTime) { 
            totalTime += dt;
            passTime += dt;
            if(passTime >= frameRate) {
                LOGI("update %f %f %f %f", totalTime, recordTime, passTime, frameRate);
                passTime -= frameRate;
                compressCurrentFrame();
            }
        } else {
            startYet = false;
            LOGI("finish record");
            stopWork();// 结束需要停止工作
        }
    }
}

JNIEXPORT void JNICALL Java_com_liyong_libav_GL2JNILib_step(JNIEnv * env, jobject obj)
{
    renderFrame();
    update();
}


