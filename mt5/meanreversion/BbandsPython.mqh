#import "pythondlib.dll"
    int pyTrainModel(double &X[], int &y[], int ntraining, int xtrain_dim,
          char &model[], int pystr_size);
#import

class sklearnModel
{
public:
  //dont know the size of a sklearn extra trees
  // serialized so put something big here 500Kb
  char pystrmodel[1024*5000];
  int  pystr_size; // size of above that is really the model
  bool isready; // is ready to be used
  void sklearnModel(void){
    isready = false;
  }
  int MaxSize(){ return ArraySize(pystrmodel); } // max size
  //void ~sklearnModel(){ ArrayFree(pystrmodel); }
};



