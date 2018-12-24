"""Fitting a NN model tools for cross-validate the model"""
import numpy as np
import torch as th

def indexSequentialFolds(length, size, verbose=True):
    """
    Given lenght (maximum array size) and fold size (window size):
    Returns a tuple of two itens:
     * array dim=2: start and end index of each fold
     * total number of folds
    """
    nfolds = length-size
    indexfold = np.zeros((nfolds, 2), dtype=int) # begin and end 2
    for i in range(nfolds):
        indexfold[i, :] = np.array([i, i+size], dtype=int)
    if verbose:
        print('number folds', nfolds)
    return indexfold, nfolds


class sKFold(object):
    """
    Sequential folds suitable for stock prediction
    cross-validation 'alike' k-folds
    Note:
    another huge difference is that besides the validation group
    to control the overfitting on the training set there is a prediction
    group default (one sample) that score's again the validation accuracy
    ahead of the validation set.
    """
    def __init__(self, X, foldsize=None, splits=4, ratio=0.75, npred=1, device='cpu'):
        """
        Create training, test and prediction sets based on number of splits.

        `X` feature vector
        `foldsize` is the window size non-overlaping for X, Y vectors
        `splits` if `foldsize` is not specified this is the total number of splits.
        default is 4 slices/splits
        `ratio` is the percentage of the fold window for due the training set
        the complement is the validation and prediction.
        `npred` number of samples on fold window used for prediction

        default is ~70% training, ~20% validation and 1 sample for prediction
        """
        length = len(X) # max array size
        assert splits < length, "splits must be smaller than X length"
        if splits < 2:
            assert splits > 2, "at least a split in two is expected"
        if foldsize is None: # fold calculated by number of splits or slices
            foldsize = length//splits
        foldf = foldsize - npred # effective fold discounting predict samples
        ntrain = int(foldf*ratio)
        ntest  = foldf - ntrain
        # calculate split indexes and real number of splits/slices
        indexes, nsplits = indexSequentialFolds(length, foldsize, verbose=False)

        self.ntrain = ntrain # training set number of samples
        self.ntest = ntest # validaton set number of samples
        self.npred = npred # prediction set number of samples
        self.split_indexes = indexes # start, end pair index for each fold
        self.nsplits = nsplits
        self.device = device

    def GetnSplits(self):
        """return number of splits"""
        return self.nsplits

    def Folds(self, X, Y):
        """
        Returns: X, y fold sets as a generator
            Xfold, yfold

        Use in case you need some preprocessing on the fold set.
        Otherwise use `splits` that return ready made trainging and validation sets.
        """
        X = th.tensor(X, device=self.device, dtype=th.float32)
        Y = th.tensor(Y, device=self.device, dtype=th.long)
        for start, end in self.split_indexes:
            Xfold, yfold  = X[start:end], Y[start:end]
            yield Xfold, yfold

    def kSplits(self, X, Y) :
        """
        alike to `kfold.split` sklearn

        Return train sets, validation sets
            Xtrain, ytrain, Xscore, yscore
        """
        ntrain, ntest = self.ntrain, self.ntest
        X = th.tensor(X, device=self.device, dtype=th.float32)
        Y = th.tensor(Y, device=self.device, dtype=th.long)
        for start, end in self.split_indexes:
            Xfold, yfold  = X[start:end], Y[start:end]
            yield Xfold[:ntrain], yfold[:ntrain], Xfold[-ntest:], yfold[-ntest:]

    def kSpliti(self, X, Y, i):
        """
        return i'th split group of training and validation
            Xtrain, ytrain, Xscore, yscore
        """
        ntrain, ntest = self.ntrain, self.ntest
        X = th.tensor(X, device=self.device, dtype=th.float32)
        Y = th.tensor(Y, device=self.device, dtype=th.long)
        assert i < self.nsplits, "index out of range"
        start, end = self.split_indexes[i]
        Xfold, yfold  = X[start:end], Y[start:end]
        return Xfold[:ntrain], yfold[:ntrain], Xfold[-ntest:], yfold[-ntest:]

    def Splits(self, X, Y) :
        """
        Return training, validation and prediction sets
            Xtrain, ytrain, Xscore, yscore, Xpred, ypred
        """
        X = th.tensor(X, device=self.device, dtype=th.float32)
        Y = th.tensor(Y, device=self.device, dtype=th.long)
        ntrain, ntest, npred = self.ntrain, self.ntest, self.npred
        for start, end in self.split_indexes:
            Xfold, yfold  = X[start:end], Y[start:end]
            yield Xfold[:ntrain], yfold[:ntrain], Xfold[ntrain:ntrain+ntest], yfold[ntrain:ntrain+ntest], Xfold[-npred:], yfold[-npred:]

    def Spliti(self, X, Y, i):
        """
        return i'th split group of training, validation and prediction
            Xtrain, ytrain, Xscore, yscore, Xpred, ypred
        """
        X = th.tensor(X, device=self.device, dtype=th.float32)
        Y = th.tensor(Y, device=self.device, dtype=th.long)
        ntrain, ntest, npred = self.ntrain, self.ntest, self.npred
        assert i < self.nsplits, "index out of range"
        start, end = self.split_indexes[i]
        Xfold, yfold  = X[start:end], Y[start:end]
        return Xfold[:ntrain], yfold[:ntrain], Xfold[ntrain:ntrain+ntest], yfold[ntrain:ntrain+ntest], Xfold[-npred:], yfold[-npred:]

# sklearn cross-validate, cross_val_score the inspiration as allways :-D
def sCrossValidate(object):
    """
    Sequential Folds Model Cross-Validation
    real cross-validation is made on prediction-set samples created
    by sequential folds class `sKFold`
    """
    def __init__(self, X, Y, classifier, ratio=0.9, cv=None,
    score=None, device='cpu'):
    """
    score list of metric functions to call over classifer after every `fit`
    """
    if cv is None: # ratio
        kfold = torchCV.sKFold(X, foldsize=ntrain, ratio=0.9, device=device)
    else:  # there will be cv steps
        kfold = torchCV.sKFold(X, foldsize=ntrain, device=device)
    accuracies = [] # whatever validation metrics stored by i
    for i, vars in enumerate(kfold.Splits(X, Y)):
        Xt, yt, Xs, ys, Xp, yp = vars
