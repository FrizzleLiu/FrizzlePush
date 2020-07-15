//
// Created by tpson on 2020/7/10.
//

#ifndef FRIZZLEPUSH_VIDEOCHANNEL_H
#define FRIZZLEPUSH_VIDEOCHANNEL_H
#include "librtmp/rtmp.h"

class VideoChannel {
    typedef void (*VideoCallback)(RTMPPacket* packet);
public:
    void setVideoEncInfo(jint width, jint height, jint fps, jint bitrate);

    void encodeData(int8_t *data);

    void setVideoCallback(VideoCallback videoCallback);
private:
    int mWidth;
    int mHeight;
    int mFps;
    int mBitrate;
    int ySize;
    int uvSize;
    //开启编码器的返回参数,编码器
    x264_t *videoCodec;
    //代表一帧,为了编码过程中临时存储数据
    x264_picture_t *in_pic;
    //编解码回调
    VideoCallback videoCallback;
    void  sendFrame(int type, uint8_t *payload, int i_payload);
    void sendSpsPps(uint8_t sps[100], uint8_t pps[100], int len, int pps_len);
};


#endif //FRIZZLEPUSH_VIDEOCHANNEL_H
