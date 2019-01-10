#include "Definitions.mqh"

bool TestReadPredictions(){
    // read 12 bytes prediction file {datetime:long, direction:int}
    // if the prediction date is equal the last ignore
    // return the prediction direction or 0 if it's the same as the last'
    string fname = "test_predictions.bin";
    //FILE_COMMON location of the file in a shared folder for all client terminals \Terminal\Common\Files
    ///home/andre/.wine/drive_c/users/andre/Application Data/MetaQuotes/Terminal/Common/Files
    int handle=FileOpen(fname, FILE_READ|FILE_BIN|FILE_COMMON);
    if(handle<0){
        Print("Something is wrong couldnt read test_predictions.bin file");
        return false;
    }
    Print("file read:", fname);
    ArrayResize(test_predictions, 6939); // Resize the array.
    //--- read all data from the file to the array
    FileReadArray(handle, test_predictions);
    FileClose(handle);
    npred = ArraySize(test_predictions);
    Print("Total number of test predictions: ",  npred);
    Print("First read : ", test_predictions[0].time, test_predictions[0].direction);
    // set the begin of the predictions
    ipred = 0;
    return true;
}

//cdef nextGuess(int[:, ::1] guess_time, int iguess, int time):
//    """"find the first time_index bigger or
//    equal than `time` in the guess book"""
//    cdef int t
//    cdef int n = guess_time.shape[0]
//    # find the next time index after index time
//    for t in range(iguess, n):
//        if guess_time[t, 1] >= time:
//            return t
//    # couldn\'t find any. time to stop simulation
//    return -1

int nextIndexPrediction(int index, datetime time){
    for(int i=index; i<npred; i++)
        if(test_predictions[i].time >= time)
            return i;
    return npred-1; // no more predictions give the last
}

bool TestGetPrediction(prediction &pnow, datetime timenow){
    ipred = nextIndexPrediction(ipred, timenow);
    // if the prediction date is equal the last ignore
    pnow.time = test_predictions[ipred].time;
    pnow.direction = test_predictions[ipred].direction;
    // the prediction time will not be exactly coincident with time now
    //  but we may have some tolerance
    if(timenow >= pnow.time && timenow <= pnow.time + 60)
        return true;
    else     // is not the time for this prediction make an order yet
        return false;
}
