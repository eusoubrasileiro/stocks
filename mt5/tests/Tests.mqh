// check almost equal with espsilon tolerance
bool almostEqual(double &a[], double &b[], int size, double eps){
  for(int i=0; i<size; i++)
    if( a[i] > b[i] + eps || a[i] < b[i] - eps)
      return false;
  return true;
}
