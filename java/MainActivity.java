package com.example.apisource;

import android.annotation.TargetApi;
import android.app.AlertDialog;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.content.DialogInterface;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.support.annotation.NonNull;
import android.support.v4.app.ActivityCompat;
import android.support.v4.content.ContextCompat;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.widget.TextView;

import java.io.IOException;
import java.io.PipedInputStream;
import java.io.PipedOutputStream;
import java.util.Set;

import static android.Manifest.permission.READ_EXTERNAL_STORAGE;
import static android.Manifest.permission.WRITE_EXTERNAL_STORAGE;
import static com.example.apisource.apithread.apiout;
import static java.lang.Boolean.FALSE;

public class MainActivity extends AppCompatActivity {

    static PipedOutputStream pos = null; //pipes used for ipc
    static PipedInputStream pis = null;
    ConnectThread ConThread;

    public static volatile int received = 0; //flag representing if the gesture detected has been received by the application thread
    public static volatile int poll = 0;
    int apiinit = 0; //goes to 1 if all threads involving api have been started
    Handler h;
    Runnable r;

    private static final int PERMISSION_REQUEST_CODE = 200;
    protected boolean checkPermission(){
        return ContextCompat.checkSelfPermission(this, WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED
                && ContextCompat.checkSelfPermission(this, READ_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED
                ;
    }
    protected void requestPermissionAndContinue(){
        if(ContextCompat.checkSelfPermission(this, WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED
                && ContextCompat.checkSelfPermission(this, READ_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED){
            if(ActivityCompat.shouldShowRequestPermissionRationale(this, WRITE_EXTERNAL_STORAGE)
                    && ActivityCompat.shouldShowRequestPermissionRationale(this, READ_EXTERNAL_STORAGE)){
                AlertDialog.Builder alertBuilder = new AlertDialog.Builder(this);
                alertBuilder.setCancelable(true);
                alertBuilder.setPositiveButton(android.R.string.yes, new DialogInterface.OnClickListener() {
                    @TargetApi(Build.VERSION_CODES.JELLY_BEAN)
                    public void onClick(DialogInterface dialog, int which) {
                        ActivityCompat.requestPermissions(MainActivity.this, new String[]{WRITE_EXTERNAL_STORAGE
                                , READ_EXTERNAL_STORAGE}, PERMISSION_REQUEST_CODE);
                    }
                });
                AlertDialog alert = alertBuilder.create();
                alert.show();
                Log.e("", "Permission Denied");
            }else ActivityCompat.requestPermissions(MainActivity.this, new String[]{WRITE_EXTERNAL_STORAGE, READ_EXTERNAL_STORAGE}, PERMISSION_REQUEST_CODE);
        } else openActivity();
    }

    //either permission is granted or program closes
    //if permission for something else, ask again
    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        if(requestCode == PERMISSION_REQUEST_CODE) {
            if(permissions.length > 0 && grantResults.length > 0) {
                boolean flag = true;
                for(int i = 0; i < grantResults.length; i++) if(grantResults[i] != PackageManager.PERMISSION_GRANTED) flag = false;
                if(flag) openActivity();
                else this.finish();
            } else this.finish();
        } else super.onRequestPermissionsResult(requestCode, permissions, grantResults);
    }

    //required for writing to external storage
    public boolean isExternalStorageWritable() {
        String state = Environment.getExternalStorageState();
        if(Environment.MEDIA_MOUNTED.equals(state)) return true;
        return false;
    }


    //permissions checked on start of activity
    @Override
    protected void onCreate(Bundle savedInstanceState){
        super.onCreate(savedInstanceState);
        if (!checkPermission()) openActivity();
        else{
            if(checkPermission()) requestPermissionAndContinue();
            else openActivity();
        }
    }

    //close pipes on exit
    public void onDestroy() {
        super.onDestroy();
        try {
            pis.close();
            pos.close();
        } catch(IOException e){
            e.printStackTrace();
        }
    }

    public void openActivity(){ //parts of the application that runs after permissions have been granted
        try {
            pos = new PipedOutputStream();
            pis = new PipedInputStream(pos);
        } catch (IOException e) {
            e.printStackTrace();
        }
        String buff = "";


        //for raw nou oof file type (deprecated)
        /*if(this.isExternalStorageWritable()){
        } else System.exit(0);
        File root = Environment.getExternalStorageDirectory();
        File dir = new File(root.getAbsolutePath()+"/Download");
        File raw = new File(dir, "test");
        File nou = new File(dir, "test.nou");
        File oof = new File(dir, "test.oof");
        if(raw.exists()) raw.delete();
        if(nou.exists()) nou.delete();
        if(oof.exists()) oof.delete();

        if(this.isExternalStorageWritable()){
        } else System.exit(0);
        File root = Environment.getExternalStorageDirectory();
        File dir = new File(root.getAbsolutePath()+"/Download"); */

        //sample ui
        setContentView(R.layout.activity_main);
        TextView tv = findViewById(R.id.sample_text);
        final TextView tv2 = findViewById(R.id.data);
        final TextView tv3 = findViewById(R.id.gesturedata);
        /*fn = findViewById(R.id.FileName);
        logb = findViewById(R.id.logb);
        record100b = findViewById(R.id.record100b);*/

        api_bt btsample = new api_bt();
        btsample.act = this;
        btsample.btadt = BluetoothAdapter.getDefaultAdapter(); //use phone's built-in bt adapter
        if(btsample.btadt == null) tv.setText("Device does not support Bluetooth.\n"); //check if bt adapter exists
        else{
            btsample.ifoffbton(); //if off turn bt on
            Set<BluetoothDevice> btdevices = btsample.listbtdevices(); //list bt paired devices
            BluetoothDevice btdev = btsample.findpaireddevice(btdevices, "HC-06"); //specify which of the paired devices is the imu using either name or name + mac
            if(btdev != null){ //paired imu found
                buff = btdev.getName() + " | " + btdev.getAddress();
                tv.setText(buff); //displays imu device name and mac
                ConThread = new ConnectThread(btdev, btsample.btadt); //creates connection from specified bt adapter to bt device
                Thread athread = new Thread(ConThread);
                athread.start(); //this has to start in another thread since thread closes after connection is established
                //application code separate from the api starts here
                h = new Handler(); //for viewing data received from imu
                r = new Runnable(){
                    @Override
                    public void run() {
                        if(ConThread.ManThread != null) if(ConThread.ManThread.proc != null) if(ConThread.ManThread.proc.AT.isAlive()) apiinit = 1;
                        if(apiinit == 1){
                            tv2.setText("xAccel: " + Double.toString(frameproc.out[0]) + "\nyAccel: " + Double.toString(frameproc.out[1]) + "\nzAccel: " + Double.toString(frameproc.out[2]) + "\nxW: " + Double.toString(frameproc.out[3]) + "\nyW: " + Double.toString(frameproc.out[4]) + "\nzW: " + Double.toString(frameproc.out[5]) + "\nRaw: " + Double.toString(frameproc.out[6]) + "\nYaw: " + Double.toString(frameproc.out[7]) + "\nPitch: " + Double.toString(frameproc.out[8]));
                            //+ "\nxCAccel: " + Double.toString(frameproc.out[9]) + "\nyCAccel: " + Double.toString(frameproc.out[10]) + "\nzCAccel: " + Double.toString(frameproc.out[11]));
                            //displays current imu data; processed data stored in global variable
                            tv3.setText(frameproc.gesture);
                            if(frameproc.gesture.equals("none") == FALSE) frameproc.received = 1;
                        }
                        h.postDelayed(this, 100); //ms
                    }
                };
                h.postDelayed(r, 1);
                /*record100b.OnClickListener(new View.OnClickListener(){
                    @Override
                    public void onClick(View v){
                        if(record==0){
                            name = fn.getText().toString();
                            fp = new File(dir, "name"+".txt");
                            record = 1;
                        }
                    }
                })*/
            } else System.exit(0); //app closes if not paired to imu device
        }
    }
}
