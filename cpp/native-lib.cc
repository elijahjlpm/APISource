#include <jni.h>
#include <string>
#include "math.h"
#include <iostream>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <cmath>
#include <iterator>
#include <numeric>
#include <cstring>
#include <iomanip>
#include <fstream>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <android/log.h>

using namespace std;

typedef long double ld;
typedef unsigned int uint;
typedef std::vector<ld>::iterator vec_iter_ld;

/**
 * Overriding the ostream operator for pretty printing vectors.
 */
template<typename T>
std::ostream &operator<<(std::ostream &os, std::vector<T> vec) {
    os << "[";
    if (vec.size() != 0) {
        std::copy(vec.begin(), vec.end() - 1, std::ostream_iterator<T>(os, " "));
        os << vec.back();
    }
    os << "]";
    return os;
}

/**
 * This class calculates mean and standard deviation of a subvector.
 * This is basically stats computation of a subvector of a window size qual to "lag".
 */
class VectorStats {
public:
    /**
     * Constructor for VectorStats class.
     *
     * @param start - This is the iterator position of the start of the window,
     * @param end   - This is the iterator position of the end of the window,
     */
    VectorStats(vec_iter_ld start, vec_iter_ld end) {
        this->start = start;
        this->end = end;
        this->compute();
    }

    /**
     * This method calculates the mean and standard deviation using STL function.
     * This is the Two-Pass implementation of the Mean & Variance calculation.
     */
    void compute() {
        ld sum = std::accumulate(start, end, 0.0);
        uint slice_size = std::distance(start, end);
        ld mean = sum / slice_size;
        std::vector<ld> diff(slice_size);
        std::transform(start, end, diff.begin(), [mean](ld x) { return x - mean; });
        ld sq_sum = std::inner_product(diff.begin(), diff.end(), diff.begin(), 0.0);
        ld std_dev = std::sqrt(sq_sum / slice_size);

        this->m1 = mean;
        this->m2 = std_dev;
    }

    ld mean() {
        return m1;
    }

    ld standard_deviation() {
        return m2;
    }

private:
    vec_iter_ld start;
    vec_iter_ld end;
    ld m1;
    ld m2;
};

/**
 * This is the implementation of the Smoothed Z-Score Algorithm.
 * This is direction translation of https://stackoverflow.com/a/22640362/1461896.
 *
 * @param input - input signal
 * @param lag - the lag of the moving window
 * @param threshold - the z-score at which the algorithm signals
 * @param influence - the influence (between 0 and 1) of new signals on the mean and standard deviation
 * @return a hashmap containing the filtered signal and corresponding mean and standard deviation.
 */
unordered_map<string, vector<ld>> z_score_thresholding(vector<ld> input, int lag, ld threshold, ld influence) {
    unordered_map<string, vector<ld>> output;

    uint n = (uint) input.size();
    vector<ld> signals(input.size());
    vector<ld> filtered_input(input.begin(), input.end());
    vector<ld> filtered_mean(input.size());
    vector<ld> filtered_stddev(input.size());

    VectorStats lag_subvector_stats(input.begin(), input.begin() + lag);
    filtered_mean[lag - 1] = lag_subvector_stats.mean();
    filtered_stddev[lag - 1] = lag_subvector_stats.standard_deviation();

    for (int i = lag; i < n; i++) {
        if (abs(input[i] - filtered_mean[i - 1]) > threshold * filtered_stddev[i - 1]) {
            signals[i] = (input[i] > filtered_mean[i - 1]) ? 1.0 : -1.0;
            filtered_input[i] = influence * input[i] + (1 - influence) * filtered_input[i - 1];
        } else {
            signals[i] = 0.0;
            filtered_input[i] = input[i];
        }
        VectorStats lag_subvector_stats(filtered_input.begin() + (i - lag), filtered_input.begin() + i);
        filtered_mean[i] = lag_subvector_stats.mean();
        filtered_stddev[i] = lag_subvector_stats.standard_deviation();
    }

    output["signals"] = signals;
    output["filtered_mean"] = filtered_mean;
    output["filtered_stddev"] = filtered_stddev;

    return output;
};

long double average_arr(long double *array){
    int num_elements = 100;

    long double average = 0.0;
    for (int i = 0; i < num_elements; ++i){
        average += array[i];
    }

    return average/double(num_elements);
}

bool isSubset(ld *arr1, ld *arr2, int m, int n){
    bool matches;
    if(n == 0) return 1;
    if(m < n) return false;
    for(unsigned int i = 0; i < m-(n-1); i++){
        if(arr1[i] == arr2[0]){
            matches = true;
            for(unsigned int j = 1; j < n; j++){
                if (arr1[i+j] != arr2[j]){
                    matches = false;
                    break;
                }
            }
            if(matches) return true;
        }
    } return false;
}

