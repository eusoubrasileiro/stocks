#property copyright "Andre L. Ferreira 2018"
#property version   "1.00"
#property script_show_inputs
//--- input parameters

#import "pythondll.dll"
int Unique(double &arr[], int n);
#import

void test_Unique(){
    double ex[] = {1, 1, 2, 3, 4, 5., 5};

    int size = Unique(ex, 7);

    if (size == 5 && ex[0]==1 && ex[1]==2 && ex[2]==3 && ex[4]==5 )
        Print("Passed - Test Unique");
    else
        Print("Failed   - Test Unique");
}

void OnStart(){
    test_Unique();
}
