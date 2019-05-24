package com.example.apisource;

import android.bluetooth.BluetoothSocket;
import android.util.Log;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

public class manageMyConnectedSocket implements Runnable{
    private static final String TAG = "manageMyConnectedSocket";
    private final BluetoothSocket mmSocket;
    private final InputStream mmInStream;
    private final OutputStream mmOutStream;
    frameproc proc;

    public manageMyConnectedSocket(BluetoothSocket socket) {
        mmSocket = socket;
        InputStream tmpIn = null;
        OutputStream tmpOut = null;

        // Get the input and output streams; using temp objects because
        // member streams are final.
        try {
            tmpIn = socket.getInputStream();
        } catch (IOException e) {
            Log.e(TAG, "Error occurred when creating input stream", e);
        }
        try {
            tmpOut = socket.getOutputStream();
        } catch (IOException e) {
            Log.e(TAG, "Error occurred when creating output stream", e);
        }

        mmInStream = tmpIn;
        mmOutStream = tmpOut;
    }

    public void run(){
        proc = new frameproc();
        int numBytes; //counter of remaining bytes in byte stream
        byte frame[] = new byte[31]; //set of bytes from byte stream equivalent to 1 set of data
        //processes byte stream from imu into data
        Thread cthread = new Thread(proc);
        cthread.start(); //this has to be done in a separate thread to reduce latency
        // Keep listening to the InputStream until an exception occurs.
        while (true) {
            try{
                if(mmInStream.available() >= 33) numBytes = mmInStream.read();
                else continue;
                if((numBytes == 0x55)){
                    numBytes = mmInStream.read();
                    if(numBytes == 0x51){
                        mmInStream.read(frame);
                        if(frame[8] == (byte)(0x55 + 0x51 + frame[0] + frame[1] + frame[2] + frame[3] + frame[4] + frame[5] + frame[6] + frame[7])){
                            if(frame[19] == (byte)(0x55 + 0x52 + frame[11] + frame[12] + frame[13] + frame[14] + frame[15] + frame[16] + frame[17] + frame[18])){
                                if(frame[30] == (byte)(0x55 + 0x53 + frame[22] + frame[23] + frame[24] + frame[25] + frame[26] + frame[27] + frame[28] + frame[29])) break;
                            }
                        }
                    }
                }
            }catch(IOException e){
                Log.d(TAG, "Input stream was disconnected", e);
                break;
            }
        }
        while(true){
            try{
                if(proc.AT.isAlive()){
                    if(proc.api.done == 1) mmInStream.skip(mmInStream.available());
                }
                if (mmInStream.available() >= 33) numBytes = mmInStream.read();
                else continue;
                if((numBytes == 0x55)){
                    numBytes = mmInStream.read();
                    if(numBytes == 0x51){
                        numBytes = mmInStream.read(frame);
                        try{
                            MainActivity.pos.write(frame, 0, numBytes); //passes frame byte stream to frame processing thread
                            MainActivity.pos.flush();
                        } catch (IOException e){
                            e.printStackTrace();
                        }
                    }
                }
            }catch(IOException e){
                Log.d(TAG, "Input stream was disconnected", e);
                break;
            }
        }
    }

    // Call this method from the main activity to shut down the connection. can be used as onDestroy in this thread
    public void cancel(){
        try{
            mmSocket.close();
        }catch (IOException e){
            Log.e(TAG, "Could not close the connect socket", e);
        }
    }
}
