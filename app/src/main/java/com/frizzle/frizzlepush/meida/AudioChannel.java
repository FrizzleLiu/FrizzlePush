package com.frizzle.frizzlepush.meida;


import android.media.AudioFormat;
import android.media.AudioRecord;
import android.media.MediaRecorder;
import android.util.Log;

import com.frizzle.frizzlepush.LivePusher;

import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class AudioChannel {
    private LivePusher mLivePusher;
    private AudioRecord audioRecord;
    private int inputSamples;
    private int channels = 1;
    int channelConfig;
    int minBufferSize;
    private ExecutorService executor;
    private boolean isLiving;
    public AudioChannel(LivePusher livePusher) {
        executor = Executors.newSingleThreadExecutor();
        mLivePusher = livePusher;
        //采样通道数
        if (channels == 2) {
            channelConfig = AudioFormat.CHANNEL_IN_STEREO;
        } else {
            channelConfig = AudioFormat.CHANNEL_IN_MONO;
        }
        //设置音频编码器信息
        mLivePusher.native_setAudioEncInfo(44100, channels);
        //16 位 2个字节
        inputSamples = mLivePusher.getInputSamples() * 2;
        //        minBufferSize 音频信息的缓冲区大小和faac音频编码器的冲区大小做比较,取小的,因为空数据也会播放音频
        minBufferSize=  AudioRecord.getMinBufferSize(44100,
                channelConfig, AudioFormat.ENCODING_PCM_16BIT)*2;
        Log.e("minBufferSize",minBufferSize+"");
        Log.e("inputSamples",inputSamples+"");
        audioRecord = new AudioRecord(MediaRecorder.AudioSource.MIC, 8000, channelConfig,
                AudioFormat.ENCODING_PCM_16BIT, minBufferSize < inputSamples ? inputSamples: minBufferSize
        );
        Log.e("audioRecord",audioRecord.getState()+"");
    }
    public void startLive() {
        isLiving = true;
        executor.submit(new AudioTeask());
    }



    public void setChannels(int channels) {
        this.channels = channels;
    }

    public void release() {
        audioRecord.release();
    }

    //开启线程推送音频信息
    class AudioTeask implements Runnable {
        @Override
        public void run() {
            audioRecord.startRecording();
            //    pcm  音频原始数据
            byte[] bytes = new byte[inputSamples];
            Log.e("isLiving",isLiving+"");
            while (isLiving) {
                int len = audioRecord.read(bytes, 0, bytes.length);
                mLivePusher.native_pushAudio(bytes);
                Log.e("音频","采集音频数据调用native推流");
            }
        }
    }
}
