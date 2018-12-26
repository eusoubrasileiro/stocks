"""Fitting a NN model tools for cross-validate the model"""
import numpy as np
import torch as th
from Tools.torchUtil import *
from Tools.util import progressbar

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
    another huge difference is that besides the validation group
    to control the overfitting on the training set there is a prediction
    group default (one sample) that score's again the validation accuracy
    ahead of the validation set.
    """
    def __init__(self, X, foldsize=None, splits=4, ratio=0.75, npred=1, device='cpu'):
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
        indexes = indexSequentialFolds(length, foldsize, verbose=False)

        self.nsplits  = len(indexes)
        self.ntrain = ntrain # training set number of samples
        self.ntest = ntest # validaton set number of samples
        self.npred = npred # prediction set number of samples
        self.split_indexes = indexes # start, end pair index for each fold
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
        X, Y = self.init(X, Y)
        for start, end in self.split_indexes:
            Xfold, yfold  = X[start:end], Y[start:end]
            yield Xfold, yfold

    # def kSplits(self, X, Y) :
    #     """
    #     alike to `kfold.split` sklearn
    #
    #     Return train sets, validation sets
    #         Xtrain, ytrain, Xscore, yscore
    #     """
    #     ntrain, ntest = self.ntrain, self.ntest
    #     X, Y = binaryTensors(X, Y)
    #     for start, end in self.split_indexes:
    #         Xfold, yfold  = X[start:end], Y[start:end]
    #         yield Xfold[:ntrain], yfold[:ntrain], Xfold[-ntest:], yfold[-ntest:]
    #
    # def kSpliti(self, X, Y, i):
    #     """
    #     return i'th split group of training and validation
    #         Xtrain, ytrain, Xscore, yscore
    #     """
    #     ntrain, ntest = self.ntrain, self.ntest
    #     assert i < self.nsplits, "index out of range"
    #     start, end = self.split_indexes[i]
    #     Xfold, yfold  = X[start:end], Y[start:end]
    #     return Xfold[:ntrain], yfold[:ntrain], Xfold[-ntest:], yfold[-ntest:]
    #
    # def Spliti(self, X, Y, i):
    #     """
    #     return i'th split group of training, validation and prediction
    #         Xtrain, ytrain, Xscore, yscore, Xpred, ypred
    #     """
    #     ntrain, ntest, npred = self.ntrain, self.ntest, self.npred
    #     assert i < self.nsplits, "index out of range"
    #     start, end = self.split_indexes[i]
    #     Xfold, yfold  = X[start:end], Y[start:end]
    #     return Xfold[:ntrain], yfold[:ntrain], Xfold[ntrain:ntrain+ntest], yfold[ntrain:ntrain+ntest], Xfold[-npred:], yfold[-npred:]

    def Splits(self) :
        """
        Return training, validation and prediction set boundary indexes
            Xtrain, ytrain, Xscore, yscore, Xpred, ypred
        """
        ntrain, ntest, npred = self.ntrain, self.ntest, self.npred
        for start, end in self.split_indexes:
            sval = start+ntrain
            spred = start+ntrain+ntest
            yield strain, sval, spred, end

    def SplitsLastn(self, n):
        """
        yields last n split group boundary indexes,
         4th dimensinal
         start-training   : start
         start-validation : sval
         start-prediction : spred
         end              : end
        """
        ntrain, ntest, npred = self.ntrain, self.ntest, self.npred
        assert n < self.nsplits, 'there are less splits'
        for i in range(n):
            start, end = self.split_indexes[-i]
            sval = start+ntrain
            spred = start+ntrain+ntest
            yield start, sval, spred, end

    def SplitsRandn(self, n):
        """
        yields n random split groups boundary indexes
         4th dimensinal
         start-training   : start
         start-validation : sval
         start-prediction : spred
         end              : end
        """
        ntrain, ntest, npred = self.ntrain, self.ntest, self.npred
        assert n < self.nsplits, 'there are less splits'
        for i in range(n):
            start, end = self.split_indexes[np.random.randint(self.nsplits)]
            sval = start+ntrain
            spred = start+ntrain+ntest
            yield start, sval, spred, end

# based on train-test split return vectors
def TrainTestSplit(X, Y, cv=''):
    """
    based on train-test split return vectors instead of indexes
    cv: maybe
        'lastn' - last subset
        'randn' - random subset
    default uses 1 'lastn'
    """

