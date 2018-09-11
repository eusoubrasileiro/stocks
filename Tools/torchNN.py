import os
import numpy as np
import pandas as pd
import copy
import random # for reproducible results
from Tools.util import progressbar
import torch as th
import torch.nn.functional as F
from torch.optim import lr_scheduler

nwasted = 120 + 120 + 7  # number of lost samples due (shift + EMA's + unknown)
ntraining = 5*8*60 # previous 1 week of 8 hours for training
nscore = 120 # validation samples
nforecast = 120 # forecasting/prediction samples
nneed = ntraining + nscore + nforecast + nwasted # needed number of samples
npredict = nscore + nforecast + ntraining # samples for predicting ONE direction
# Pytorch
modelinit = None # reuse of model did not increase performance meaningfully
criterion = th.nn.CrossEntropyLoss()
# optimizer = None
# scheduler = None

def TorchModelPredict(model, X):
    y_prob = model(X)
    y_pred = th.argmax(y_prob, 1) # binary class clip convertion
    return y_prob, y_pred

def getDevice():
    return th.device("cuda" if th.cuda.is_available() else "cpu")

def initModel(input_size, model=None, device="cuda"):
    """create a sequential NN or just copy the weights
    initiate optimizer and scheduler"""
    global modelinit
    if model is None: # create a new model
        model = th.nn.Sequential(th.nn.Linear(input_size, 1024), th.nn.ReLU(), th.nn.Dropout(.1),
                         th.nn.Linear(1024,280), th.nn.ReLU(), th.nn.Dropout(.3),
                         th.nn.Linear(280, 70), th.nn.ReLU(),  th.nn.Dropout(.05),
                         th.nn.Linear(70, 2), th.nn.Softmax(dim=1)) # dim == 1 collumns add up to 1 probability
        model = model.to(device)
        modelinit = copy.deepcopy(model.state_dict())
    else: # just set the params for the beginning (reset weights)
        model.load_state_dict(modelinit)
    #weight_decay regularization not applied
    optimizer = th.optim.Adam(model.parameters(), lr=0.1) # learning rate optimal
    # Decay LR by a factor of 0.1 every 10 epochs
    scheduler = lr_scheduler.StepLR(optimizer, step_size=15, gamma=0.1)
    return model, optimizer, scheduler


def trainTorchNet(X, y, X_s, y_s, input_size, device="cuda", verbose=True,
        model=None, batch_size=256, nepochs=30):
    """
    X, y         : vectors for training
    X_s, y_s     : vector for validation (scoring the model)

    the best model with smaller error in validation will be returned
    after the nepochs of training
    """

    model, optimizer, scheduler = initModel(input_size, model, device)

    best_model = None
    best_lossv = 1000.
    best_error = 100.

    # def tensorShuffle(X, y, size):
    #     """ get a random slice of size from X and y
    #     note:.better use the dataset api in the future"""
    #     i = np.random.randint(X.shape[0]-size)
    #     return X[i:i+size], y[i:i+size]

    # shuflled slices
    start = th.randint(X.shape[0]-batch_size, (nepochs,), dtype=th.int64).to(device)
    end = start + batch_size

    for t in range(nepochs):

        X_t, y_t = X[start[t]:end[t]], y[start[t]:end[t]]

        y_pred = model(X_t)
        # Compute and print loss
        lossy = criterion(y_pred, y_t)
        # Zero gradients, perform a backward pass, and update the weights.
        optimizer.zero_grad()
        lossy.backward()
        optimizer.step()
        # could also store best score and try
        # to continue training from that like tensorflow checkpoints
        # Model Evaluation
        model.eval()
        # calculate error complement of accuracy
        # on training set
        y_pred = th.argmax(y_pred, 1)
        error_train = th.nn.functional.mse_loss(
            y_pred.float(), y_t.float())
        # calculate percentage accuracy
        # on validation set
        y_pred = model(X_s)
        lossv = criterion(y_pred, y_s)
        y_pred = th.argmax(y_pred, 1)
        error_validation = th.nn.functional.mse_loss(
            y_pred.float(), y_s.float())

        ### error on validation set next 60 samples / minutes
        ### todo: better tecniques of early stopping, early stopping
        ### is a regularization so other techinques might be better?
        ### or easier?
        if lossv < best_lossv: # now loss for validation is smaller
            # lets save this network
            best_lossv = lossv
            best_errorv = error_validation
            best_errort = error_train
            best_model = copy.deepcopy(model.state_dict()) # save the actual weights

        if verbose:
            if t%int(nepochs/5) == 0:
                # on the loss values could apply a kind of normalization due the fact that they
                # don't have same size of samples BUT just BEAR ON MIND
                # loss error on validation set absolute value has NOTHING to do with
                # with the absolute value of the training set
                print("iteration : {:6d} l training: {:.2f} l validation: {:.2f}"
                      " Err trainging :{:.2f}  Err validation ; {:.3f}".format(
                      t, lossy, lossv, error_train, error_validation))

        model.train()
        scheduler.step()

    # return the best model in the validation and the accuracy
    best_errorv = best_errorv.item()
    best_errort = best_errort.item()
    model.load_state_dict(best_model)
    #print(best_errort, best_errorv)
    # return the model and accuracy of training and validation
    return model, best_errort, best_errorv

