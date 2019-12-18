"""tools for cross-validate a classification model sklearn mainly"""
import numpy as np

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
