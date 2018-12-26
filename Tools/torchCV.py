"""Fitting a NN model tools for cross-validate the model"""
import numpy as np
import torch as th

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

    def init(self, X, Y):
        X = th.tensor(X, device=self.device, dtype=th.float32)
        Y = th.tensor(Y, device=self.device, dtype=th.long)
        return X, Y

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

    def kSplits(self, X, Y) :
        """
        alike to `kfold.split` sklearn

        Return train sets, validation sets
            Xtrain, ytrain, Xscore, yscore
        """
        ntrain, ntest = self.ntrain, self.ntest
        X, Y = self.init(X, Y)
        for start, end in self.split_indexes:
            Xfold, yfold  = X[start:end], Y[start:end]
            yield Xfold[:ntrain], yfold[:ntrain], Xfold[-ntest:], yfold[-ntest:]

    def kSpliti(self, X, Y, i):
        """
        return i'th split group of training and validation
            Xtrain, ytrain, Xscore, yscore
        """
        ntrain, ntest = self.ntrain, self.ntest
        assert i < self.nsplits, "index out of range"
        start, end = self.split_indexes[i]
        Xfold, yfold  = X[start:end], Y[start:end]
        return Xfold[:ntrain], yfold[:ntrain], Xfold[-ntest:], yfold[-ntest:]

    def Splits(self, X, Y) :
        """
        Return training, validation and prediction sets
            Xtrain, ytrain, Xscore, yscore, Xpred, ypred
        """
        X, Y = self.init(X, Y)
        ntrain, ntest, npred = self.ntrain, self.ntest, self.npred
        for start, end in self.split_indexes:
            Xfold, yfold  = X[start:end], Y[start:end]
            yield Xfold[:ntrain], yfold[:ntrain], Xfold[ntrain:ntrain+ntest], yfold[ntrain:ntrain+ntest], Xfold[-npred:], yfold[-npred:]

    def Spliti(self, X, Y, i):
        """
        return i'th split group of training, validation and prediction
            Xtrain, ytrain, Xscore, yscore, Xpred, ypred
        """
        ntrain, ntest, npred = self.ntrain, self.ntest, self.npred
        assert i < self.nsplits, "index out of range"
        start, end = self.split_indexes[i]
        Xfold, yfold  = X[start:end], Y[start:end]
        return Xfold[:ntrain], yfold[:ntrain], Xfold[ntrain:ntrain+ntest], yfold[ntrain:ntrain+ntest], Xfold[-npred:], yfold[-npred:]

    def SplitsLastn(self, X, Y, n):
        """last n split groups"""
        ntrain, ntest, npred = self.ntrain, self.ntest, self.npred
        X, Y = self.init(X, Y)
        assert n < self.nsplits, 'there are less splits'
        for i in range(n):
            start, end = self.split_indexes[-i]
            Xfold, yfold  = X[start:end], Y[start:end]
            yield Xfold[:ntrain], yfold[:ntrain], Xfold[ntrain:ntrain+ntest], yfold[ntrain:ntrain+ntest], Xfold[-npred:], yfold[-npred:]

    def SplitsRandn(self, X, Y, n):
        """n random split groups"""
        ntrain, ntest, npred = self.ntrain, self.ntest, self.npred
        X, Y = self.init(X, Y)
        assert n < self.nsplits, 'there are less splits'
        for i in range(n):
            start, end = self.split_indexes[np.random.randint(self.nsplits)]
            Xfold, yfold  = X[start:end], Y[start:end]
            yield Xfold[:ntrain], yfold[:ntrain], Xfold[ntrain:ntrain+ntest], yfold[ntrain:ntrain+ntest], Xfold[-npred:], yfold[-npred:]

def Accuracy(model, X, y, cutoff=0.7, verbose=True):
    """
    Use mse loss to calculate score error - 1. = accuracy
    Clip predictions with probability bellow cutoff.
    """
    yprob, ypred = model.predict(X, cutoff=cutoff)
    n = ypred.size(0)
    nans = th.isnan(ypred)
    ypred = ypred[~nans]
    y = y[~nans]
    if verbose and cutoff is not None:# percentage of data above cutoff
        print('data above probability cutoff: {:.2f}'.format(ypred.size()[0]/n))
    error = th.nn.functional.mse_loss(
        ypred.float(), y.float())  # MSE accuracy
    return 1-error.item()

# sklearn cross-validate, cross_val_score the inspiration as allways :-D
def sCrossValidate(object):
    """
    Sequential Folds Model Cross-Validation
    real cross-validation is made on prediction-set samples created
    by sequential folds class `sKFold`
    """
    def __init__(self, X, Y, classifier, foldsize, ratio=0.9, cv=None, fit_params = dict(), scores=[], device='cpu'):
        """
         * scores : list of metric functions to call over classifer after every `fit`
         * fit_params : dict of params to pass to `classifer.fit` method
        """
    if cv is None: # use all possible splits
        kfold = torchCV.sKFold(X, foldsize, ratio=ratio, device=device)
    else:  # there will be cv steps using the last data
        kfold = torchCV.sKFold(X, foldsize, ratio=ratio, device=device)
        # include additional case where validation is just on the latest data possible
    if not scores: # empty score function
        scores  = Accuracy # default accuracy metric
    accuracies = [] # whatever validation metrics stored by i
    metricvalues = [] # store score values for each metric
    for i, vars in enumerate(kfold.Splits(X, Y)):
        Xt, yt, Xs, ys, Xp, yp = vars
        # faster than isinstanciate a new class?
        classifier.reset() # reset weights and everything else

        trainscore, valscore = classifer.fit(Xt, yt, Xs, ys, **fit_params)
        for score in scores:
            metricvalues.append(score(classifer, Xp, yp))
        accuracies.append([i, trainscore, valscore, *metricvalues])

# if score is empty use `classifier.score`
# easier to always use classifier.score and
# just add the additional metrics
