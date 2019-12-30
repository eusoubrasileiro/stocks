"""tools for cross-validate a classification model sklearn mainly"""
import numpy as np
import pandas as pd

def indexSequentialFolds(length, size, verbose=True):
    """
    Given length (maximum array size) and fold size (window size):
    Returns
     * array dim=2: start and end index of each fold
    Total number of folds is the len()
    """
    nfolds = length-size
    indexfold = np.zeros((nfolds, 2), dtype=int) # begin and end 2
    for i in range(nfolds):
        indexfold[i, :] = np.array([i, i+size], dtype=int)
    if verbose:
        print('number folds', nfolds)
    return indexfold

class sKFold(object):
    """
    Sequential folds suitable for stock prediction
    cross-validation 'alike' k-folds
    Note:
    another difference is that besides training set
    there is a prediction
    group default (one sample) that score's again the validation accuracy
    ahead of the training set
    """
    def __init__(self, X, foldsize=None, splits=4, ratio=0.75, npred=0):
        """
        Create training, test and prediction sets based on number of splits.

        Main methods create split boundaries indexes.

        `X` feature vector
        `foldsize` is the window size non-overlaping for X, Y vectors
        `splits` if `foldsize` is not specified this is the total number of splits.
        default is 4 slices/splits
        `ratio` is the percentage of the fold window for due the training set
        the complement is the validation and prediction.
        `npred` number of samples on fold window used for prediction

        default is ~70% training, ~20% sample for prediction
        """
        length = len(X) # max array size
        assert splits < length, "splits must be smaller than X length"
        if splits < 2:
            assert splits > 2, "at least a split in two is expected"
        if foldsize is None: # fold calculated by number of splits or slices
            foldsize = length//splits
        if npred != 0:
            ntrain = foldsize - npred
        else:
            ntrain = int(foldsize*ratio)
        npred  = foldsize - ntrain
        # calculate split indexes and real number of splits/slices
        indexes = indexSequentialFolds(length, foldsize, verbose=False)

        self.nsplits  = len(indexes)
        self.ntrain = ntrain # training set number of samples
        self.npred = npred # prediction set number of samples
        self.split_indexes = indexes # start, end pair index for each fold

    def GetnSplits(self):
        """return number of splits"""
        return self.nsplits

    def kSplits(self, X, Y) :
        """
        alike to `kfold.split` sklearn

        Return train sets, validation sets
            Xtrain, ytrain, Xscore, yscore
        """
        ntrain, npred = self.ntrain, self.npred
        for start, end in self.split_indexes:
            Xfold, yfold  = X[start:end], Y[start:end]
            yield Xfold[:ntrain], yfold[:ntrain], Xfold[-npred:], yfold[-npred:]

    def SplitsRandn(self, n, X, Y):
        """
        yields n random split groups boundary indexes

        Return train sets, validation sets
            Xtrain, ytrain, Xscore, yscore
        """
        ntrain, npred = self.ntrain, self.npred
        assert n < self.nsplits, 'there are less splits'
        for i in range(n):
            start, end = self.split_indexes[np.random.randint(self.nsplits)]
            Xfold, yfold  = X[start:end], Y[start:end]
            yield Xfold[:ntrain], yfold[:ntrain], Xfold[-npred:], yfold[-npred:]

    def SplitsRandnIdxs(self, n):
        """
        yields n random split groups boundary indexes

        3 values that defines train sets, validation sets
            train_start, end_train/start_pred, pred_end
        """
        ntrain, npred = self.ntrain, self.npred
        assert n < self.nsplits, 'there are less splits'
        for i in range(n):
            start, end = self.split_indexes[np.random.randint(self.nsplits)]
            yield start, start + ntrain, end

# methods are compatible with cross_val_score
# class skfold(object):
#     """
#     Sequential folds suitable for stock prediction
#     cross-validation 'alike' k-folds sklearn cv-search compatible
#     """
#     def __init__(self, X, foldsize=None, splits=4, ratio=0.7, kind='default', n=None):
#         """
#         Create training, test sets indexes based on number of splits.
#
#         `X` feature vector
#         `foldsize` is the window size non-overlaping for X, Y vectors
#         `splits` if `foldsize` is not specified this is the total number of splits.
#         default is 4 slices/splits
#         `ratio` is the percentage of the fold window for due the training set
#         the complement is the validation
#
#         kind: can be one of those
#             'randn' where n specify number of splits
#             'lastn' where n specify number of splits
#              None default using all possible splits
#
#         default is ~70% training, ~20% validation
#         """
#         length = len(X) # max array size
#         assert splits < length, "splits must be smaller than X length"
#         if splits < 2:
#             assert splits > 2, "at least a split in two is expected"
#         if foldsize is None: # fold calculated by number of splits or slices
#             foldsize = length//splits
#         ntrain = int(foldsize*ratio)
#         ntest  = foldsize - ntrain
#         # calculate split indexes and real number of splits/slices
#         indexes = indexSequentialFolds(length, foldsize, verbose=False)
#         self.kind = kind
#         self.ntrain = ntrain # training set number of samples
#         self.ntest = ntest # validaton set number of samples
#         self.split_indexes = indexes # start, end pair index for each fold
#         # changed due gridsearch cv
#         self.n = len(indexes)
#         if self.kind == 'randn':
#             self.split = self._srandn
#             self.n_splits  = self.n
#         elif self.kind == 'lastn':
#             self.split = self._slastn
#             self.n_splits  = self.n
#         else: # default
#             self.split = self._splits
#
#     def get_n_splits(self,  X=None, y=None, groups=None):
#         """return number of splits"""
#         return self.n_splits
#
#     def _srandn(self, X=None, y=None, groups=None):
#         """
#         yields n random split groups  indexes
#         """
#         ntrain, ntest = self.ntrain, self.ntest
#         for i in range(self.n_splits):
#             start, end = self.split_indexes[np.random.randint(self.n)]
#             sval = start+ntrain
#             yield list(range(start, sval)), list(range(sval, end))
#
#     def _slastn(self, X=None, y=None, groups=None):
#         """
#         yields last n split group indexes
#         """
#         ntrain, ntest = self.ntrain, self.ntest
#         for i in range(self.n_splits):
#             start, end = self.split_indexes[-(1+i)]
#             sval = start+ntrain
#             yield list(range(start, sval)), list(range(sval, end))
#
#     def _splits(self, X=None, y=None, groups=None):
#         """
#         Return training, validation indexes
#             Xtrain, ytrain, Xscore, yscore
#         """
#         ntrain, ntest = self.ntrain, self.ntest
#         for start, end in self.split_indexes:
#             sval = start+ntrain
#             yield list(range(start, sval)), list(range(sval, end))



