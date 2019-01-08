""""Executor of prediction"""
import numpy as np
import pandas as pd
from sklearn import preprocessing
from sklearn.ensemble import RandomForestClassifier, ExtraTreesClassifier
import RF_Prediction as rf_predictor
import RF_Forex as rf_forex
import scipy
import sys
from optparse import OptionParser

parser = OptionParser()
parser.add_option("--timefile", dest="timefile",
                  help="TimeIndexSeries pickle filename")
parser.add_option("--xtrainfile", dest="xtrainfile",
                  help="Xtrain vector pickle filename")
parser.add_option("--ytrainfile", dest="ytrainfile",
                  help="ytrain vector pickle filename")
parser.add_option("--starti", dest="starti",
                  help="start index to process the data given")
parser.add_option("--endi", dest="endi",
                  help="end index to process the data given")

(options, args) = parser.parse_args()

sIndex = pd.read_pickle(options.timefile)
Xtrain = pd.read_pickle(options.xtrainfile)
ytrain = pd.read_pickle(options.ytrainfile)
starti = int(options.starti)
endi = int(options.endi)


# change here if you wish OPTIONS
shift=60
nforecast=shift
ntraining = 8*60 # 8 hours before for training
nwindow = nforecast+ntraining 
estimators=800
#span=2*60 
#dt=60

# sliding window with step sample of shift   
prediction_book = pd.DataFrame(index=np.arange(endi-starti), columns=['tindex', 'buy'])
# closed at left
for i in rf_predictor.progressbar(np.arange(starti, endi)):    
    
    index_window = sIndex[i:i+nwindow]
    # train using the window samples
    X_trainFolds = Xtrain[i:i+nwindow-nforecast]
    y_trainFolds = ytrain[i:i+nwindow-nforecast]
    
    # test using the samples just after the window       
    X_testFold = Xtrain[i+nwindow-nforecast:i+nwindow+1]
    #y_testFold = ytrain[i+nwindow-nforecast:i+nwindow+1] 

    clfmodel = ExtraTreesClassifier(n_estimators=estimators, n_jobs=4, bootstrap=True) 
    clfmodel.fit(X_trainFolds, y_trainFolds)
    prediction = clfmodel.predict(X_testFold)
    
    down = abs(prediction.sum()-len(prediction))/len(prediction)
    up = abs(1-down)
    if up > 0.8 or down > 0.8: # only if certain by 80% that will go up or down
        # where will it be bought in time
        # allways +5 minutes after prediction
        tindex = index_window.iloc[5-nforecast]
        buy = up > down
        if buy: # buy
            buy = 1
        else: # sell
            buy = -1
        prediction_book.loc[i-starti] = [tindex, buy]
    del clfmodel

# save recommended orders from starti to endi
prediction_book.to_pickle('prediction_book_'+str(starti)+'_'+str(endi))
#    else: # there will be no order just continue to the next analysis
#        continue   
#clfmodel.fit(Xtrain.tail(window_size), ytrain.tail(window_size))
#prediction = clfmodel.predict(pp.topredict)