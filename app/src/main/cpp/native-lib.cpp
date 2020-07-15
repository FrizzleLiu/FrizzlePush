#include <jni.h>
#include <string>
#include "x264.h"
#include "librtmp/rtmp.h"
#include "VideoChannel.h"
#include "macro.h"
#include "safe_queue.h"

VideoChannel *videoChannel;
//开启线程的标志位,防止多次调用
int isStart = 0;
//线程
pthread_t pid;
//线程回调的函数,相当于Java中的Thread中的run()方法,线程开启成功url会传递到这里,void *args可以接受任意类型的参数
//开始时间,用于音视频同步
uint32_t start_time;
//表示开始推流
int readyPushing = 0;
//数据包队列
SafeQueue<RTMPPacket *> packets;
//编解码回调函数
void callback(RTMPPacket *packet) {
    if (packet) {
        //设置时间戳
        packet->m_nTimeStamp = RTMP_GetTime() - start_time;
        //加入队列
        packets.put(packet);
    }
}

//发送数据包后,释放数据包
void releasePacket(RTMPPacket *&packet) {
    if (packet) {
        RTMPPacket_Free(packet);
        delete (packet);
        packet = 0;
    }
}

void *start(void *args) {
    char *url = static_cast<char *>(args);
    RTMP *rtmp = 0;
    rtmp = RTMP_Alloc();
    if (!rtmp) {
        LOGE("RTMP_Alloc 失败");
        return NULL;
    }
    //初始化
    RTMP_Init(rtmp);
    //设置URl地址
    int resultCode = RTMP_SetupURL(rtmp, url);
    if (!resultCode) {
        LOGE("RTMP 设置Url地址失败: %s", url);
        return NULL;
    }
    rtmp->Link.timeout = 5;
    //设置Rtmp可写
    RTMP_EnableWrite(rtmp);
    //连接服务器,native层socket实现,第二个参数是数据包,可用于连接测试
    resultCode = RTMP_Connect(rtmp, 0);
    if (!resultCode) {
        LOGE("RTMP 连接服务器失败: %s", url);
        return NULL;
    }
    //连接流
    resultCode = RTMP_ConnectStream(rtmp, 0);
    if (!resultCode) {
        LOGE("RTMP 连接流失败: %s", url);
        return NULL;
    }
    //开始推流时间
    start_time = RTMP_GetTime();
    readyPushing = 1;
    packets.setWork(1);
    RTMPPacket *packet = 0;
    //循环推送
    while (readyPushing) {
        //队列中取数据packets
        packets.get(packet);
        LOGE("取出一帧数据");
        if (!readyPushing) {
            break;
        }
        if (!packet) {
            continue;
        }
        //当前的流的类型,(音频或视频)
        packet->m_nInfoField2 = rtmp->m_stream_id;
        //最后一个参数表示RTMP内部缓存一个队列,处理如网络延迟等情况
        resultCode = RTMP_SendPacket(rtmp, packet, 1);
        //发送完毕释放packet
        releasePacket(packet);
    }

    //重置标志位,释放资源
    isStart = 0;
    readyPushing = 0;
    packets.setWork(0);
    packets.clear();
    if (rtmp) {
        RTMP_Close(rtmp);
        RTMP_Free(rtmp);
    }
    delete (url);
    return  0;
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_frizzle_frizzlepush_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    x264_picture_t *x264_picture = new x264_picture_t;
    RTMP_Alloc();
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}

extern "C"
JNIEXPORT void JNICALL
Java_com_frizzle_frizzlepush_LivePusher_native_1init(JNIEnv *env, jobject thiz) {
    videoChannel = new VideoChannel;
    videoChannel->setVideoCallback(callback);
}

//初始化一些参数,宽高,fps,码率(bit率)
extern "C"
JNIEXPORT void JNICALL
Java_com_frizzle_frizzlepush_LivePusher_native_1setVideoEncInfo(JNIEnv *env, jobject thiz,
                                                                jint width, jint height, jint fps,
                                                                jint bitrate) {
    if (!videoChannel) {
        return;
    }
    videoChannel->setVideoEncInfo(width, height, fps, bitrate);
}

//开启推流
extern "C"
JNIEXPORT void JNICALL
Java_com_frizzle_frizzlepush_LivePusher_native_1start(JNIEnv *env, jobject thiz, jstring rtmp_url) {
    const char *path = env->GetStringUTFChars(rtmp_url, 0);
    if (isStart) {
        return;
    }
    isStart = 1;
    //将path的内容复制到url中,防止在线程运行过程中path被释放销毁
    char *url = new char[strlen(path) + 1];
    strcpy(url, path);

    //开启线程,开启成功url参数会传到start函数中
    pthread_create(&pid, 0, start, url);


    env->ReleaseStringUTFChars(rtmp_url, path);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_frizzle_frizzlepush_LivePusher_native_1pushVideo(JNIEnv *env, jobject thiz,
                                                          jbyteArray data_) {
    if (!videoChannel || !readyPushing) {
        return;
    }
    jbyte *data = env->GetByteArrayElements(data_, NULL);

    videoChannel->encodeData(data);

    env->ReleaseByteArrayElements(data_, data, 0);
}