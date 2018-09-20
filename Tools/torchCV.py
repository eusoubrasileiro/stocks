"""Fitting a NN model tools for cross-validate the model"""
import numpy as np

def indexSequentialFolds(length, size, verbose=True):
    """
    Sequential folds suitable for stock prediction 'like' k-folds
    Returns index of folds and number of folds.
    """
    nfolds = length-size
    indexfold = np.zeros((nfolds, 2), dtype=int) # begin and end 2
    for i in range(nfolds):
        indexfold[i, :] = np.array([i, i+size], dtype=int)
    print('number folds ', nfolds)
    return indexfold, nfolds


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
