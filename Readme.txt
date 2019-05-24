How to integrate Android API into your application:

Prerequisites:
> IMU already paired with Android device
> Android device has Android OS 6.0 or higher
> Android application will make use of NDK

1. In android studio, modify AndroidManifest.xml to match file included in repository.
2. Copy all java files into your project's java folder. Modify MainActivity code on only parts after connecting to the IMU via Bluetooth.
3. Copy CMakeLists.txt and native-lib.cpp into your project's cpp folder.
4. Modify line1 on all files into your project package.
5. Modify names functions inside native-lib.cpp to match the path of the method in your project according to JNI standards.
6. Take note of UUID that your IMU uses. Change ConnectThread.id to match your device's UUID.
7. You can change the gesture detection algorithm used by changing the function called in line 28 of apithread.java into either apirun, apirunmt, or apirunhmm.
8. To obtain detected gesture, wait for apithread's done flag and get the string inside frameproc.gesture.
9. Enable the frameproc.received flag on the application thread and then process the data. Clear the variable used to store gesture input into "none" afterwards. 
10. Use runOnUiThread() on parts of application using gesture input during runtime.
11. Other features of the API are contained within functions.java. These can be used by simply calling the method during runtime.