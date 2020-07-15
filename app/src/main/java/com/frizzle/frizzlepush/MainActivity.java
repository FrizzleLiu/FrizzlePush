package com.frizzle.frizzlepush;

import androidx.appcompat.app.AppCompatActivity;

import android.Manifest;
import android.content.pm.PackageManager;
import android.hardware.Camera;
import android.os.Build;
import android.os.Bundle;
import android.view.SurfaceView;
import android.view.View;
import android.widget.TextView;

public class MainActivity extends AppCompatActivity {

    private LivePusher livePusher;
    private SurfaceView surfaceView;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        surfaceView = findViewById(R.id.surfaceView);
        livePusher = new LivePusher(this, 800, 480, 800_000, 10, Camera.CameraInfo.CAMERA_FACING_BACK);
        //  设置摄像头预览的界面
        requestPerms();
    }

    private void requestPerms() {
        //权限,简单处理下
        if (Build.VERSION.SDK_INT>Build.VERSION_CODES.N) {
            String[] perms= {Manifest.permission.CAMERA,Manifest.permission.WRITE_EXTERNAL_STORAGE,Manifest.permission.RECORD_AUDIO};
            if (checkSelfPermission(perms[0]) == PackageManager.PERMISSION_DENIED) {
                requestPermissions(perms,200);
            }else {
                livePusher.setPreviewDisplay(surfaceView.getHolder());
            }
        }
    }

    public void switchCamera(View view) {
    }

    public void startLive(View view) {
        livePusher.startLive("rtmp://8.210.126.235/myapp");
    }

    public void stopLive(View view) {
    }
}
