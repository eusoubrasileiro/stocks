
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


bool CExpertBands::PythonTrainModel(){
  // create input params for Python function
  double X[m_ntraining*m_xtrain_dim]; // 2D strided array less code to call python
  int y[m_ntraining];
  for(int i=0; i<m_ntraining;i++){ // get the most recent
    // X order doesnt matter pattern identification
     XyPair xypair = m_xypairs.GetData(i);
     ArrayCopy(X[i], xypair.X, i*m_xtrain_dim, 0, m_ntraining);
     y[i] = xypair.y[i];
   }
   int result = pyTrainModel(X, y, m_ntraining, m_xtrain_dim, m_model.pystrmodel, m_model.pystr_size);
   if(result != -1)
    return true;

   return false;
}
