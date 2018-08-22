import os
import numpy as np
import pandas as pd
from util import progressbar
import torch as th
import torch.nn.functional as F
import copy
from torch.optim import lr_scheduler

def TorchModelPredict(model, X):
    y_prob = model(X)
    y_pred = th.argmax(y_prob, 1)
    y_pred = y_pred.to("cpu")
    return y_prob, y_pred.data.numpy()

def tensor_shuffle(X, y, size):
    """better use the dataset api in the future"""
    i = np.random.randint(X.shape[0]-size)
    return X[i:i+size], y[i:i+size]

def TrainTorchNet(X, y, X_s, y_s, input_size, model=None, nepochs=30, device="cuda",
                  batch_size=256, verbose=True):
    """
    X, y         : vectors for training
    X_s, y_s     : vector for validation (scoring the model)
    the best model with smaller error in validation will be returned
    after the nepochs of training
    """
    if model == None:
        model = th.nn.Sequential(th.nn.Linear(input_size, 1024), th.nn.ReLU(), th.nn.Dropout(.1),
                                 th.nn.Linear(1024,280), th.nn.ReLU(), th.nn.Dropout(.3),
                                 th.nn.Linear(280, 70), th.nn.ReLU(),  th.nn.Dropout(.05),
                                 th.nn.Linear(70, 2), th.nn.Softmax(dim=1)) # dim == 1 collumns add up to 1 probability
        criterion = th.nn.CrossEntropyLoss()
        model = model.to(device)
        #weight_decay regularization not applied
        optimizer = th.optim.Adam(model.parameters(), lr=0.1) # learning rate optimal

    # Decay LR by a factor of 0.1 every 10 epochs
    scheduler = lr_scheduler.StepLR(optimizer, step_size=15, gamma=0.1)

    best_model = None
    best_lossv = 1000.
    best_error = 100.

    for t in range(nepochs):

        X_t, y_t = tensor_shuffle(X, y, batch_size)

        y_pred = model(X_t)
        # Compute and print loss
        lossy = criterion(y_pred, y_t)
        # Zero gradients, perform a backward pass, and update the weights.
        optimizer.zero_grad()
        lossy.backward()
        optimizer.step()
        # Model Evaluation
        model.eval()
        # calculate error complement of accuracy
        # on training set
        y_pred = th.argmax(y_pred, 1)
        error_train = th.nn.functional.mse_loss(
            y_pred.float(), y_t.float())
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
    best_errorv = best_errorv.to("cpu").item()
    best_errort = best_errort.to("cpu").item()
    model.load_state_dict(best_model)
    #print(best_errort, best_errorv)
    # return the model and accuracy of training and validation
    return model, best_errort, best_errorv

# parameters
nforecast=120
nvalidation=nforecast # to validate the model prior prediction
ntraining = 5*8*60 # 5*8 hours before for training - 1 week
nwindow = nvalidation+nforecast+ntraining

def TorchNNTrainPredictDecide(X, y, X_p, input_size, verbose=False, device='cuda'):
    """
    Using Pytorch train binary classifier NN based on X, y and classify Xp
    Decide if will buy or sell or not do anything.

    * input_size : X.shape[1]

    Return : {1 or 0, score}
        if criteria of model accuracy is reached otherwise returns None
    """
    # training samples are the first ones
    X_t = X[:ntraining]
    y_t = y[:ntraining]
    # control samples for scoring (validation of the model)
    # SCORE using the nforecast samples just after the window
    X_s = X[ntraining:ntraining+nvalidation]
    y_s = y[ntraining:ntraining+nvalidation]

    # return model, accuracy
    clfmodel, errort, errorv = TrainTorchNet(X_t, y_t, X_s, y_s,
                                                   input_size, device=device, verbose=False)
    # accurary of the model if higher than 90% we can predict
    if verbose:
        print(" input size ", input_size)
        print(" Error training : ", errort, " Error validation : ", errorv)
    # we cannot predict unless the model is 90%+ accurate
    # accurate on validation. on training we accept 70%+
    if errorv > 0.1 or errort > 0.3:
        del clfmodel # cleanup memory gac can get it back
        return None

    # decision of buying or selling or nothing
    probability, prediction = TorchModelPredict(clfmodel, X_p)
    down = abs(prediction.sum()-len(prediction))/len(prediction)
    up = abs(1-down)

    if up > 0.9 or down > 0.9: # only if certain by 90% that will go up or down
        buy = up > down
        if buy: # buy
            buy = 1
        else: # sell
            buy = -1
    else:
        del clfmodel # cleanup memory gac can get it back
        return None

    del clfmodel # cleanup memory gac can get it back
    return buy, errorv

def TrainPredict(X, y, Xp, verbose=True):
    """
    X, y training/validation vectors for binary classification class y
    Xp vector for prediction
    """

    times = Xp.index # same times, last time will saved for prediction
    X = X.values.astype(np.float32) # due Float tensors
    Xp = Xp.values.astype(np.float32)
    y = y.values.astype(np.int64) # due Cross Entropy Loss requeires Tensor Long
    input_size = X.shape[1]

    device = th.device("cuda" if th.cuda.is_available() else "cpu")

    X = th.tensor(X)
    X = X.to(device)
    y = th.tensor(y)
    y = y.to(device)
    Xp = th.tensor(Xp)
    Xp = X.to(device)

    return TorchNNTrainPredictDecide(X, y, Xp, input_size, verbose=verbose, device=device)
