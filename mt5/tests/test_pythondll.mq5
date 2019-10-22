#property copyright "Andre L. Ferreira 2018"
#property version   "1.00"
#property script_show_inputs
//--- input parameters

#import "pythondlib.dll"
int  pyTrainModel(double &X[], int &y[], int ntraining, int xtrain_dim,
    char& model[], int pymodel_size_max);
int pyPredictwModel(double &X[], int xtrain_dim, 
    char &model[], int pymodel_size);
int Unique(double &arr[], int n);
#import

void test_Unique(){
    double ex[] = {1, 1, 2, 3, 4, 5., 5};

    int size = Unique(ex, 7);

    if (size == 5 && ex[0]==1 && ex[1]==2 && ex[2]==3 && ex[4]==5 )
        Print("Passed - Test Numpy Unique");
    else
        Print("Failed - Test Numpy Unique");
}

void test_pyTrainModel(){
	// xor example
	double X[] = { 0, 0, 1, 1, 0, 1, 1, 0};  // second dim = 2
	int y[] = { 0, 0, -1, -1 }; // first dim = 4
	int xdim = 2;
	int ntrain = 4;
	char pymodel[];
	ArrayResize(pymodel, 1024*500); // 500 Kb
	int pymodel_size = pyTrainModel(X, y, 4, 2, pymodel, 1024*500);
	if(pymodel_size > 0){
		Print("Passed - Test pyTrainModel");
    for(int i=0; i<3; i++){ // input {0, 1} what will be the prediction? 1 should be
      double Xp[2] = {0, 1};
      int ypred = pyPredictwModel(Xp, 2, pymodel, pymodel_size);
      if(ypred == -1)
        Print("Passed - Test pyPredictwModel");
    }
  }
	ArrayFree(pymodel);
}

void OnStart(){
    for(int i=0; i<3;i++){
        test_Unique();
        test_pyTrainModel();
    }
}
