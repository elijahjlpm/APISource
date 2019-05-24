package com.example.apisource;

import android.util.Log;

import static com.example.apisource.frameproc.buff;
import static java.lang.Boolean.FALSE;

public class apithread implements Runnable{
    static {
        System.loadLibrary("native-lib");
    }
    public native String apirunmt(double[][] darr); //multithreaded version
    public native String apirun(double[][] darr);
    public native String apirunhmm(double[][] darr);
    public static volatile String apiout = "none";
    public static volatile double[][] apiInput;
    public static volatile int done = 0;
    public void run(){
        while(true){
            try{
                Thread.sleep(1);
            }catch(InterruptedException e){
                Log.d("APIThread", "Sleep error.");
            }
            if((apiInput != null) && (done == 0)){
                while(frameproc.updated == 0);
                apiInput = functions.Copy2DimDouble(buff, 100, 9);
                apiout = apirun(apiInput);
                if(apiout.equals("none") == FALSE) done = 1; //gesture was detected, turn done to 0 after gesture has been processed by thread
            }
        }
    }
}