# this is equivalent to accuracy score of sklearn
def accuracy(model, X, y, cutoff=None, verbose=False):
    """
    equivalent to `accuracy_score` sklearn but using pytorch
    Use mse loss to calculate accuracy = (1-error)
    Clip predictions with probability bellow cutoff.
    """
    yprob, ypred = model.predict(X, cutoff=cutoff)
    n = ypred.size(0)
    nans = ypred < 0 # remove -1 class meaning nans due cutoff
    ypred = ypred[~nans]
    y = y[~nans]
    if verbose and cutoff is not None:# percentage of data above cutoff
        print('data above probability cutoff: {:.2f}'.format(ypred.size()[0]/n))
    error = th.nn.functional.mse_loss(
        ypred.float(), y.float())  # MSE accuracy
    return 1-error.item()

# from sklearn.metrics import classification_report
# print(classification_report(yp, ys))
# def balanced_accuracy(model, X, y, cutoff=0.7, verbose=True):
#     """
#     equivalent to `balanced_accuracy_score` sklearn
#     Average of recall obtained on each class.
#     Clip predictions with probability bellow cutoff.
#     """
#     yprob, ypred = model.predict(X, cutoff=cutoff)
#     n = ypred.size(0)
#     nans = th.isnan(ypred)
#     ypred = ypred[~nans]
#     y = y[~nans]
#     if verbose and cutoff is not None:# percentage of data above cutoff
#         print('data above probability cutoff: {:.2f}'.format(ypred.size()[0]/n))


# sklearn cross-validate, cross_val_score the inspiration as allways :-D
"""
Walk-forward Validation or
Sequential Folds Model Cross-Validation
global (real) cross-validation is made on prediction-set samples created
by sequential folds class `sKFold`
"""
def sCrossValidate(X, Y, classifier, foldsize, ratio=0.9, cv=5, kind='lastn', report_simple=False, fit_params = dict(), scores=[], predict=False, device='cpu'):
    """
     * scores : list of metric functions to call over classifer after every `fit` default `torchCV.accuracy`
     * fit_params : dict of params to pass to `classifer.fit` method
     * kind : any one those
        'lastn' : last n folds 'use latest data principle' (default)
        'randn' : n random folds
        'all'   : all folds possible (ignore cv)
    * cv : maybe a int number (used in combination with kind) or
        iterable/generator that yields splitted groups of
        training, validation and prediction (accuracy)
    * report_simple:
        True  : returns only the average of default score function
        False : returns array with columns
            index, train. score, valid. score, metric values ...
    * predict:
        True to save as the three last column the prediction probabilities
        for each class and the correct prediction
    """
    kfold = sKFold(X, foldsize, ratio=ratio, device=device)
    if kind == 'all':
        iterable = kfold.Splits()
    else:
        if type(cv) is int: # use specified kind or 'lastn' default
            if kind == 'lastn':
                iterable = kfold.SplitsLastn(cv)
            if kind == 'randn':
                iterable = kfold.SplitsRandn(cv)
    scorefuncs = [accuracy] # default accuracy metric function
    scorefuncs.extend(scores) # extend adding additonal metric functions
    accuracies = [] # whatever validation metrics values stored by i
    metricvalues = [] # store score values for each metric
    for start, sval, spred, end in iterable:
        # indexes are referenced to X, Y parent vectors
        # strain, sval, spred, end = vars
        Xt, yt = X[start:sval], Y[start:sval]
        Xs, ys = X[sval:spred], Y[sval:spred]
        # use :end on slicing to avoid using unsqueze
        Xp, yp = X[spred:end], Y[spred:end] # only ONE sample
        trainscore, valscore = classifier.fit(Xt, yt, Xs, ys, **fit_params)
        metricvalues.clear() # clean the list for new values
        for score in scorefuncs: # calculate every metric
            metricvalues.append(score(classifier, Xp, yp))
        if predict: # wether save the prediction probabilities
            yprobs, ypred = classifier.predict(Xp)
            metricvalues.extend(yprobs.tolist()[0])
            metricvalues.append(yp) # the expected class
        # spred: index position of the prediction
        accuracies.append([spred, trainscore, valscore, *metricvalues])
        # faster than isinstanciate a new class??
        classifier.reset() # reset weights and everything else
    accuracies = np.array(accuracies)
    if report_simple: # just the mean accuracy
        return np.mean(accuracies[:, 3])
    else:
        return accuracies

# cross val predict

# if score is empty use `classifier.score`
# easier to always use classifier.score and
# just add the additional metrics
