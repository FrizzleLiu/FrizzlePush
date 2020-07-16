//
// Created by tpson on 2020/7/15.
//

#ifndef FRIZZLEPUSH_AUDIOCHANNEL_H
#define FRIZZLEPUSH_AUDIOCHANNEL_H


#include <cstdint>
#include <jni.h>
#include <faac.h>
#include "librtmp/rtmp.h"

class AudioChannel {
    typedef void (*AudioCallback)(RTMPPacket *packet);
public:
    void encodeData(int8_t *data);
    void setAudioEncIfo(int simplesInSize,int channels);
    jint getInputSamples();
    void setAudioCallback(AudioCallback audioCallback);
private:
    AudioCallback audioCallback;
    int mChannels;
    faacEncHandle audioCodec;
    u_long inputSamples;
    u_long maxOutputBytes;
    u_char *buffer = 0;
};

#endif //FRIZZLEPUSH_AUDIOCHANNEL_H
