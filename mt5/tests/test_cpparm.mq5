#property copyright "Andre L. Ferreira 2018"
#property version   "1.00"
#property script_show_inputs
//--- input parameters

#import "cpparm.dll"
int Unique(double &arr[], int n);
// bins specify center of bins
int  Histsma(double &data[], int n,  double &bins[], int nb, int nwindow);
#import



void test_Unique(){
    double ex[] = {1, 1, 2, 3, 4, 5., 5};

    int size = Unique(ex, 7);

    if (size == 5 && ex[0]==1 && ex[1]==2 && ex[2]==3 && ex[4]==5 )
        Print("Passed - Test Unique");
    else
        Print("Failed - Test Unique");
}

int arange(double start, double stop, double step, double &arr[]){
    int size = MathFloor((stop-start)/step)+1;
    ArrayResize(arr, size);
    for(int i=0; i<size; i++)
        arr[i] = start + step*i;
    return size;
}

// very simple histogram calculation
void test_Histsma_Simple(){
    double ex[] = {1, 1, 2, 3, 4, 5., 5};
    double binsize=1;
    double bins[];
    double vmin = ex[ArrayMinimum(ex)];
    double vmax = ex[ArrayMaximum(ex)];
   
    int nbins = arange(vmin-binsize, vmax+binsize, binsize, bins);

    Histsma(ex, 7, bins, nbins, 1); // window 1 means no smooth at all

    if (bins[0]==0 && bins[1]==2 && bins[2]==1 &&  bins[3]==1 && bins[4]==1 && bins[5] == 2 && bins[6] == 0 )
        Print("Passed - Test Histsma_Simple");
    else
        Print("Failed - Test Histsma_Simple");
}


// smoothed histogram calculation
void test_Histsma_Sma(){
    double ex[] = {1, 1, 2, 3, 4, 5., 5};
    double binsize=1;
    double bins[];
    double vmin = ex[ArrayMinimum(ex)];
    double vmax = ex[ArrayMaximum(ex)];
   
    int nbins = arange(vmin-binsize, vmax+binsize, binsize, bins);

    Histsma(ex, 7, bins, nbins, 2); // window 1 means no smooth at all

    if (bins[0]==1 && bins[1]==1.5 && bins[2]==1 && bins[3]==1 && bins[4] == 1.5 && bins[5] == 1 )
        Print("Passed - Test Histsma_Sma");
    else
        Print("Failed - Test Histsma_Sma");
}

void OnStart(){
    test_Unique();
    test_Histsma_Simple();
    test_Histsma_Sma();
}
