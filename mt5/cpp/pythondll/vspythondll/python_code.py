import numpy as np
from sklearn.ensemble import ExtraTreesClassifier
import pickle

# s = pickle.dumps(clf)
# clf2 = pickle.loads(s)
# clf2.predict(X[0:1])

def pyTrainModel(X, y):
    trees = ExtraTreesClassifier(n_estimators=120, verbose=0)
    trees.fit(X, y)
    # save model
    str_trees = pickle.dumps(trees)
    return str_trees # easier to pass to C++

def Unique(arr):
    return np.unique(arr);
