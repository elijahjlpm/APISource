package com.example.apisource;

import android.app.Activity;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.content.Intent;
import android.util.Log;

import java.util.Set;

public class api_bt{
    //for permissions required by android
    private final static int REQUEST_ENABLE_BT = 1;
    BluetoothAdapter btadt; //phone bt adapter to be used
    Activity act;

    public int ifoffbton(){ //turn bt adapter on if prev off
        if (!btadt.isEnabled()){
            Intent enableBtIntent = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
            act.startActivityForResult(enableBtIntent, REQUEST_ENABLE_BT);
            return 1;
        }
        return 0;
    }

    public Set<BluetoothDevice> listbtdevices() {
        Set<BluetoothDevice> pairedDevices = btadt.getBondedDevices(); //bt devices paired
        return pairedDevices;
    }

    public BluetoothDevice findpaireddevice(Set<BluetoothDevice> btlist, String deviceName, String deviceHardwareAddress){ //get specified bt device from list of paired devices
        if (btlist.size() > 0) {
            for (BluetoothDevice device : btlist){
                if(device.getName().equals(deviceName)){
                    if(device.getAddress().equals(deviceHardwareAddress)) return device;
                    else continue;
                } else continue;
            } return null;
        } else return null;
    }

    public BluetoothDevice findpaireddevice(Set<BluetoothDevice> btlist, String deviceName){
        if (btlist.size() > 0){
            for (BluetoothDevice device : btlist) {
                Log.d("test", device.getName());
                if(device.getName().equals(deviceName)) return device;
            } return null;
        } else return null;
    }
}