def predictDecideBuySell(errort, errorv, model, X_p):
    """1 for going up -1 for going down"""
    # we cannot predict unless the model is 90%+ accurate
    # accurate on validation. on training we accept 70%+
    if errorv > 0.1 or errort > 0.3:
        return 0
    buy=0 # do nothing
    probability, prediction = TorchModelPredict(model, X_p)

    # calculate percent of direction on the prediction minutes
    down = th.abs((th.sum(prediction)-nforecast))/nforecast
    up = th.abs(1-down)
    down = down.item()
    up = up.item()
    # only if next minutes will be 90% time going up or down
    if up > 0.9 or down > 0.9:
        # where will it be bought in time
        # allways +5 minutes after prediction
        buy = up > down
        if buy: # buy
            buy = 1
        else: # sell
            buy = -1
    return buy

def TrainPredictDecide(X, y, Xp, verbose=True):
    """
    X, y training/validation vectors for binary classification class y
    Xp vector for prediction
    Use only the latest npredict number of samples!
    """

    times = Xp.index # same times, last time will saved for prediction
    X = X.values.astype(np.float32) # due Float tensors
    Xp = Xp.values.astype(np.float32)
    y = y.values.astype(np.int64) # due Cross Entropy Loss requeires Tensor Long
    input_size = X.shape[1]

    device = getDevice()
    # get the latest npredict size samples (FUNDAMENTAL)
    # otherwise prediction is wrong!
    X = X[-npredict:]
    y = y[-npredict:]

    X = th.tensor(X)
    X = X.to(device)
    y = th.tensor(y)
    y = y.to(device)
    Xp = th.tensor(Xp)
    Xp = X.to(device)

    X_t = X[:ntraining] # training
    y_t = y[:ntraining]
    # control samples for scoring (validation of the model)
    # SCORE using the nforecast samples just after the window
    X_s = X[ntraining:ntraining+nscore] # validation
    y_s = y[ntraining:ntraining+nscore]
    # Xp predict on the last nforecast samples was provided

    clfmodel, errort, errorv = trainTorchNet(X_t, y_t, X_s, y_s, input_size,
                                             device, verbose)
    buy = predictDecideBuySell(errort, errorv, clfmodel, Xp)

    if verbose:
        print(" errort : ", errort, " errorv: ", errorv, " buy ", buy)

    return buy

####### Backtesting only
def backtestPredictions(X, y, inputsize, device="cuda",
            verbose=True):
    """
    Make predictions with a sliding window over X, y feature and target classes
    vectors.

    X : historical data X vector
        with all cross feature collumns and standardized
    y : target binary class
        pair of X vector above

    inputsize : number of features in X vector

    returns prediction book dataframe with attributes:
    {prediction time, training error, validation error, predicted direction}

    prediction time is allways in the minute of npredict window
    no delay assumed due computing time
    """
    size = len(X)
    # number of possible predictions with data size : size
    # because forecast are allways array of forecasts
    # defined by the shift made
    nshifts = size-(ntraining+nscore+nforecast)

    if nshifts < 0:
        print('somethings is wrong, array of data too small')
        print('minimum size is ntraining+nscore+nforecast for 1 prediction')
        return

    if verbose:
        print('maximum number of predictions is: ', nshifts+1)

    # sliding window with step of one sample shift
    prediction_book = th.zeros([nshifts, 4], dtype=th.float32)

    time = X.index # time indexes used in the end
    X = X.values.astype(np.float32) # due Float tensors
    y = y.values.astype(np.int64) # due Cross Entropy Loss requeires Tensor Long

    # all to cuda memory
    X = th.tensor(X).to(device)
    y = th.tensor(y).to(device)
    clfmodel = None # no model yet

    j = 0
    for i in progressbar(range(nshifts)):
        # training samples are the first ones
        X_t = X[i:i+ntraining]
        y_t = y[i:i+ntraining]
        # control samples for scoring (validation of the model)
        # SCORE using the nforecast samples just after the window
        X_s = X[i+ntraining:i+ntraining+nscore]
        y_s = y[i+ntraining:i+ntraining+nscore]
        # predict on the last nforecast samples
        X_p = X[i+ntraining+nscore:i+ntraining+nscore+nforecast]
        # return mode, accuracy
        clfmodel, errort, errorv = trainTorchNet(X_t, y_t, X_s, y_s, inputsize,
                                                 device, verbose)
        buy = predictDecideBuySell(errort, errorv, clfmodel, X_p)

        if buy != 0:
            prediction_book[j, 0] = i+npredict
            prediction_book[j, 1] = errort
            prediction_book[j, 2] = errorv
            prediction_book[j, 3] = buy
            j = j+1
        # accurary of the model if higher than 90% we can predict
        if verbose:
            print(i, " errort : ", errort, " errorv: ", errorv, " buy ", buy)

    predictions = prediction_book.numpy() # tensor to numpy array
    predictions = pd.DataFrame(predictions[:j],
        columns=['time', 'terror', 'verror', 'direction'])
    predictions['time'] = time[predictions['time'].values.astype(int)]

    return predictions

# Remember LOSS includes probability of classification (cross-class)
# is not as simple as eclidian error (ms_loss).
#
# So even if MSLOSS might be smaller what matters
# more as metric is the true cross entropy loss.
# Specially because MSLOSS is calculated over the clipped array of classs probibilities.

# TODO: in future
# import numpy as np
# from scipy.stats import entropy
#
# def entropy1(labels, base=None):
#     value,counts = np.unique(labels, return_counts=True)
#     return entropy(counts, base=base)
