"""Fitting a NN model tools for cross-validate the model"""
import numpy as np


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
    """
    def __init__(self, X, foldsize=None, splits=4, ratio=0.75):
        """
        Create training and test sets based on number of splits.

        `X` feature vector
        `foldsize` is the window size non-overlaping for X, Y vectors
        `splits` if `foldsize` is not specified this is the total number of splits.
        default is 4 slices/splits
        `ratio` is the training set size divided by the window/fold number of samples.
        default is 75% training and 25% validation
        """
        length = len(X) # max array size
        assert splits < length, "splits must be smaller than X length"
        if splits != 4:
            assert splits > 2, "at least a split in two is expected"
        if foldsize is not None: # fold calculated by number of splits or slices
            foldsize = length//splits
        ntrain, ntest = int(foldsize*ratio), int(foldsize*(1.-ratio))
        # calculate split indexes and real number of splits/slices
        indexes, nsplits = indexSequentialFolds(length, foldsize, verbose=False)

        self.ntrain = ntrain # training set number of samples
        self.ntest = ntest # validaton set number of samples
        self.split_indexes = indexes # start, end pair index for each fold
        self.nsplits = nsplits

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
        for start, end in self.split_indexes:
            Xfold, yfold  = X[start:end].copy(), Y[start:end].copy()
            yield Xfold, yfold

    def Splits(self, X, Y) :
        """
        alike to `kfold.split` sklearn

        Return train sets, validation sets
            Xtrain, ytrain, Xscore, yscore
        """
        ntrain, ntest = self.ntrain, self.ntest
        for start, end in self.split_indexes:
            Xfold, yfold  = X[start:end].copy(), Y[start:end].copy()
            yield Xfold[:ntrain], yfold[:ntrain], Xfold[-ntest:], yfold[-ntest:]

    def SplitI(self, X, Y, i):
        """
        return i'th split group of training and validation
            Xtrain, ytrain, Xscore, yscore
        """
        ntrain, ntest = self.ntrain, self.ntest
        assert i < self.nsplits, "index out of range"
        start, end = self.split_indexes[i]
        Xfold, yfold  = X[start:end].copy(), Y[start:end].copy()
        return Xfold[:ntrain], yfold[:ntrain], Xfold[-ntest:], yfold[-ntest:]

# def crossValidate(ntrain, ntest, nscore, Xn, Yn):
#
#
# ntrain= 4*60*5
# ntest = 4*60*5*4*6
# indexfolds, nfolds = indexSequentialFolds(len(Xn), ntrain+ntest)
# accuracies = [] # validation accuracies
# for n in range(30):
#     i = np.random.randint(nfolds)
#     Xfold, yfold  = Xn[indexfolds[i,0]:indexfolds[i,1]].copy(), Yn[indexfolds[i,0]:indexfolds[i,1]].copy()
#     Xfold = (Xfold - Xfold.mean())/Xfold.std() # normalize variance=1 mean=0
#     # tensors
#     Xt, yt = torchNN.dfvectorstoTensor(Xfold[:ntrain], yfold[:ntrain], device)
#     Xs, ys = torchNN.dfvectorstoTensor(Xfold[-ntest:], yfold[-ntest:], device)
#     classifier = torchNN.BinaryNN(dropout=0.3, learn=5e-4, patience=7)
#     accuracy = classifier.fit(Xt, yt, Xs, ys, device, epochs=15, batch=64, score=0.5, gma=0.925, verbose=False)
#     accuracies.append(accuracy)
