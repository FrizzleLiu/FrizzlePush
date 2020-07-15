package com.frizzle.frizzlepush;

import android.app.Activity;
import android.view.SurfaceHolder;

import com.frizzle.frizzlepush.meida.AudioChannel;
import com.frizzle.frizzlepush.meida.VideoChannel;


public class LivePusher {
    private AudioChannel audioChannel;
    private VideoChannel videoChannel;
    static {
        System.loadLibrary("native-lib");
    }

    public LivePusher(Activity activity, int width, int height, int bitrate,
                      int fps, int cameraId) {
        native_init();
        videoChannel = new VideoChannel(this, activity, width, height, bitrate, fps, cameraId);
        audioChannel = new AudioChannel(this);
    }
    public void setPreviewDisplay(SurfaceHolder surfaceHolder) {
        videoChannel.setPreviewDisplay(surfaceHolder);
    }
    public void switchCamera() {
        videoChannel.switchCamera();
    }

    /**
     * @param rtmpUrl 服务端推流地址
     *                开启推流
     */
    public void startLive(String rtmpUrl) {
        native_start(rtmpUrl);
        videoChannel.startLive();
    }


    public native void native_init();
    public native void native_setVideoEncInfo(int width,int height,int fps,int bitrate);
    public native void native_start(String rtmpUrl);
    public native void native_pushVideo(byte[] data);
}
