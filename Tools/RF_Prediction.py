import numpy as np
import matplotlib.pyplot as plt
import pandas as pd
from sklearn import preprocessing
from sklearn.ensemble import RandomForestClassifier, ExtraTreesClassifier
from . import RF_Forex  as rfforex
import scipy
import sys
import os

def progressbar(it, prefix="", size=80):
    count = len(it)
    def _show(_i):
        x = int(size*_i/count)
        sys.stdout.write("%s[%s%s] %i/%i\r" % (prefix, "#"*x, "."*(size-x), _i, count))
        sys.stdout.flush()

    _show(0)
    for i, item in enumerate(it):
        yield item
        _show(i+1)
    sys.stdout.write("\n")
    sys.stdout.flush()

class PreparePredictionData:
    """Prepare Data for random forest classificication and prediction"""
    def __init__(self, dfquotes, targetquote, dt=10):
        self.targetquote = targetquote
        self.dfquotes = dfquotes
        self.srtarget = dfquotes[targetquote] # target serie
        self.dt = pd.Timedelta(dt, unit='s') # sampling rate in seconds
        self.bkquotes = dfquotes.copy()
        self.time_index = self.bkquotes.index
    
    def viewquote(self, quote=-1):
        if quote == -1:
            quote = self.targetquote
        pyplot.figure(figsize(15,3))
        self.dfquotes[self.targetquote].plot()
        # store this serie for use at the end
        
    def create_binary_zig_zag_class_sm(self, span=10, smooth=3, plot=True):

        
        self.srpivots = rfforex.create_binary_zig_zag_class_sm(
            self.dfquotes, self.targetquote, span, smooth, plot=plot)
        # create the binary classification class "du" withoutshift (down/up - 0/1)
        self.srdu = self.dfquotes['du'] # save a copy for future use
        
        # create statistics of ups/downs, find p90 and p50
        # calculate delta times for statistics
        deltas = pd.Series([self.srpivots.index[i+1]-self.srpivots.index[i] 
                            for i in range(self.srpivots.index.size-1)])
        # convert to minutes
        deltas = deltas.apply(lambda x : x.total_seconds()/60.) 
        deltas = deltas.values        
            
        # percentiles of deltas beween ups and downs < reversed P99 in fact is  percentile of 1
        self.percentiles = np.percentile(deltas, [1, 10,  50,  90,  99])
        
        
        if plot:
            plt.figure(figsize=(8,3))
            nna = plt.hist(deltas, bins=80, normed=True);
            #plt.plot(np.arange(0, xmax, 0.01), scipy.stats.expon.pdf(np.arange(0, xmax, 0.01), shape, loc))
            #plt.xlim(0, xmax)
            print("zig zag times (minutes) >> 1% 10% 50% 90% 99% ")
            np.set_printoptions(precision=2)
            # store percentiles for future use          
            print(self.percentiles[::-1])
            print("90 percent data trends with length P90 > %.2f minutes"%(self.percentiles[1]))
        
    def create_binary_zig_zag_class(self, ptsvariation, plot=True, lasttrend=False):
        """ALSO BROKEN THE PLOT"""
        self.srpivots = rfforex.create_binary_zig_zag_class(
            self.dfquotes, self.targetquote, ptsvariation, plot=plot, lasttrend=lasttrend)
        # create the binary classification class "du" withoutshift (down/up - 0/1)
        self.srdu = self.dfquotes['du'] # save a copy for future use
        
        # create statistics of ups/downs, find p90 and p50
        # calculate delta times for statistics
        deltas = pd.Series([self.srpivots.index[i+1]-self.srpivots.index[i] 
                            for i in range(self.srpivots.index.size-1)])
        # convert to minutes
        deltas = deltas.apply(lambda x : x.total_seconds()/60.) 
        deltas = deltas.values        
            
        # percentiles of deltas beween ups and downs < reversed P99 in fact is  percentile of 1
        self.percentiles = np.percentile(deltas, [1, 10,  50,  90,  99])
        
        
        if plot:
            plt.figure(figsize=(8,3))
            nna = plt.hist(deltas, bins=80, normed=True);
            #plt.plot(np.arange(0, xmax, 0.01), scipy.stats.expon.pdf(np.arange(0, xmax, 0.01), shape, loc))
            #plt.xlim(0, xmax)
            print("zig zag times (minutes) >> 1% 10% 50% 90% 99% ")
            np.set_printoptions(precision=2)
            # store percentiles for future use          
            print(self.percentiles[::-1])
            print("90 percent data trends with length P90 > %.2f minutes"%(self.percentiles[1]))
        
    def make_shifted_binary_zig_zag_class(self, shift=-1, plot=True):
        # for prediction shift samples to left
        # samples that cannot be used for training are 
        # set NAN. Those samples correlate to future times 
        # those we dont know YET that we will predict then
        if shift == -1: # use default based on pivots distribution statics in time
            # percentiles[1] P90 provite minimum needed time from 
            # pivots define up/down change of trend
            delta_buy = 10*60 # additional time to perform setup of buying?
            shift = delta_buy+self.percentiles[1]*60.
            shift = int(np.ceil(shift/self.dt.total_seconds())) 
            
        if plot:
            print('number of samples shifted ', shift)
            print('shift %.1f (minutes)'%(shift*self.dt.total_seconds()/60.))
        
        self.shift = shift
        # create the binary classification class "du" with shift (down/up - 0/1)        
        self.srdup = rfforex.make_shift_binary_class(self.dfquotes, self.shift, plot=plot) 
        # store some new stuff
        # time of the last valid sample for traininig
        # and last sample recorded
        self.time_index_training = self.srdup.dropna(inplace=False).index  
        self.last_time_real = self.srdup.index[-1] 
        self.last_time_training = self.srdup.dropna(inplace=False).index[-1] 
    
    def create_indicatores(self):
        # remove the target_quote/target_variable and create many indicators
        # can improve here with new indicators, RSI discrete, BB discrete
        rfforex.create_indicators(self.dfquotes, self.targetquote)
        
    def normalize(self):
        # normalize 0 to 1
        self.dfquotes = self.dfquotes.apply(lambda x: (x - np.min(x)) / (np.max(x) - np.min(x)))
        
    def calculate_corr_and_remove(self, verbose=True):
        # print 
        # 5 first and 5 last correlations
        # calculate correlation with class to predict remove < 5% correlation
        rfforex.calculate_corr_and_remove(self.dfquotes, self.srdup, verbose=verbose)
    
    def prepare_training(self):
        # remove time index to make it possible through sckit-learn
        self.dfquotes.reset_index(inplace=True, drop=True)
        self.srdup.reset_index(inplace=True, drop=True)
        self.dfquotes['target_variable_dup'] = pd.Series(self.srdup, 
                                                         index=self.dfquotes.index)
        #print(self.dfquotes.head(1))
        # separate data for prediction and for training
        self.topredict = self.dfquotes[ pd.isnull(self.dfquotes['target_variable_dup']) ]         
        self.dfquotes.dropna(inplace=True)
        self.topredict = self.topredict.copy() # avoid stupid warning from pandas
        self.topredict.drop('target_variable_dup', axis='columns', inplace=True)
        self.topredict.dropna(inplace=True)
        # two groups of vectors training and prediction
        Xtrain, ytrain = rfforex.prepareDataForClassification(self.dfquotes) 
        return Xtrain, ytrain, self.topredict       

    def get_astraining(self):
        self.dfquotes['target_variable_dup'] = self.srdup
        # separate data for training
        self.dfquotes.dropna(inplace=True)
        # return index of time for control
        Index = self.dfquotes.index    
        # remove time index to make it possible through sckit-learn
        self.dfquotes.reset_index(inplace=True, drop=True)    
        # two groups of vectors training and prediction
        Xtrain, ytrain = rfforex.prepareDataForClassification(self.dfquotes) 
        return Index, Xtrain, ytrain

    # def train_parallel(ncores=6):
    #  """   
    #     ## split for 6 cores
    #     will create 6 prediction books
    #     #### Create Script BAT for running ncores python instances
    #     still not that fast because the classifier is very cpu consuming. 
    #     would be faster if could be trained on GPU
    # """
    # # Params
    # shift=60
    # nforecast=shift
    # ntraining = 8*60
    # nwindow = nforecast+ntraining  # traininig + forecasting window
    # estimators=800  
    # dt=60
    # batch_size = int(np.floor(len(sIndex)/ncores))
    # # samples to complete the non integer division
    # # will be added in the final batch of samples
    # nmiss = (len(sIndex) - batch_size*ncores) 
    # # last batch will have more sample to complete the non integer division
    # slices = np.array([i*batch_size for i in range(1,7)])
    # slices[-1] += nmiss
    # # each batch must have the previous sample size of training + forecast window
    # istarts = np.append(0, slices[:-1]-nwindow)
    # iends = slices
    # # first and last batch have different size of samples
    # print(istarts)
    # print(iends)
    # print(iends-istarts)
    # #python Predict.py --xtrainfile ..\training_data\Xtrain_data.pickle 
    # #--ytrainfile ..\training_data\ytrain_data.pickle 
    # #--timefile ..\training_data\sIndex_data.pickle --starti 0 --endi 3752

    # try:
    #     os.remove('paralalel.bat')
    # except:
    #     pass
    # cmd = (r"start /B C:\Anaconda3\python.exe Predict.py --xtrainfile " + xtrain_file + ' '
    #        "--ytrainfile " + ytrain_file + ' '
    #        "--timefile " + index_file + ' '
    #        " --starti {0} --endi {1}\n")
    # with open('paralalel.bat', 'w') as f:
    #     for i, j in zip(istarts, iends):
    #         f.write(cmd.format(i, j))
    #     f.close()
