/**
 * @file frameproc.java
 * @author IMU API team
 *
 * This processes frame of bytes from IMU into physical data.
 * This also passes 100 samples of physical data into the API thread which returns gesture detected.
 * Contains the updated and received flag used for synchronization with the API thread.
 * Also contains the String object gesture which contains the API thread's output.
 */

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
    //static double[] out = new double[12]; //to be displayed in ui w/ compensated accel
    static double[] out = new double[9];
    static volatile String gesture = "none";
    static volatile double[][] buff = new double[100][9]; //to be sent to model for input probability
    byte[] frame = new byte[31]; //data frame from imu
    static volatile int updated = 0;
    static volatile int received = 0;
    private int LPF_en = 0;
    private long counter = 0; //for lowpass filter
    private double ftr = 1; //for lowpass filter
    apithread api = new apithread();
    Thread AT = new Thread(api); //gesture recognition algorithm runs in a different thread

    /**
     *Enables lowpass filter with factor.
     */
    public void setLPF(int enable, double factor){
        LPF_en = enable;
        ftr = factor;
    }

    /**
     * Clears sample buffer to 0.
     * Also clears bytes in IMU to Android machine byte stream.
     */
    public void buffreset(){
        for(int row = 0; row < 100; row++) Arrays.fill(buff[row], 0);
        try {
            MainActivity.pis.skip(MainActivity.pis.available());
        } catch(IOException e) {
            e.printStackTrace();
        }
        counter = 0;
    }

    /**
     * Frame buffer is reset everytime a gesture is detected.
     * Waits for received flag to be enabled by main thread when a gesture is detected.
     */
    public void run(){
        AT.start();
        buffreset();
        api.apiInput = buff;
        while(true){
            if(api.done == 1){ //if gesture detected, empty buffer and hold off buffer update while communicating with main thread
                gesture = api.apiout;
                Log.d("APIThread", "Gesture: " + gesture);
                buffreset();//reset buffer after gesture detected
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
            if(LPF_en == 1) out = functions.LPF(out, buff, counter, ftr);
            buff = functions.RemoveLeastRecent(buff, 100, 9); //to keep buffer at 100 samples
            System.arraycopy(out,0,buff[99],0,9); //update latest point
            updated = 1;
        }
    }
}