extern "C" JNIEXPORT jdoubleArray JNICALL
//format for android: Java_com_<packagename but replace "." with "_">_where function is called_FunctionName
Java_com_example_apisource_frameproc_frpfxn(
        JNIEnv *env,
        jobject /* this */,
        jbyteArray barr) {
    jint length = env->GetArrayLength(barr);
    jbyte* buffer = env->GetByteArrayElements(barr,JNI_FALSE);

    jdouble a[3];
    jdouble w[3];
    jdouble Angle[3];
    double uv[3];
    jdouble out[9];

    if(length == 31){
        if (buffer[8] == (jbyte) (0x55 + 0x51 + buffer[0] + buffer[1] + buffer[2] + buffer[3] + buffer[4] + buffer[5] + buffer[6] + buffer[7])) {
            a[0] = (jdouble)((jshort) (buffer[1] << 8 | buffer[0])) / 32768.0 * 16.0;
            a[1] = (jdouble)((jshort) (buffer[3] << 8 | buffer[2])) / 32768.0 * 16.0;
            a[2] = (jdouble)((jshort) (buffer[5] << 8 | buffer[4])) / 32768.0 * 16.0;
        } else {
            __android_log_write(ANDROID_LOG_DEBUG,"frpfxn", "Checksum Error");
            return NULL; //checksum error
        }
        if (buffer[19] == (jbyte) (0x55 + 0x52 + buffer[11] + buffer[12] + buffer[13] + buffer[14] + buffer[15] + buffer[16] + buffer[17] + buffer[18])) {
            w[0] = (jdouble)((jshort) (buffer[12] << 8 | buffer[11])) / 32768.0 * 2000.0;
            w[1] = (jdouble)((jshort) (buffer[14] << 8 | buffer[13])) / 32768.0 * 2000.0;
            w[2] = (jdouble)((jshort) (buffer[16] << 8 | buffer[15])) / 32768.0 * 2000.0;
        } else {
            __android_log_write(ANDROID_LOG_DEBUG,"frpfxn", "Checksum Error");
            return NULL; //checksum error
        }
        if (buffer[30] == (jbyte) (0x55 + 0x53 + buffer[22] + buffer[23] + buffer[24] + buffer[25] + buffer[26] + buffer[27] + buffer[28] + buffer[29])) {
            Angle[0] = (jdouble)((jshort) (buffer[23] << 8 | buffer[22])) / 32768.0 * 180.0;
            Angle[1] = (jdouble)((jshort) (buffer[25] << 8 | buffer[24])) / 32768.0 * 180.0;
            Angle[2] = (jdouble)((jshort) (buffer[27] << 8 | buffer[26])) / 32768.0 * 180.0;
        } else {
            __android_log_write(ANDROID_LOG_DEBUG,"frpfxn", "Checksum Error");
            return NULL; //checksum error
        }

        /*uv[0] = (cos(Angle[2]* M_1_PIl) * sin(Angle[1]* M_1_PIl) * cos(Angle[0]* M_1_PIl)) + (sin(Angle[2]* M_1_PIl) * sin(Angle[0]* M_1_PIl));
        uv[1] = (sin(Angle[2]* M_1_PIl) * sin(Angle[1]* M_1_PIl) * cos(Angle[0]* M_1_PIl)) - (cos(Angle[2]* M_1_PIl) * sin(Angle[0]* M_1_PIl));
        uv[2] = cos(Angle[1]* M_1_PIl) * cos(Angle[0]* M_1_PIl);*/

        out[0] = a[0];
        out[1] = a[1];
        out[2] = a[2];
        out[3] = w[0];
        out[4] = w[1];
        out[5] = w[2];
        out[6] = Angle[0];
        out[7] = Angle[1];
        out[8] = Angle[2];

        /*out[9] = a[0] - uv[0];
        out[10] = a[1] - uv[1];
        out[11] = a[2] - uv[2];*/

        jdoubleArray outJNI = env->NewDoubleArray(9);
        if(NULL == outJNI) return NULL;
        env->SetDoubleArrayRegion(outJNI,0,9,out);

        return outJNI;
    } else {
        __android_log_write(ANDROID_LOG_DEBUG,"frpfxn", "Framelength Error");
        return NULL; //Framelength Error
    }
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_apisource_apithread_apirunmt(
        JNIEnv *env,
        jobject /* this */,
        jobjectArray buffer){

    ld *accX, *accY, *accZ, *avY;
    jint len1 = env -> GetArrayLength(buffer);
    accX = new ld[len1];
    accY = new ld[len1];
    accZ = new ld[len1];
    avY = new ld[len1];
    jdoubleArray oneDim;
    jdouble *element;

    for(int i=0; i<len1; ++i){
        oneDim = (jdoubleArray)env->GetObjectArrayElement(buffer, i);
        element = env->GetDoubleArrayElements(oneDim,0);
        accX[i] = (ld)element[0];
        accY[i] = (ld)element[1];
        accZ[i] = (ld)element[2];
        avY[i] = (ld)element[4];
        env->ReleaseDoubleArrayElements(oneDim, element, 0);
        env->DeleteLocalRef(oneDim);
    }

    vector<ld> inputX;
    vector<ld> inputY;
    vector<ld> inputZ;
    vector<ld> inputAVY; //angular velocity

    ld pos[] = {1,1,1,1,1,-1,-1,-1,-1,-1};
    ld neg[] = {-1,-1,-1,-1,-1,1,1,1,1,1};

    ld pos_rot[] = {1,1,1,1};
    ld neg_rot[] = {-1,-1,-1,-1};

    jint lag = 15;
    ld threshold = 3;
    ld influence = 0.0;

    jint sum_of_elems = 0;

    jdouble threshold_bandstop = 0.5;
    jdouble upperbound;
    jdouble lowerbound;

    ld signal_average;

    jstring result;

    static int fd1[2]; //Used to pass thread results from child to parent. fd[0] for read, fd[1] for write
    pipe(fd1); //for ipc bet. parent and child1
    if (pipe(fd1)==-1){
        result = env->NewStringUTF("pipe error");
        return result; //pipe error
    }
    pid_t pid = fork();

    if (pid == 0){ //child1
        close(fd1[0]);

        jint detect_left = 0;
        jint detect_right = 0;

        jint detect_up = 0;
        jint detect_down = 0;

        jint detect_upright = 0;
        jint detect_upleft = 0;

        jint detect_downright = 0;
        jint detect_downleft = 0;

        //X and Z axis gesture detection on this thread
        signal_average = average_arr(accX);

        upperbound = threshold_bandstop;
        lowerbound = threshold_bandstop*-1;

        for(size_t i = 0; i < 100; ++i) if((lowerbound < accX[i]) && (accX[i] < upperbound)) accX[i] = 0;
        copy(accX, accX + 100, std::back_inserter(inputX));

        unordered_map<string, vector<ld>> outputX = z_score_thresholding(inputX, lag, threshold, influence);

        outputX["signals"].erase(remove(outputX["signals"].begin(), outputX["signals"].end(), 0), outputX["signals"].end());
        outputX["signals"].shrink_to_fit();

        sum_of_elems = 0;
        for(auto& n : outputX["signals"]) sum_of_elems += n;

        vector <ld> Xbuff(outputX["signals"].begin(), outputX["signals"].end());
        ld *Xbuffarr = Xbuff.data();
        bool found_X_pos = isSubset(Xbuffarr, pos, Xbuff.size(), sizeof(pos)/sizeof(ld));
        if(found_X_pos == 1 && (outputX["signals"][1] == 1)) ++detect_right;

        bool found_X_neg = isSubset(Xbuffarr, neg, Xbuff.size(), sizeof(neg)/sizeof(ld));
        if(found_X_neg == 1 && (outputX["signals"][1] == -1)) ++detect_left;

        //Z Axis for Up Down Detection
        signal_average = average_arr(accZ);

        upperbound = signal_average + threshold_bandstop;
        lowerbound = signal_average - threshold_bandstop;

        for(size_t i = 0; i < 100; ++i) if((lowerbound < accZ[i]) && (accZ[i] < upperbound)) accZ[i] = 0;

        copy(accZ, accZ + 100, std::back_inserter(inputZ));

        unordered_map<string, vector<ld>> outputZ = z_score_thresholding(inputZ, lag, threshold, influence);

        outputZ["signals"].erase(remove(outputZ["signals"].begin(), outputZ["signals"].end(), 0), outputZ["signals"].end());
        outputZ["signals"].shrink_to_fit();

        sum_of_elems = 0;
        for(auto& n : outputZ["signals"]) sum_of_elems += n;

        vector <ld> Zbuff(outputZ["signals"].begin(), outputZ["signals"].end());
        ld *Zbuffarr = Zbuff.data();
        bool found_Z_pos = isSubset(Zbuffarr, pos, Zbuff.size(), sizeof(pos)/sizeof(ld));
        if(found_Z_pos == 1 && (outputZ["signals"][1] == 1)) ++detect_up;

        bool found_Z_neg = isSubset(Zbuffarr, neg, Zbuff.size(), sizeof(neg)/sizeof(ld));
        if(found_Z_neg == 1 && (outputZ["signals"][1] == -1)) ++detect_down;

        if(found_Z_pos == 1 && (outputZ["signals"][1] == 1)){
            if(found_X_pos == 1 && (outputX["signals"][1] == 1)) ++detect_upright;
            if(found_X_neg == 1 && (outputX["signals"][1] == -1)) ++detect_upleft;
        }

        if(found_Z_neg == 1 && (outputZ["signals"][1] == -1)){
            if(found_X_pos == 1 && (outputX["signals"][1] == 1)) ++detect_downright;
            if(found_X_neg == 1 && (outputX["signals"][1] == -1)) ++detect_downleft;
        }

        int pid1res[] = {detect_right, detect_left, detect_up, detect_down, detect_upright, detect_upleft, detect_downright, detect_downleft};
        write(fd1[1], pid1res, sizeof(pid1res));
        close(fd1[1]);

        inputX.clear();
        inputX.shrink_to_fit();
        inputY.clear();
        inputY.shrink_to_fit();
        inputZ.clear();
        inputZ.shrink_to_fit();
        inputAVY.clear();
        inputAVY.shrink_to_fit();
        Xbuff.clear();
        Zbuff.clear();
        outputX.clear();
        outputZ.clear();
        delete[] accX;
        delete[] accY;
        delete[] accZ;
        delete[] avY;
        exit(0);
    } else if (pid > 0){ //parent
        close(fd1[1]);

        jint detect_forward = 0;
        jint detect_backward = 0;

        jint detect_cw = 0;
        jint detect_cc = 0;

        //Rotation Gesture Detection on parent thread
        threshold_bandstop = 200;

        upperbound = avY[0] + threshold_bandstop;
        lowerbound = avY[0] - threshold_bandstop;

        for(size_t i = 0; i < 100; ++i) if((lowerbound < avY[i]) && (avY[i] < upperbound)) avY[i] = 0;

        copy(avY, avY + 100, std::back_inserter(inputAVY));

        unordered_map<string, vector<ld>> outputAVY = z_score_thresholding(inputAVY, lag, threshold, influence);

        outputAVY["signals"].erase(remove(outputAVY["signals"].begin(), outputAVY["signals"].end(), 0), outputAVY["signals"].end());
        outputAVY["signals"].shrink_to_fit();

        vector <ld> rotbuff(outputAVY["signals"].begin(), outputAVY["signals"].end());
        ld *rotbuffarr = rotbuff.data();
        bool found = isSubset(rotbuffarr, pos_rot, rotbuff.size(), sizeof(pos_rot)/sizeof(ld));
        if(found == 1 && (outputAVY["signals"][1] == 1)) ++detect_cw;

        found = isSubset(rotbuffarr, neg_rot, rotbuff.size(), sizeof(neg_rot)/sizeof(ld));
        if(found == 1 && (outputAVY["signals"][1] == -1)) ++detect_cc;

        //Y axis gesture detection on this thread
        threshold_bandstop = 0.5;
        signal_average = average_arr(accY);

        upperbound = signal_average + threshold_bandstop;
        lowerbound = signal_average - threshold_bandstop;

        for(size_t i = 0; i < 100; ++i) if((lowerbound < accY[i]) && (accY[i] < upperbound)) accY[i] = 0;
        copy(accY, accY+ 100, std::back_inserter(inputY));
        unordered_map<string, vector<ld>> outputY = z_score_thresholding(inputY, lag, threshold, influence);

        outputY["signals"].erase(remove(outputY["signals"].begin(), outputY["signals"].end(), 0), outputY["signals"].end());
        outputY["signals"].shrink_to_fit();

        sum_of_elems = 0;
        for(auto& n : outputY["signals"]) sum_of_elems += n;

        vector <ld> Ybuff(outputY["signals"].begin(), outputY["signals"].end());
        ld *Ybuffarr = Ybuff.data();
        bool found_Y_pos = isSubset(Ybuffarr, pos, Ybuff.size(), sizeof(pos)/sizeof(ld));
        if(found_Y_pos == 1 && (outputY["signals"][1] == 1)) ++detect_forward;

        bool found_Y_neg = isSubset(Ybuffarr, neg, Ybuff.size(), sizeof(neg)/sizeof(ld));
        if(found_Y_neg == 1 && (outputY["signals"][1] == -1)) ++detect_backward;

        int pid1rec[8];
        wait(NULL);
        read(fd1[0], pid1rec, sizeof(pid1rec));
        close(fd1[0]);

        inputX.clear();
        inputX.shrink_to_fit();
        inputY.clear();
        inputY.shrink_to_fit();
        inputZ.clear();
        inputZ.shrink_to_fit();
        inputAVY.clear();
        inputAVY.shrink_to_fit();
        rotbuff.clear();
        Ybuff.clear();
        outputAVY.clear();
        outputY.clear();
        delete[] accX;
        delete[] accY;
        delete[] accZ;
        delete[] avY;

        if(detect_cc){
            result = env->NewStringUTF("cc");
            return result;
        }
        if(detect_cw){
            result = env->NewStringUTF("cw");
            return result;
        }
        if(pid1rec[5] != 0){
            result = env->NewStringUTF("upleft");
            return result;
        }
        if(pid1rec[4] != 0){
            result = env->NewStringUTF("upright");
            return result;
        }
        if(pid1rec[7] != 0){
            result = env->NewStringUTF("downleft");
            return result;
        }
        if(pid1rec[6] != 0){
            result = env->NewStringUTF("downright");
            return result;
        }
        if(pid1rec[1] != 0){
            result = env->NewStringUTF("left");
            return result;
        }
        if(pid1rec[0] != 0){
            result = env->NewStringUTF("right");
            return result;
        }
        if(pid1rec[2] != 0){
            result = env->NewStringUTF("up");
            return result;
        }
        if(pid1rec[3] != 0){
            result = env->NewStringUTF("down");
            return result;
        }
        if(detect_forward){
            result = env->NewStringUTF("forward");
            return result;
        }
        if(detect_backward){
            result = env->NewStringUTF("backward");
            return result;
        }
        result = env->NewStringUTF("none");
        return result;
    }
    else{
        result = env->NewStringUTF("fork failed");
        return result; //fork failed
    }
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_apisource_apithread_apirun(
        JNIEnv *env,
        jobject /* this */,
        jobjectArray buffer){

    ld *accX, *accY, *accZ, *avY;
    jint len1 = env -> GetArrayLength(buffer);
    accX = new ld[len1];
    accY = new ld[len1];
    accZ = new ld[len1];
    avY = new ld[len1];
        jdoubleArray oneDim;
    jdouble *element;

    for(int i=0; i<len1; ++i){
        oneDim = (jdoubleArray)env->GetObjectArrayElement(buffer, i);
        element = env->GetDoubleArrayElements(oneDim,0);
        accX[i] = (ld)element[0];
        accY[i] = (ld)element[1];
        accZ[i] = (ld)element[2];
        avY[i] = (ld)element[4];
        env->ReleaseDoubleArrayElements(oneDim, element, 0);
        env->DeleteLocalRef(oneDim);
    }

    vector<ld> inputX;
    vector<ld> inputY;
    vector<ld> inputZ;
    vector<ld> inputAVY; //angular velocity

    ld neg[] = {-1,-1,-1,-1,1,1,1,1};
    ld pos[] = {1,1,1,1,-1,-1,-1,-1};

    jint detect_left = 0;
    jint detect_right = 0;

    jint detect_forward = 0;
    jint detect_backward = 0;

    jint detect_up = 0;
    jint detect_down = 0;

    jint detect_upright = 0;
    jint detect_upleft = 0;

    jint detect_downright = 0;
    jint detect_downleft = 0;

    jint detect_cw = 0;
    jint detect_cc = 0;

    ld pos_rot[] = {1,1,1,1};
    ld neg_rot[] = {-1,-1,-1,-1};

    jint lag = 4;
    ld threshold = 3.0;
    ld influence = 0.01;

    jint sum_of_elems = 0;

    jdouble threshold_bandstop = 500;
    jdouble upperbound;
    jdouble lowerbound;

    ld signal_average;

    jstring result;

    //Rotation Detection Using Y Angular Velocity
    upperbound = avY[0] + threshold_bandstop;
    lowerbound = avY[0] - threshold_bandstop;

    for(size_t i = 0; i < 100; ++i) if((lowerbound < avY[i]) && (avY[i] < upperbound)) avY[i] = 0;

    copy(avY, avY + 100, std::back_inserter(inputAVY));

    unordered_map<string, vector<ld>> outputAVY = z_score_thresholding(inputAVY, lag, threshold, influence);

    outputAVY["signals"].erase(remove(outputAVY["signals"].begin(), outputAVY["signals"].end(), 0), outputAVY["signals"].end());
    outputAVY["signals"].shrink_to_fit();

    vector <ld> rotbuff(outputAVY["signals"].begin(), outputAVY["signals"].end());
    ld *rotbuffarr = rotbuff.data();
    bool found = isSubset(rotbuffarr, pos_rot, rotbuff.size(), sizeof(pos_rot)/sizeof(ld));
    if(found == 1 && (outputAVY["signals"][1] == 1)) ++detect_cw;

    found = isSubset(rotbuffarr, neg_rot, rotbuff.size(), sizeof(neg_rot)/sizeof(ld));
    if(found == 1 && (outputAVY["signals"][1] == -1)) ++detect_cc;

    inputAVY.clear();
    inputAVY.shrink_to_fit();
    rotbuff.clear();
    rotbuff.shrink_to_fit();
    outputAVY.clear();
    delete[] avY;

    if(detect_cc || detect_cw){
        inputX.clear();
        inputX.shrink_to_fit();
        inputY.clear();
        inputY.shrink_to_fit();
        inputZ.clear();
        inputZ.shrink_to_fit();
        delete[] accX;
        delete[] accY;
        delete[] accZ;
        if(detect_cc) result = env->NewStringUTF("cc");
        else result = env->NewStringUTF("cw");
        return result;
    }

    threshold_bandstop = 0.85;
    //X axis for left right and diagonals detection
    signal_average = average_arr(accX);

    upperbound = threshold_bandstop;
    lowerbound = threshold_bandstop*-1;

    for(size_t i = 0; i < 100; ++i) if((lowerbound < accX[i]) && (accX[i] < upperbound)) accX[i] = 0;
    copy(accX, accX + 100, std::back_inserter(inputX));

    unordered_map<string, vector<ld>> outputX = z_score_thresholding(inputX, lag, threshold, influence);

    outputX["signals"].erase(remove(outputX["signals"].begin(), outputX["signals"].end(), 0), outputX["signals"].end());
    outputX["signals"].shrink_to_fit();

    sum_of_elems = 0;
    for(auto& n : outputX["signals"]) sum_of_elems += n;

    vector <ld> Xbuff(outputX["signals"].begin(), outputX["signals"].end());
    ld *Xbuffarr = Xbuff.data();
    bool found_X_pos = isSubset(Xbuffarr, pos, Xbuff.size(), sizeof(pos)/sizeof(ld));
    if(found_X_pos == 1 && (outputX["signals"][1] == 1)) ++detect_right;

    bool found_X_neg = isSubset(Xbuffarr, neg, Xbuff.size(), sizeof(neg)/sizeof(ld));
    if(found_X_neg == 1 && (outputX["signals"][1] == -1)) ++detect_left;

    inputX.clear();
    inputX.shrink_to_fit();
    Xbuff.clear();
    Xbuff.shrink_to_fit();
    delete[] accX;

    //Z axis for up down detection
    signal_average = average_arr(accZ);

    upperbound = signal_average + threshold_bandstop;
    lowerbound = signal_average - threshold_bandstop;

    for(size_t i = 0; i < 100; ++i) if((lowerbound < accZ[i]) && (accZ[i] < upperbound)) accZ[i] = 0;

    copy(accZ, accZ + 100, std::back_inserter(inputZ));

    unordered_map<string, vector<ld>> outputZ = z_score_thresholding(inputZ, lag, threshold, influence);

    outputZ["signals"].erase(remove(outputZ["signals"].begin(), outputZ["signals"].end(), 0), outputZ["signals"].end());
    outputZ["signals"].shrink_to_fit();

    sum_of_elems = 0;
    for(auto& n : outputZ["signals"]) sum_of_elems += n;

    vector <ld> Zbuff(outputZ["signals"].begin(), outputZ["signals"].end());
    ld *Zbuffarr = Zbuff.data();
    bool found_Z_pos = isSubset(Zbuffarr, pos, Zbuff.size(), sizeof(pos)/sizeof(ld));
    if(found_Z_pos == 1 && (outputZ["signals"][1] == 1)) ++detect_up;

    bool found_Z_neg = isSubset(Zbuffarr, neg, Zbuff.size(), sizeof(neg)/sizeof(ld));
    if (found_Z_neg == 1 && (outputZ["signals"][1] == -1)) ++detect_down;

    if(found_Z_pos == 1 && (outputZ["signals"][1] == 1)){
        if(found_X_pos == 1 && (outputX["signals"][1] == 1)) ++detect_upright;
        if(found_X_neg == 1 && (outputX["signals"][1] == -1)) ++detect_upleft;
    }

    if(found_Z_neg == 1 && (outputZ["signals"][1] == -1)){
        if(found_X_pos == 1 && (outputX["signals"][1] == 1)) ++detect_downright;
        if(found_X_neg == 1 && (outputX["signals"][1] == -1)) ++detect_downleft;
    }

    inputZ.clear();
    inputZ.shrink_to_fit();
    Zbuff.clear();
    Zbuff.shrink_to_fit();
    outputX.clear();
    outputZ.clear();
    delete[] accZ;

    if(detect_upleft || detect_upright || detect_downleft || detect_downright){
        inputY.clear();
        inputY.shrink_to_fit();
        delete[] accY;
        if(detect_upleft) result = env->NewStringUTF("upleft");
        else if(detect_upright) result = env->NewStringUTF("upright");
        else if(detect_downleft) result = env->NewStringUTF("downleft");
        else result = env->NewStringUTF("downright");
        return result;
    }

    if(detect_left || detect_right){
        inputY.clear();
        inputY.shrink_to_fit();
        delete[] accY;
        if(detect_left)result = env->NewStringUTF("left");
        else result = env->NewStringUTF("right");
        return result;
    }

    if(detect_up || detect_down){
        inputY.clear();
        inputY.shrink_to_fit();
        delete[] accY;
        if(detect_up)result = env->NewStringUTF("up");
        else result = env->NewStringUTF("down");
        return result;
    }

    //Y axis for forward backward detection
    signal_average = average_arr(accY);

    upperbound = signal_average + threshold_bandstop;
    lowerbound = signal_average - threshold_bandstop;

    for(size_t i = 0; i < 100; ++i) if((lowerbound < accY[i]) && (accY[i] < upperbound)) accY[i] = 0;
    copy(accY, accY+ 100, std::back_inserter(inputY));
    unordered_map<string, vector<ld>> outputY = z_score_thresholding(inputY, lag, threshold, influence);

    outputY["signals"].erase(remove(outputY["signals"].begin(), outputY["signals"].end(), 0), outputY["signals"].end());
    outputY["signals"].shrink_to_fit();

    sum_of_elems = 0;
    for(auto& n : outputY["signals"]) sum_of_elems += n;

    vector <ld> Ybuff(outputY["signals"].begin(), outputY["signals"].end());
    ld *Ybuffarr = Ybuff.data();
    bool found_Y_pos = isSubset(Ybuffarr, pos, Ybuff.size(), sizeof(pos)/sizeof(ld));
    if(found_Y_pos == 1 && (outputY["signals"][1] == 1)) ++detect_forward;

    bool found_Y_neg = isSubset(Ybuffarr, neg, Ybuff.size(), sizeof(neg)/sizeof(ld));
    if(found_Y_neg == 1 && (outputY["signals"][1] == -1)) ++detect_backward;

    inputY.clear();
    inputY.shrink_to_fit();
    outputY.clear();
    Ybuff.clear();
    Ybuff.shrink_to_fit();
    delete[] accY;

    if(detect_forward || detect_backward){
        if(detect_forward)result = env->NewStringUTF("forward");
        else result = env->NewStringUTF("backward");
        return result;
    }

    result = env->NewStringUTF("none");
    return result;
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_apisource_apithread_apirunhmm(
        JNIEnv *env,
        jobject /* this */,
        jobjectArray buffer){

    ld *accX, *accY, *accZ, *avY;
    jint len1 = env -> GetArrayLength(buffer);
    accX = new ld[len1];
    accY = new ld[len1];
    accZ = new ld[len1];
    avY = new ld[len1];
    jdoubleArray oneDim;
    jdouble *element;

    for(int i=0; i<len1; ++i){
        oneDim = (jdoubleArray)env->GetObjectArrayElement(buffer, i);
        element = env->GetDoubleArrayElements(oneDim,0);
        accX[i] = (ld)element[0];
        accY[i] = (ld)element[1];
        accZ[i] = (ld)element[2];
        avY[i] = (ld)element[4];
        env->ReleaseDoubleArrayElements(oneDim, element, 0);
        env->DeleteLocalRef(oneDim);
    }

    vector<ld> inputX;
    vector<ld> inputY;
    vector<ld> inputZ;
    vector<ld> inputAVY; //angular velocity

    ld prob_pos_x;
    ld prob_pos_y;
    ld prob_pos_z;

    ld prob_neg_x;
    ld prob_neg_y;
    ld prob_neg_z;

    ld prob_pos_rot;
    ld prob_neg_rot;

    jint detect_left = 0;
    jint detect_right = 0;

    jint detect_forward = 0;
    jint detect_backward = 0;

    jint detect_up = 0;
    jint detect_down = 0;

    jint detect_upright = 0;
    jint detect_upleft = 0;

    jint detect_downright = 0;
    jint detect_downleft = 0;

    jint detect_cw = 0;
    jint detect_cc = 0;

    ld neg[2][2] = {
            {0.999989, 1.10E-05},
            {0.0840672, 0.915933}
    };

    ld pos[2][2] = {
            {0.90725, 0.0927499},
            {1.00E-06, 0.999999}
    };


    ld neg_rot[2][2] = {
            {0.5, 0.5},
            {1.00E-06, 0.999999}
    };

    ld pos_rot[2][2] = {
            {0.999999, 1.00E-06},
            {0.5, 0.5}
    };

    jint lag = 15;
    ld threshold = 1;
    ld influence = 0;

    jdouble threshold_bandstop = 0.65;
    jdouble upperbound;
    jdouble lowerbound;

    ld signal_average;

    jint prestate = 0;
    jint currstate = 0;

    jstring result;

//=========================== X AXIS ===========================
    // SIGNAL PREP
    signal_average = average_arr(accX);

    upperbound = signal_average + threshold_bandstop;
    lowerbound = signal_average - threshold_bandstop;

    for(size_t i = 0; i < 100; ++i) if((lowerbound < accX[i]) && (accX[i] < upperbound)) accX[i] = 0;

    copy(accX, accX + 100, std::back_inserter(inputX));

    unordered_map<string, vector<ld>> outputX = z_score_thresholding(inputX, lag, threshold, influence);

    outputX["signals"].erase(remove(outputX["signals"].begin(), outputX["signals"].end(), 0), outputX["signals"].end());
    outputX["signals"].shrink_to_fit();

    vector <ld> intbufX(outputX["signals"].begin(), outputX["signals"].end());
    ld *o_x = intbufX.data();

    if(outputX["signals"].size() != 0){
        // HMM BASED RECOG, Compute probabilities
        prob_pos_x = 1;
        prob_neg_x = 1;
        for(int i = 1; i < 20; ++i){
            if(o_x[i - 1] == 1) prestate = 0;
            else prestate = 1;

            if(o_x[i] == 1) currstate = 0;
            else currstate = 1;

            prob_neg_x *= neg[prestate][currstate];
            prob_pos_x *= pos[prestate][currstate];
        }
        // Compare probabilities
        double no_movement_threshold = pow(0.99999,30);
        if(prob_pos_x > no_movement_threshold || prob_neg_x > no_movement_threshold) result = env->NewStringUTF("none");
        else if(prob_pos_x > prob_neg_x) ++detect_right;
        else ++detect_left;
    }
    intbufX.clear();

//=========================== Y AXIS ===========================
    // SIGNAL PREP
    signal_average = average_arr(accY);

    upperbound = signal_average + threshold_bandstop;
    lowerbound = signal_average - threshold_bandstop;

    for(size_t i = 0; i < 100; ++i) if((lowerbound < accY[i]) && (accY[i] < upperbound)) accY[i] = 0;

    copy(accY, accY + 100, std::back_inserter(inputY));

    unordered_map<string, vector<ld>> outputY = z_score_thresholding(inputY, lag, threshold, influence);

    outputY["signals"].erase(remove(outputY["signals"].begin(), outputY["signals"].end(), 0), outputY["signals"].end());
    outputY["signals"].shrink_to_fit();

    vector <ld> intbufY(outputY["signals"].begin(), outputY["signals"].end());
    ld *o_y = intbufY.data();


    if(outputY["signals"].size() != 0){
        // HMM BASED RECOG, Compute probabilities
        prob_pos_y = 1;
        prob_neg_y = 1;
        for(int i = 1; i < 20; ++i){
            if(o_y[i - 1] == 1) prestate = 0;
            else prestate = 1;

            if(o_y[i] == 1) currstate = 0;
            else currstate = 1;

            prob_neg_y *= neg[prestate][currstate];
            prob_pos_y *= pos[prestate][currstate];
        }
        // Compare probabilities
        double no_movement_threshold = pow(0.99999,30);
        if (prob_pos_y > no_movement_threshold || prob_neg_y > no_movement_threshold) result = env->NewStringUTF("none");
        else if (prob_pos_y > prob_neg_y) ++detect_forward;
        else ++detect_backward;
    }
    intbufY.clear();
//=========================== Z AXIS ===========================
    // SIGNAL PREP
    signal_average = average_arr(accZ);

    upperbound = signal_average + threshold_bandstop;
    lowerbound = signal_average - threshold_bandstop;

    for(size_t i = 0; i < 100; ++i) if((lowerbound < accZ[i]) && (accZ[i] < upperbound)) accZ[i] = 0;

    copy(accZ, accZ + 100, std::back_inserter(inputZ));

    unordered_map<string, vector<ld>> outputZ = z_score_thresholding(inputZ, lag, threshold, influence);

    outputZ["signals"].erase(remove(outputZ["signals"].begin(), outputZ["signals"].end(), 0), outputZ["signals"].end());
    outputZ["signals"].shrink_to_fit();
    vector <ld> intbufZ(outputZ["signals"].begin(), outputZ["signals"].end());
    ld *o_z = intbufZ.data();

    if(outputZ["signals"].size() != 0){
        // HMM BASED RECOG, Compute probabilities
        prob_pos_z = 1;
        prob_neg_z = 1;
        for(int i = 1; i < 20; ++i){
            if(o_z[i - 1] == 1) prestate = 0;
            else prestate = 1;

            if(o_z[i] == 1) currstate = 0;
            else currstate = 1;

            prob_neg_z *= neg[prestate][currstate];
            prob_pos_z *= pos[prestate][currstate];
        }

        // Compare probabilities
        double no_movement_threshold = pow(0.99999,30);
        if(prob_pos_z > no_movement_threshold || prob_neg_z > no_movement_threshold || prob_neg_z == 0 || prob_pos_z == 0) result = env->NewStringUTF("none");
        else if(prob_pos_z > prob_neg_z){
            ++detect_up;
            if (prob_pos_x > no_movement_threshold || prob_neg_x > no_movement_threshold) result = env->NewStringUTF("none");
            else if (prob_pos_x > prob_neg_x) ++detect_upright;
            else ++detect_upleft;
        }
        else {
            ++detect_down;
            if(prob_pos_x > no_movement_threshold || prob_neg_x > no_movement_threshold) result = env->NewStringUTF("none");
            else if(prob_pos_x > prob_neg_x) ++detect_downright;
            else ++detect_downleft;
        }
    }
    intbufZ.clear();

//===================== ROTATION
    threshold_bandstop = 200;

    upperbound = threshold_bandstop;
    lowerbound = threshold_bandstop*(-1);

    for (size_t i = 0; i < 100; ++i) if ((lowerbound < avY[i]) && (avY[i] < upperbound)) avY[i] = 0;

    copy(avY, avY + 100, std::back_inserter(inputAVY));
    unordered_map<string, vector<ld>> outputAVY = z_score_thresholding(inputAVY, lag, threshold, influence);
    outputAVY["signals"].erase(remove(outputAVY["signals"].begin(), outputAVY["signals"].end(), 0), outputAVY["signals"].end());
    outputAVY["signals"].shrink_to_fit();

    vector <ld> intbufAVY(outputAVY["signals"].begin(), outputAVY["signals"].end());
    ld *o_rot = intbufAVY.data();

    if(outputAVY["signals"].size() != 0){
        // HMM BASED RECOG, Compute probabilities
        prob_pos_rot = 1;
        prob_neg_rot = 1;
        for(int i = 1; i < 3; ++i){
            if(o_rot[i - 1] == 1) prestate = 0;
            else prestate = 1;

            if(o_rot[i] == 1) currstate = 0;
            else currstate = 1;

            prob_neg_rot *= neg_rot[prestate][currstate];
            prob_pos_rot *= pos_rot[prestate][currstate];
        }
        // Compare probabilities
        if(prob_pos_rot < prob_neg_rot) ++detect_cc;
        else ++detect_cw;
    }

    inputX.clear();
    inputY.clear();
    inputZ.clear();
    inputAVY.clear();

    outputX.clear();
    outputY.clear();
    outputZ.clear();
    outputAVY.clear();

    delete[] accX;
    delete[] accY;
    delete[] accZ;
    delete[] avY;

    if(detect_cc == 1){
        result = env->NewStringUTF("cc");
        return result;
    }
    if(detect_cw == 1){
        result = env->NewStringUTF("cw");
        return result;
    }
    if(detect_right == 1){
        result = env->NewStringUTF("right");
        return result;
    }
    if(detect_left == 1){
        result = env->NewStringUTF("left");
        return result;
    }
    if(detect_up == 1){
        result = env->NewStringUTF("up");
        return result;
    }
    if(detect_down == 1){
        result = env->NewStringUTF("down");
        return result;
    }
    if(detect_forward == 1){
        result = env->NewStringUTF("forward");
        return result;
    }
    if(detect_backward == 1){
        result = env->NewStringUTF("backward");
        return result;
    }

    if(detect_upright == 1){
        result = env->NewStringUTF("upright");
        return result;
    }
    if(detect_upleft == 1){
        result = env->NewStringUTF("upleft");
        return result;
    }
    if(detect_downright == 1){
        result = env->NewStringUTF("downright");
        return result;
    }
    if(detect_downleft == 1){
        result = env->NewStringUTF("downleft");
        return result;
    }
    result = env->NewStringUTF("none");
    return result;
}