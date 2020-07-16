//
// Created by tpson on 2020/7/15.
//
#include <cstring>
#include "pty.h"
#include "AudioChannel.h"
#include "faac.h"
#include "librtmp/rtmp.h"
#include "macro.h"

//编码音频信息
void AudioChannel::encodeData(int8_t *data) {
    //编码
    int bytelen = faacEncEncode(audioCodec, reinterpret_cast<int32_t *>(data), inputSamples, buffer,
                                maxOutputBytes);
    if (bytelen > 0) {
        LOGE("音频编码");
        RTMPPacket *packet=new RTMPPacket;
        //编码规范,前两位是0xAF和0x01 所以size大小是bytelen+2
        int bodySize = 2+bytelen;
        RTMPPacket_Alloc(packet,bodySize);
        //编码规范
        packet->m_body[0]=0xAF;
        packet->m_body[1]=0x01;
        if (mChannels == 1) {
            packet->m_body[0] = 0xAE;
        }
        //编码后的aac数据,数据大小不固定,不包含前两位固定格式
        memcpy(&packet->m_body[2],buffer,bytelen);

        //相对时间
        packet->m_hasAbsTimestamp=0;
        //数据包大小,包含前两位固定编码
        packet->m_nBodySize=bodySize;
        //数据格式,音频格式
        packet->m_packetType=RTMP_PACKET_TYPE_AUDIO;
        //频道可以随便给,只要不和系统重复就ok 视频中给的是0x10
        packet->m_nChannel=0x11;

        packet->m_headerType=RTMP_PACKET_SIZE_LARGE;
        audioCallback(packet);
    }
}

void AudioChannel::setAudioCallback(AudioChannel::AudioCallback audioCallback) {
    this->audioCallback = audioCallback;
}

//设置编码器信息
void AudioChannel::setAudioEncIfno(int simplesInSize, int channels) {
    //打开编码器
    audioCodec = faacEncOpen(simplesInSize, channels, &inputSamples, &maxOutputBytes);
    //设置参数
    faacEncConfigurationPtr config = faacEncGetCurrentConfiguration(audioCodec);
    //版本
    config->mpegVersion = MPEG4;
    //lc 标准
    config->aacObjectType = LOW;
    //16位
    config->inputFormat = FAAC_INPUT_16BIT;
    // 编码出原始数据 既不是adts也不是adif
    config->outputFormat = 0;
    faacEncSetConfiguration(audioCodec, config);
    buffer = new u_char[maxOutputBytes];
}

int AudioChannel::getInputSamples() {
    return inputSamples;
}
