package com.example.apisource;

import java.io.BufferedWriter;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.FileWriter;
import java.io.IOException;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Scanner;

public class functions {
    public static double[][] RemoveLeastRecent(double[][] array, int rows, int cols){//to keep imu buffer for model as 100 most recent samples
        int i = 0;
        while(i<rows-1){
            System.arraycopy(array[i+1],0,array[i],0,cols);
            i++;
        } return array;
    }

    public static double[][] Copy2DimDouble(double[][] src, int rows, int cols){//can copy buffer corresponding to gesture, for debugging
        int i = 0;
        double[][] dest = new double[rows][cols];
        while(i<rows){
            System.arraycopy(src[i],0,dest[i],0,cols);
            i++;
        } return dest;
    }

    public static void LogData(File path, double[] array, int cols){ //adds 1 frame in a new line of specified file
        // when starting new log with same name, overwrite file  with append = false since function only appends
        int i = 0;
        try {
            FileOutputStream fos = new FileOutputStream(path, true);
            while(i<cols-1){
                fos.write((array[i] + ", ").getBytes());
                i++;
            }
            fos.write((array[cols] + "\n").getBytes());
            fos.close();
        } catch (FileNotFoundException e) {
            e.printStackTrace();
        } catch (IOException e){
            e.printStackTrace();
        }
    }

    public static double[][] GetTrainingSample(String name, double[][] array, int cnt, double[] frame, int cols, int rows){
        //gets sample to train model
        //name is gesture name, array buffer to be used in model, cnt is nth frame of sample, cols is number of values in a single frame, rows is # of frames in a single sample
        //cnt has to be adjusted in calling function; increment by 1 after calling this function, reset to 0 after cnt==rows
        int i = 0;
        if(cnt<rows){
            while(i<cols){
                array[cnt][i] = frame[i];
                i++;
            }
        } return array;
    }

    public static void RemoveFirstLine(File input){//for raw nou oof (deprecated)
        try {
            Scanner fileScanner = new Scanner(input);
            fileScanner.nextLine();
            FileWriter fileStream = new FileWriter(input);
            BufferedWriter out = new BufferedWriter(fileStream);
            while (fileScanner.hasNextLine()) {
                String next = fileScanner.nextLine();
                if (next.equals("\n"))
                    out.newLine();
                else
                    out.write(next);
                out.newLine();
            }
            out.close();
        } catch(FileNotFoundException e){
            e.printStackTrace();
        } catch(IOException e){
            e.printStackTrace();
        }
    }
    public static String getCurrentTimeStamp(){ //can be used to know latency between time frame arrived and frame processed in debugging without adb which can cause larger delays
        SimpleDateFormat sdfDate = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss.SSS");
        Date now = new Date();
        String strDate = sdfDate.format(now);
        return strDate;
    }
    public static double[] LPF(double[] curr, double[][] prev, long size, double factor){
        int i = 0;
        int j = 0;
        double[] filtered = new double[curr.length];
        while(i < size){
            j = 0;
            while(j < curr.length){
                filtered[j] = prev[(prev.length - 1) - i][j] + filtered[j];
                j++;
            }
            i++;
        }
        j = 0;
        while(j < curr.length){
            filtered[j] = filtered[j] / size;
            filtered[j] = ((1-factor) * filtered[j]) + (factor * curr[j]);
            j++;
        }
        return filtered;
    }
}
