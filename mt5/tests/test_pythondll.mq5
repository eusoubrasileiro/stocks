#property copyright "Andre L. Ferreira 2018"
#property version   "1.00"
#property script_show_inputs
//--- input parameters

#import "pythondlib.dll"
int pyTrainModel(double &X[], int &y[], int ntraining, int xtrain_dim, char &model[], int pystr_size);
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

void test_pyTrainModel(){
	// xor example
	double X[] = { 0, 0, 1, 1, 0, 1, 1, 0};  // second dim = 2
	int y[] = { 0, 0, 1, 1 }; // first dim = 4
	int xdim = 2;
	int ntrain = 4;
	char strmodel[];
	ArrayResize(strmodel, 1024*500); // 500 Kb
	int result = pyTrainModel(X, y, 4, 2, strmodel, 1024 * 500);
	if(result > 0)
		Print("Passed - Test pyTrainModel");
	ArrayFree(strmodel);
}

void OnStart(){
    for(int i=0; i<3;i++){
        test_Unique();
        test_pyTrainModel();
    }
}
