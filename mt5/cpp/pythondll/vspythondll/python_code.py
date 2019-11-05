import numpy as np
from sklearn.ensemble import ExtraTreesClassifier
import pickle

debug = True
if debug:
    fileout = 'pytrain_out.txt'

def pyTrainModel(X, y):
    trees = ExtraTreesClassifier(
        n_estimators=820, # have to increase it someday to 1k at least
        verbose=0,
        class_weight='balanced_subsample', # inbalance class 'correction'
        criterion='entropy',
        min_weight_fraction_leaf=0.05, # by Marcos help not overfitting oob score
        oob_score=True,
        bootstrap=True,
        max_features=X.shape[1]//2) # help not overfitting half of features max

    trees.fit(X, y)

    if debug:
        np.savetxt(fileout, np.array([trees.oob_score_]))
        np.savetxt(fileout, X, newline=" ", delimiter=',', fmt='%.5f')
        np.savetxt(fileout, y, newline=" ", delimiter=',', fmt='%.5f')

    # save model
    str_trees = pickle.dumps(trees)
    return str_trees # easier to pass to C++

def pyPredictwModel(X, str_trees):
    trees = pickle.loads(str_trees);
    return trees.predict(X)[0]

def Unique(arr):
    return np.unique(arr);
