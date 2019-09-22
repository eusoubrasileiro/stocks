
class sklearnModel
{
public:
  //dont know the size of a sklearn extra trees
  // serialized so put something big here 100Kb
  char pystrmodel[1024*100];
  int  pystr_size; // size of above that is really the model
  bool isready; // is ready to be used
  void sklearnModel(void){
    isready = false;
  }
};
