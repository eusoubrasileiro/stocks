class XyPair
{ // single element needed for training
public:
    int xdim; // dimension of X feature vector
    double X[]; // X feature vector
    int y; // class value
    datetime time; // when that happend datetime
    // used to assembly X feature vector
    int cbuffindex; // buffer index value of y class
    bool isready; // if XyPair is ready for training

    XyPair(void): xdim(16), time(0) {
        ArrayResize(X, xdim);
        isready = false;
    }

    XyPair(int vy, datetime vtime, int vcbuffindex){
        y = vy;
        time = vtime;
        cbuffindex = vcbuffindex;
        xdim=0;
        isready=false;
    }

    ~XyPair(void){ if(xdim >0) ArrayFree(X); };

    void Resize(const int size){
        ArrayResize(X, size);
        xdim = size;
    }
};
