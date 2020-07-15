package com.frizzle.frizzlepush.meida;

import android.app.Activity;
import android.hardware.Camera;
import android.view.SurfaceHolder;

import com.frizzle.frizzlepush.LivePusher;


public class VideoChannel implements Camera.PreviewCallback, CameraHelper.OnChangedSizeListener {
    private CameraHelper cameraHelper;
    private int mBitrate;
    private int mFps;
    private boolean isLiving;
    private LivePusher mLivePusher;

    public VideoChannel(LivePusher livePusher, Activity activity, int width, int height, int bitrate, int fps, int cameraId) {
        mLivePusher = livePusher;
        mBitrate = bitrate;
        mFps = fps;
        cameraHelper = new CameraHelper(activity, cameraId, width, height);
        cameraHelper.setPreviewCallback(this);
        cameraHelper.setOnChangedSizeListener(this);
    }

    @Override
    public void onPreviewFrame(byte[] data, Camera camera) {
        //data Android端摄像头的视频流都是NV21 需要转换为I420格式
        if (isLiving) {
            mLivePusher.native_pushVideo(data);
        }
    }

    @Override
    public void onChanged(int w, int h) {
        mLivePusher.native_setVideoEncInfo(w, h, mFps, mBitrate);
    }

    public void switchCamera() {
        cameraHelper.switchCamera();
    }

    public void setPreviewDisplay(SurfaceHolder surfaceHolder) {
        cameraHelper.setPreviewDisplay(surfaceHolder);
    }

    public void startLive() {
        isLiving = true;
    }
}
