package com.example.apisource;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothSocket;
import android.util.Log;

import java.io.IOException;
import java.util.UUID;

public class ConnectThread implements Runnable{

    private final BluetoothSocket mmSocket;
    private final BluetoothDevice mmDevice;
    private static final String TAG = "ConnectThread"; //for debugging
    private static final UUID id = UUID.fromString("00001101-0000-1000-8000-00805F9B34FB"); //string obtained from imu manufacturer sample app
    BluetoothAdapter btadpt;
    manageMyConnectedSocket ManThread;

    public ConnectThread(BluetoothDevice device, BluetoothAdapter bluetoothAdapter) {
        // Use a temporary object that is later assigned to mmSocket
        // because mmSocket is final.
        this.btadpt = bluetoothAdapter;
        BluetoothSocket tmp = null;
        mmDevice = device;

        try {
            // Get a BluetoothSocket to connect with the given BluetoothDevice.
            // MY_UUID is the app's UUID string, also used in the server code.
            tmp = mmDevice.createInsecureRfcommSocketToServiceRecord(id);
        } catch (IOException e) {
            Log.e(TAG, "Socket's create() method failed.", e);
        }
        mmSocket = tmp;
    }

    public void run() {
        // Cancel discovery because it otherwise slows down the connection.
        btadpt.cancelDiscovery();

        try {
            // Connect to the remote device through the socket. This call blocks
            // until it succeeds or throws an exception.
            Thread.sleep(1000);
            mmSocket.connect();
        } catch (IOException connectException) {
            // Unable to connect; close the socket and return.
            try {
                Log.i(TAG, "Socket closed.");
                mmSocket.close();
            } catch (IOException closeException) {
                Log.e(TAG, "Could not close the client socket.", closeException);
            }
            return;
        } catch (InterruptedException e) {
            Log.d(TAG, "Sleep error!", e);
        }

        // The connection attempt succeeded. Perform work associated with
        // the connection in a separate thread.
        ManThread = new manageMyConnectedSocket(mmSocket);
        Thread bthread = new Thread(ManThread);
        bthread.start();
    }

    // Closes the client socket and causes the thread to finish. Can be used as onDestroy on this thread
    public void cancel() {
        try {
            mmSocket.close();
        } catch (IOException e) {
            Log.e(TAG, "Could not close the client socket.", e);
        }
    }
}
