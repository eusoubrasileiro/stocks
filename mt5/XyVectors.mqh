
class Xfeature 
{
public:
    int xdim; // dimension of X feature vector
    double X[]; // X feature vector
    Xfeature(void) : xdim(0) {}
    ~Xfeature(void){ if(xdim >0) ArrayFree(X); };
    void Resize(const int size){
        ArrayResize(X, size);
        xdim = size;
    }
};

class XyPair
{ // single element needed for training
public:
    int xdim; // dimension of X feature vector
    double X[]; // X feature vector
    int y; // class value
    // cannot use cbuffindex because buffers change/get updated
    //int cbuffindex; // buffer index value of y class
    MqlDateTime time; // when that happend datetime
    // used to assembly X feature vector

    bool isready; // if XyPair is ready for training

    XyPair(void): xdim(16) {
        ArrayResize(X, xdim);
        isready = false;
        time.year=time.mon=time.day=time.hour=time.min=time.sec=0;
    }

    XyPair(int vy, MqlDateTime &vtime){
        y = vy;
        time = vtime;
        xdim=0;
        isready=false;
    }

    ~XyPair(void){ if(xdim >0) ArrayFree(X); };

    void Resize(const int size){
        ArrayResize(X, size);
        xdim = size;
    }
};
