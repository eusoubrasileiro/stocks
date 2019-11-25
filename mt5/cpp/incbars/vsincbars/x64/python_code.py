#script only for testing 
import numpy as np
from sklearn.ensemble import ExtraTreesClassifier
import pickle

def pyTrainModel(X, y) :
    trees = ExtraTreesClassifier(n_estimators = 120, verbose = 0)
    trees.fit(X, y)
    # save model
    str_trees = pickle.dumps(trees)
    return str_trees # easier to pass to C++

def pyPredictwModel(X, str_trees) :
    trees = pickle.loads(str_trees);
    return trees.predict(X)[0]
