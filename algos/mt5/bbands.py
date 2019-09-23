import numpy as np
from sklearn.ensemble import ExtraTreesClassifier
import pickle


# s = pickle.dumps(clf)
# clf2 = pickle.loads(s)
# clf2.predict(X[0:1])

def pyTrainModel(X, y):
    print(X)
    print(y)
    trees = ExtraTreesClassifier(n_estimators=120, verbose=0)
    trees.fit(X, y)
    # save model
    str_trees = pickle.dumps(trees)
    return np.array(str_trees) # easier to pass to C++

def fitPredict(X, y, Xp, njobs=None):
    if njobs is None:
        trees = ExtraTreesClassifier(n_estimators=120, verbose=0)#, max_features=300)
    else:
        trees = ExtraTreesClassifier(n_estimators=120, verbose=0, n_jobs=njobs)
    ypred = None
    try:
        # if don't have 3-classes for training cannot train model
        if len(np.unique(y)) == 3:
            trees.fit(X, y)
            ypred = trees.predict_proba(Xp.reshape(1, -1))
    except Exception as e:
        print(e, file=sys.stderr)
        if debug:
            raise(e)
    return ypred
