//
// Created by tpson on 2020/7/10.
//
#include <stdint.h>
#include <x264.h>
#include <pty.h>
#include <cstring>
#include <jni.h>
#include "VideoChannel.h"
#include "librtmp/rtmp.h"
#include "macro.h"
#include "include/x264.h"

void VideoChannel::sendFrame(int type, uint8_t *payload, int i_payload) {
    if (payload[2] == 0x00) {
        i_payload -= 4;
        payload += 4;
    } else {
        i_payload -= 3;
        payload += 3;
    }
    //看表
    int bodySize = 9 + i_payload;
    RTMPPacket *packet = new RTMPPacket;
    //
    RTMPPacket_Alloc(packet, bodySize);

    packet->m_body[0] = 0x27;
    if(type == NAL_SLICE_IDR){
        packet->m_body[0] = 0x17;
        LOGE("关键帧");
    }
    //类型
    packet->m_body[1] = 0x01;
    //时间戳
    packet->m_body[2] = 0x00;
    packet->m_body[3] = 0x00;
    packet->m_body[4] = 0x00;
    //数据长度 int 4个字节
    packet->m_body[5] = (i_payload >> 24) & 0xff;
    packet->m_body[6] = (i_payload >> 16) & 0xff;
    packet->m_body[7] = (i_payload >> 8) & 0xff;
    packet->m_body[8] = (i_payload) & 0xff;

    //图片数据
    memcpy(&packet->m_body[9], payload, i_payload);

    packet->m_hasAbsTimestamp = 0;
    packet->m_nBodySize = bodySize;
    packet->m_packetType = RTMP_PACKET_TYPE_VIDEO;
    packet->m_nChannel = 0x10;
    packet->m_headerType = RTMP_PACKET_SIZE_LARGE;
    videoCallback(packet);
}

void VideoChannel::setVideoEncInfo(jint width, jint height, jint fps, jint bitrate) {

    //初始化--------------
    mWidth = width;
    mHeight = height;
    mFps = fps;
    mBitrate = bitrate;
    ySize = width * height;
    uvSize = ySize / 4;
    //初始化x264的编码器
    x264_param_t param;
    //ultrafast 编码最快   zerolatency零延迟
    x264_param_default_preset(&param, "ultrafast", "zerolatency");
    //编码复杂度,一般是32
    param.i_level_idc = 32;
    //输入数据格式,Android手机都是NV21,这里转成I420速度快(这里是初始化,还没有转换),服务器一般都支持I420
    param.i_csp = X264_CSP_I420;
    //宽高
    param.i_height = height;
    param.i_width = width;

    //无B帧,首开,直播一般都是0,这个表示B帧的编码数量,每个多长时间编码B帧
    param.i_bframe = 0;
    //参数i_rc_method表示码率控制，CQP(恒定质量)， CRF(恒定码率), ABR(平均码率)
    param.rc.i_rc_method = X264_RC_ABR;
    //码率(比特率,单位Kbps)
    param.rc.i_bitrate = bitrate / 1000;
    //瞬时最大码率
    param.rc.i_vbv_max_bitrate = bitrate / 1000 * 1.2;
    //设置了i_vbv_max_bitrate必须设置此参数，码率控制区大小，单位kbps
    param.rc.i_vbv_max_bitrate = bitrate / 1000;

    //帧率的分子
    param.i_fps_num = fps;
    //帧率的分母 在音视频中关于时间的参数都有个分母和时间基
    param.i_fps_den = 1;
    //时间基的分子 主要是为了做音视频同步使用i_timebase_den/i_timebase_num得到的是每一帧的时间
    param.i_timebase_num = param.i_fps_num;
    //时间基的分母
    param.i_timebase_den = param.i_fps_den;

    //用fps计算帧间距离,而不是时间戳
    param.b_vfr_input = 0;
    //帧距离 每两秒一个关键帧
    param.i_keyint_max = fps * 2;
    //是否复制sps和pps放在每个关键帧的前面 该参数设置是让每个关键帧(I帧)前面都附带上sps/pps
    param.b_repeat_headers = 1;
    //多线程 0自动开启多线程,1表示单线程
    param.i_threads = 1;
    //编码质量
    x264_param_apply_profile(&param, "baseline");
    //开启编码器
    videoCodec = x264_encoder_open(&param);
    //临时存储
    in_pic= new x264_picture_t;
    //声明in_pic一帧的空间
    x264_picture_alloc(in_pic,X264_CSP_I420,width,height);

    //-----------------------------------------
    //编码

}
//设置回调
void VideoChannel::setVideoCallback(VideoCallback  videoCallback) {
    this->videoCallback = videoCallback;
}

