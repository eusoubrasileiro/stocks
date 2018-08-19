#include "Definitions.mqh"


bool TestReadPredictions(){
    // read 12 bytes prediction file {datetime:long, direction:int}
    // if the prediction date is equal the last ignore
    // return the prediction direction or 0 if it's the same as the last'
    string fname = "predictions.bin";
    //FILE_COMMON location of the file in a shared folder for all client terminals \Terminal\Common\Files
    ///home/andre/.wine/drive_c/users/andre/Application Data/MetaQuotes/Terminal/Common/Files
    int handle=FileOpen(fname, FILE_READ|FILE_BIN|FILE_COMMON);
    if(handle<0){
        Print("Something is wrong couldnt read predictions file");
        return false;
    }
    Print("file read:", fname);
    ArrayResize(predictions, 850); // Resize the array.
    //--- read all data from the file to the array
    FileReadArray(handle, predictions);
    FileClose(handle);
    int size = ArraySize(predictions);
    Print("Total number of predictions: ",  size);
    Print("First read : ", predictions[0].time, predictions[0].direction);
    // set the begin of the predictions
    ipred = 0;
    return true;
}

bool TestGetPrediction(prediction &pnow, datetime timenow){
    // if the prediction date is equal the last ignore
    pnow.time = predictions[ipred].time;        
    pnow.direction = predictions[ipred].direction;    
    // the prediction time will not be exactly coincident with time now
    //  but we may have some tolerance
    if(timenow > pnow.time && timenow < pnow.time + 120)
        ipred++;
    else     // is not the time for this prediction make an order yet
        return false;
    return true;
}
