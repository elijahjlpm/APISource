package com.example.apisource;

import android.util.Log;

import java.io.IOException;
import java.util.Arrays;

//import static com.example.test2.MainActivity.record;

public class frameproc implements Runnable{
    static {
        System.loadLibrary("native-lib");
    }
    public native double[] frpfxn(byte[] barr); //frame processing function using jni
    public static volatile int rawlen = 0; //counter
    //short rawdat[] = new short[9]; //for raw file (deprecated)
    //static double[] out = new double[12]; //to be displayed in ui w/ compensated accel
    static double[] out = new double[9];
    static volatile String gesture = "none";
    static volatile double[][] buff = new double[100][9]; //to be sent to model for input probability
    byte[] frame = new byte[31]; //data frame from imu
    static volatile int updated = 0;
    static volatile int received = 0;
    long counter = 0;
    double ftr = 0.9;
    apithread api = new apithread();
    Thread AT = new Thread(api); //gesture recognition algorithm runs in a different thread

    public void run(){
        AT.start();
        for(int row = 0; row < 100; row++) Arrays.fill(buff[row], 0);
        api.apiInput = buff;
        while(true){
            if(api.done == 1){ //if gesture detected, empty buffer and hold off buffer update while communicating with main thread
                gesture = api.apiout;
                Log.d("APIThread", "Gesture: " + gesture);
                for(int row = 0; row < 100; row++) Arrays.fill(buff[row], 0);//reset buffer after gesture detected
                try {
                    MainActivity.pis.skip(MainActivity.pis.available());
                } catch (IOException e){
                    e.printStackTrace();
                }
                counter = 0;
                /*try {
                    Thread.sleep(250);
                } catch (InterruptedException e){
                    e.printStackTrace();
                }*/
                while(received == 0);
                received = 0;
                api.apiout = "none";
                gesture = api.apiout;
                api.done = 0; // gesture recog thread ready to process another set of data
            }
            try{
                if(MainActivity.pis.available() >= 31){
                    MainActivity.pis.read(frame, 0, 31); //gets bytes of frame from IMU thread
                    updated = 0;
                }
                else continue;
            } catch (IOException e) {
                e.printStackTrace();
            }
            out = frpfxn(frame);
            if(out == null) continue; //in case of errors from frame processing such as checksum error or wrong number of bytes for a frame
            if(counter<100)counter++;
            //out = functions.LPF(out, buff, counter, ftr);
            //Log.d("accZ", Double.toString(out[2]));
            buff = functions.RemoveLeastRecent(buff, 100, 9); //to keep buffer at 100 samples
            System.arraycopy(out,0,buff[99],0,9); //update latest point
            updated = 1;
            /*if(record==1){
                functions.LogData(MainActivity.fp, buff[rawlen],true);
                MainActivity.samp++;
                if(MainActivity.samp == 99){
                    record = 0;
                    MainActivity.runOnUiThread(new Runnable() {
                        public void run() {
                            Toast.makeText(MainActivity, "100 samples saved to " + MainActivity.fn +".txt", Toast.LENGTH_SHORT).show();
                        }
                    });
                }
            } else MainActivity.samp = 0;*/

            /*try { //for raw nou oof file type generation(deprecated)
                if (rawlen < 100);
                else {
                    functions.RemoveFirstLine(raw);
                    functions.RemoveFirstLine(nou);
                    functions.RemoveFirstLine(oof);
                }
                rawdat[0] = (short) (frame[1] << 8 | frame[0]);
                rawdat[1] = (short) (frame[3] << 8 | frame[2]);
                rawdat[2] = (short) (frame[5] << 8 | frame[4]);
                rawdat[3] = (short) (frame[12] << 8 | frame[11]);
                rawdat[4] = (short) (frame[14] << 8 | frame[13]);
                rawdat[5] = (short) (frame[16] << 8 | frame[15]);
                rawdat[6] = (short) (frame[23] << 8 | frame[22]);
                rawdat[7] = (short) (frame[25] << 8 | frame[24]);
                rawdat[8] = (short) (frame[27] << 8 | frame[26]);
                FileOutputStream fos = new FileOutputStream(raw, true);
                fos.write((functions.getCurrentTimeStamp() + ": " + rawdat[0] + ", " + rawdat[1] + ", " + rawdat[2] + ", " + rawdat[3] + ", " + rawdat[4] + ", " + rawdat[5] + ", " + rawdat[6] + ", " + rawdat[7] + ", " + rawdat[8] + "\n").getBytes());
                fos.close();
                FileOutputStream fos1 = new FileOutputStream(nou, true);
                fos1.write((functions.getCurrentTimeStamp() + ": " + a[0] + ", " + a[1] + ", " + a[2] + ", " + w[0] + ", " + w[1] + ", " + w[2] + ", " + Angle[0] + ", " + Angle[1] + ", " + Angle[2] + "\n").getBytes());
                fos1.close();
                FileOutputStream fos2 = new FileOutputStream(oof, true);
                fos2.write((functions.getCurrentTimeStamp() + ": " + (a[0] - uv[0]) + ", " + (a[1] - uv[1]) + ", " + (a[2] - uv[2]) + ", " + w[0] + ", " + w[1] + ", " + w[2] + ", " + Angle[0] + ", " + Angle[1] + ", " + Angle[2] + "\n").getBytes());
                fos2.close();
                rawlen++;
            } catch (FileNotFoundException e){
                e.printStackTrace();
            } catch (IOException e){
                e.printStackTrace();
            }*/
        }
    }
}