void VideoChannel::encodeData(int8_t *data) {
    //解码 需要将NV21的格式转换成I420
    //数据data 容器in_pic(一帧)
    //将y数据 存放在第0个位置
    memcpy(in_pic->img.plane[0],data,ySize);
    for (int i = 0; i < uvSize; ++i) {
        //uv数据
        *(in_pic->img.plane[1] + i) = *(data + ySize + i*2 + 1);//奇数位存放的是u数据
        *(in_pic->img.plane[2] + i) = *(data + ySize + i*2 );//偶数位存放的是v数据
    }
    //NALU单元数组
    x264_nal_t *pp_nal;
    //编码出来有多少数据(NALU单元)
    int pi_nal;
    //编码后的一帧数据
    x264_picture_t out_pic;
    x264_encoder_encode(videoCodec,&pp_nal,&pi_nal,in_pic,&out_pic);
    int sps_len;
    int pps_len;
    uint8_t sps[100];
    uint8_t pps[100];
    for (int i = 0; i <pi_nal ; ++i) {
        //关键帧的NALU中携带的编码信息 单独发送sps和pps数据
        if(pp_nal[i].i_type==NAL_SPS){
            //sps数据的实际长度等于i_payload的长度-4 因为sps和pps的数据前有00 00 00 01(或者00 00 01)的十六进制标志位分隔,标志位占四个字节
            sps_len=pp_nal[i].i_payload-4;
            //copy数据
            memcpy(sps, pp_nal[i].p_payload + 4, sps_len);
        } else if(pp_nal[i].i_type==NAL_PPS){
            //同上 取的是pps数据
            pps_len=pp_nal[i].i_payload-4;
            memcpy(pps, pp_nal[i].p_payload + 4, pps_len);
            sendSpsPps(sps,pps,sps_len,pps_len);
        } else{
            //发送关键帧和非关键帧数据
            sendFrame(pp_nal[i].i_type, pp_nal[i].p_payload, pp_nal[i].i_payload);
        }
    }
}

//发送sps和pps数据 格式要符合RTMP的格式 放在Packet载体中发送
void VideoChannel::sendSpsPps(uint8_t *sps, uint8_t *pps, int sps_len, int pps_len) {
    //sps pps数据封装到Packet中,这里要参考Rtmp的封装格式
    int bodySize = 13 + sps_len + 3 + pps_len;
    RTMPPacket *packet = new RTMPPacket;
    RTMPPacket_Alloc(packet, bodySize);
    //已下参照Rtmp的封装格式,大多为固定格式,相当于是编解码的协议,需遵循
    int i = 0;
    //固定头
    packet->m_body[i++] = 0x17;
    //类型
    packet->m_body[i++] = 0x00;
    //composition time 0x000000
    packet->m_body[i++] = 0x00;
    packet->m_body[i++] = 0x00;
    packet->m_body[i++] = 0x00;

    //版本
    packet->m_body[i++] = 0x01;
    //编码规格
    packet->m_body[i++] = sps[1];
    packet->m_body[i++] = sps[2];
    packet->m_body[i++] = sps[3];
    packet->m_body[i++] = 0xFF;

    //整个sps
    packet->m_body[i++] = 0xE1;
    //sps长度
    packet->m_body[i++] = (sps_len >> 8) & 0xff;
    packet->m_body[i++] = sps_len & 0xff;
    memcpy(&packet->m_body[i], sps, sps_len);
    i += sps_len;

    //pps
    packet->m_body[i++] = 0x01;
    packet->m_body[i++] = (pps_len >> 8) & 0xff;
    packet->m_body[i++] = (pps_len) & 0xff;
    memcpy(&packet->m_body[i], pps, pps_len);
    //流的格式(视频流)
    packet->m_packetType=RTMP_PACKET_TYPE_VIDEO;
    packet->m_nBodySize = bodySize;
    //随意分配一个管道（尽量避开rtmp.c中使用的）
    packet->m_nChannel = 10;
    //sps pps没有时间戳
    packet->m_nTimeStamp = 0;
    //不使用绝对时间
    packet->m_hasAbsTimestamp = 0;
    packet->m_headerType = RTMP_PACKET_SIZE_MEDIUM;
    if (videoCallback){
        videoCallback(packet);
    }
}
