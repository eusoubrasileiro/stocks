#import "pythondlib.dll"
int  pyTrainModel(double &X[], int &y[], int ntraining, int xtrain_dim,
    char& model[], int pymodel_size_max);
int pyPredictwModel(double &X[], int xtrain_dim, 
    char &model[], int pymodel_size);
#import

class sklearnModel
{
public:
  // dont know the size of a sklearn extra trees
  // serialized so put something big here 5MB
  char pymodel[1024*1024*5];
  // size of python model in bytes after created
  int  pymodel_size;
  bool isready; // is ready to be used
  void sklearnModel(void){
    isready = false;
    pymodel_size = 0;
  }
  // max size of a sklearn model in bytes
  int MaxSize(){ return ArraySize(pymodel); }
};
