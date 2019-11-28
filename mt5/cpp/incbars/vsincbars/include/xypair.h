#pragma once
#include "time.h"

//class Xfeature
//{
//public:
//    int xdim; // dimension of X feature vector
//    double *X; // X feature vector
//    Xfeature(void) : xdim(0) {}
//    void Resize(const int size) {
//        ArrayResize(X, size);
//        xdim = size;
//    }
//};

class XyPair
{ // single element needed for training
public:
    int xdim; // dimension of X feature vector
    std::vector<double> X; // X feature vector
    int bandn; // number of band where the signal happend
    int y; // class value
    // cannot use cbuffindex because buffers change/get updated
    //int cbuffindex; // buffer index value of y class
    timeday time; // when that happend
    // used to assembly X feature vector
    bool isready; // if XyPair is ready for training

    XyPair(void) : xdim(16) {
        X.resize(xdim);
        isready = false;
        time.ms = 0;
        time.day = 0;
        bandn = -1;
        y = 0;
    }

    XyPair(int vy, timeday& vtime, int vbandn) {
        y = vy;
        time = vtime;
        xdim = 0;
        isready = false;
        bandn = vbandn;
    }

    void Resize(const int size) {
        X.resize(size);
        xdim = size;
    }
};