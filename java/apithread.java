/**
 * @file apithread.java
 * @author IMU API team
 *
 * This contains the thread which detects gestures based on a chosen gesture detection algorithm.
 */

package com.example.apisource;

import android.util.Log;

import static com.example.apisource.frameproc.buff;
import static java.lang.Boolean.FALSE;

public class apithread implements Runnable{
    static {
        System.loadLibrary("native-lib");
    }
    public native String apirunmt(double[][] darr); //multithreaded version of heuristic
    public native String apirun(double[][] darr); //normal heuristic
    public native String apirunhmm(double[][] darr); //hmm
    public static volatile String apiout = "none";
    public static volatile double[][] apiInput;
    public static volatile int done = 0;
    /**
     * Thread.sleep can be modified for responsiveness but is limited by the Android machine's hardware.
     * This raises the done flag when a gesture is detected.
     * Stops when a gesture hasnt been received by the main thread yet.
     * Edit line 41 to choose gesture detection algorithm.
     */
    public void run(){
        while(true){
            try{
                Thread.sleep(250);
            }catch(InterruptedException e){
                Log.d("APIThread", "Sleep error.");
            }
            if((apiInput != null) && (done == 0)){
                while(frameproc.updated == 0);
                apiInput = functions.Copy2DimDouble(buff, 100, 9);
                apiout = apirunmt(apiInput);
                if(apiout.equals("none") == FALSE) done = 1; //gesture was detected, turn done to 0 after gesture has been processed by thread
            }
        }
    }
}