# Marcos Lopez de Prado
# PurgedKFold
# removing information from training samples (leaking prevention)


#`lbsigns['twhen']` and `lbsigns['tdone']`
# are start and end index of bars needed to form that specific Xy pair
# works like this t1 = pd.Series(lbsigns.tdone, index=lbsigns.twhen)
def getTrainTimes(t1,testTimes):
    """
    Given testTimes, find the times of the training observations.
    —t1.index: Time when the observation started.
    —t1.value: Time when the observation ended.
    —testTimes: Times of testing observations.
    """
    trn=t1.copy(deep=True)
    for i,j in testTimes.iteritems():
        df0=trn[(i<=trn.index)&(trn.index<=j)].index # train starts within test
        df1=trn[(i<=trn)&(trn<=j)].index # train ends within test
        df2=trn[(trn.index<=i)&(j<=trn)].index # train envelops test
        trn=trn.drop(df0.union(df1).union(df2))
    return trn

from sklearn.model_selection._split import _BaseKFold


# works like this
# dfX = pd.DataFrame(X, index=t1.index)
# dfy = pd.DataFrame(y, index=t1.index)
# pkfold = cv.PurgedKFold(t1=t1)
# for idxt, idxs in pkfold.split(dfX):
#     print(len(idxt), len(idxs))
class PurgedKFold(_BaseKFold):
    """
    Extend KFold class to work with labels that span intervals
    The train is purged of observations overlapping test-label intervals
    Test set is assumed contiguous (shuffle=False), w/o training samples in between
    """
    def __init__(self,n_splits=3,t1=None,pctEmbargo=0.):
        if not isinstance(t1,pd.Series):
            raise ValueError('Label Through Dates must be a pd.Series')
        super(PurgedKFold,self).__init__(n_splits,shufﬂe=False,random_state=None)
        self.t1=t1
        self.pctEmbargo=pctEmbargo

    def split(self,X,y=None,groups=None):
        if (X.index==self.t1.index).sum()!=len(self.t1):
            raise ValueError('X and ThruDateValues must have the same index')
        indices=np.arange(X.shape[0])
        mbrg=int(X.shape[0]*self.pctEmbargo)
        test_starts=[(i[0],i[-1]+1) for i in \
        np.array_split(np.arange(X.shape[0]),self.n_splits)]
        for i,j in test_starts:
            t0=self.t1.index[i] # start of test set
            test_indices=indices[i:j]
            maxT1Idx=self.t1.index.searchsorted(self.t1[test_indices].max())
            train_indices=self.t1.index.searchsorted(self.t1[self.t1<=t0].index)
            if maxT1Idx<X.shape[0]: # right train (with embargo)
                train_indices=np.concatenate((train_indices,indices[maxT1Idx+mbrg:]))
            yield train_indices,test_indices

from sklearn.metrics import log_loss, accuracy_score, confusion_matrix

# I dont care I cant see any usage for sample_weight
def cvScore(clf,X,y,sample_weight=None,scoring='neg_log_loss',t1=None,cv=None,cvGen=None,
pctEmbargo=None):
    if scoring not in ['neg_log_loss','accuracy']:
        raise Exception('wrong scoring method.')
    if cvGen is None:
        cvGen=PurgedKFold(n_splits=cv,t1=t1,pctEmbargo=pctEmbargo) # purged
    score=[]
    for train,test in cvGen.split(X=X):
        ﬁt=clf.ﬁt(X=X.iloc[train,:],y=y.iloc[train].values.ravel())
        #,sample_weight=sample_weight.iloc[train].values)
        prob=ﬁt.predict_proba(X.iloc[test,:])
        score_=-log_loss(y.iloc[test].values.ravel(),prob,
        labels=clf.classes_)
        #sample_weight=sample_weight.iloc[test].values,labels=clf.classes_)
        pred=ﬁt.predict(X.iloc[test,:])
        cm = confusion_matrix(y.iloc[test].values.ravel(), pred)
        if cm.shape[0] < 2: # case only 1 class on both vectors
            acc =  1 if (np.unique(y.iloc[test].values.ravel()) == np.unique(pred)) else 0
        else:
            acc = cm[0, 0]/(cm[0, 0]+cm[1, 0]) # for class 0 what matters
        score_ = [score_, acc, len(X.iloc[train,:]), len(X.iloc[test,:])]
        score.append(score_)
    return np.array(score)
