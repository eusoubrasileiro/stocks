"""
Load *.mt5bin data exported from Metatrader 5 as binary Mqlrates Array
The entire data for PETR4 for example has 65 MB from 2008 to 2018.

Minute data.

There might be missing minutes, no workaround found to recover every minute.
There will be allways some minute bar missing.

$DOL/@WIN data starts only on 2013 in XP servers

"""

import pandas as pd
from scipy import stats
import glob
import datetime
import numpy as np
import os
import sys
from pathlib import Path

def calculateMissing(odf):
    """
    Calculate the percentage of missing minutes in the df data set
    Consider first and last minute as boundaries
    """
    df = odf.copy()
    # Calculate last minute of operation for each day in `df`
    df.loc[:, 'time'] = np.nan
    df.loc[:, 'time'] = df.index.astype(np.int64)//10**9 # (to unix timestamp) from nano seconds 10*9 to seconds
    days = df.groupby(df.index.date)['time'].agg(['min', 'max', 'count']) # aggreagate on groupby
    # total number of minutes on the day
    totalminday = (days['max']-days['min'])//60
    # minutes with data by day
    countminday = days['count'] # -1 due count is +1
    missminday = totalminday-countminday
    percmissminday = missminday/totalminday

    # print('not working on daemon just on jupyter notebook!!!')
    return np.mean(percmissminday) # average of missing minutes

def removeDays(minutes=4*60):
    """remove days with less than minutes data"""
    global masterdf
    masterdf['data'] = masterdf.index.date
    days = []
    for day, group in masterdf.groupby(masterdf.data):
        if len(group) < minutes: # len is number of minutes
            continue
        days.append(day)
    masterdf = masterdf.loc[masterdf.data.isin(days)]
    masterdf.drop('data', axis=1, inplace=True)


"""path to data already loaded"""
path_data_bundle = ""
"""path to *.bin metatrader5 - 1 minute data"""
path_bin_data = ""
""" all symbols loaded (stocks or currency) """
symbols = None
""" pandas dataframe all symbols loaded stored here """
masterdf = None

def loadExistent(verbose=True):
    global masterdf
    global symbols
    ### Try to load already created data from data bundle folder
    masterdf_filepath = os.path.join(path_data_bundle,'masterdf.pickle')
    symbols_filepath  = os.path.join(path_data_bundle,'symbols.pickle')
    filem = Path(masterdf_filepath)
    files = Path(symbols_filepath)
    if filem.is_file():
        masterdf = pd.read_pickle(masterdf_filepath)
        if verbose:
            print('Master data loaded size (minutes): ', len(masterdf), file=sys.stderr)
    else:
        if verbose:
            print('Must load metatrader 5 *.mt5bin files!', file=sys.stderr)
        return
    if files.is_file():
        symbols = pd.read_pickle(symbols_filepath)
        if verbose:
            print('symbols loaded:', file=sys.stderr)
            print(symbols.values[:], file=sys.stderr)
    else:
        if verbose:
            print('Couldnt find symbols file', file=sys.stderr)

def setDataPath(_path_data_bundle, _path_bin_data, preload=True, verbose=True):
    """ set data and binary data path before loading or if already loaded """
    global path_bin_data
    global path_data_bundle
    path_data_bundle = _path_data_bundle
    path_bin_data = _path_bin_data
    if preload:
        loadExistent(verbose)

## MQL5 MqlRates struct all data comes as a an array of that
#struct MqlRates
#  {
#   datetime time;         // Hora inicial do período 8 bytes
#   double   open;         // Preço de abertura
#   double   high;         // O preço mais alto do período
#   double   low;          // O preço mais baixo do período
#   double   close;        // Preço de fechamento
#  long     tick_volume;  // Volume de Tick
#   int      spread;       // Spread
#   long     real_volume;  // Volume de negociação
#  };

def loadMeta5Binary(filename):
    dtype = np.dtype([
        ("time", np.int64),
        ("O", np.float64),
        ("H", np.float64),
        ("L", np.float64),
        ("C", np.float64),
        ("TV", np.int64),
        ("S", np.int32),
        ("RV", np.int64)
    ])
    data = np.fromfile(filename, dtype=dtype)
    df = pd.DataFrame(data)
    ## convert from unix time (meta5) 8 bytes to datetime utc
    #df.time = df.time.apply(lambda x: datetime.datetime.utcfromtimestamp(x))
    df.time = pd.to_datetime(df.time.values, unit='s')
    return df.set_index('time') # set index as datetime

def loadMeta5Data(verbose=True, suffix='M1.mt5bin', cleandays=True, preload=True):
    """
    Load All *.bin files in the current path_bin_data folder
    Use defined suffix. Print all symbols loaded.

    cleandays : remove days with less than x-hours of trading
    preload : reuse previous lodaded data

    Return number of symbols loaded.
    """

    global symbols
    global masterdf

    if preload: # use preloaded data
        loadExistent(verbose)
        if (not(masterdf is None)) and (not(symbols is None)):
            print('Using previous loaded data!', file=sys.stderr)
            return len(symbols)

    # move to path_bin_data
    os.chdir(path_bin_data)

    # read all suffixed files in this folder
    symbols = []
    dfsymbols = []
    for filename in glob.glob('*'+suffix):
        dfsymbols.append(loadMeta5Binary(filename))
        symbol = filename.split(suffix)[0]
        symbols.append(symbol)

    symbols = pd.Series(symbols)
    indexes = [pd.DataFrame(index=df.index) for df in dfsymbols]

    if len(indexes) > 0: # more than one symbol
        # get the intersecting indexes
        # remove if exist (some case there are) duplicated indexes
        inner_index = indexes[0]
        for index in indexes[1:]:
            inner_index = inner_index.join(index, how='inner')
        inner_index = inner_index.index.drop_duplicates()
        # apply inner_index on all data -- all indexes not in the inner list of indexes are dropped
        for i in range(len(symbols)):
            dfsymbols[i] = dfsymbols[i][~dfsymbols[i].index.duplicated(keep='first')]
            dfsymbols[i] = dfsymbols[i].loc[inner_index] # get just the intersecting indexes

        ### The Master DataFrame with
        #### AMAZING MULTINDEX FOR LABELS --- will be left for the future for while
        #iterables = [symbols,['OPEN', 'HIGH', 'LOW', 'CLOSE', 'TICKVOL', 'VOL', 'SPREAD']]
        #index = pd.MultiIndex.from_product(iterables, names=['SYMBOL', 'ATRIB',])
        #index
        #masterdf.columns = index
        #masterdf.head(1)
        #masterdf.drop([0, 1], axis=0, level=0)
        #masterdf.drop('SPREAD', axis=1, level=1).head(1)

        masterdf = pd.concat(dfsymbols, axis=1)
        # masterdf.drop('S', axis=1, inplace=True) # useless so far S=Spread
        # masterdf.dropna(inplace=True)

    if cleandays:
        removeDays() # remove useless days for training less than xx minutes

    if verbose:
        print('symbols loaded:', file=sys.stderr)
        print(symbols.values[:], file=sys.stderr)
        print("percent missing: ", calculateMissing(masterdf), file=sys.stderr)

    # move to data_bundle_path and save data
    os.chdir(path_data_bundle)
    symbols.to_pickle('symbols.pickle')
    masterdf.to_pickle('masterdf.pickle')

    return len(symbols)


# collum mapping which collums are from which symbol
def getSymbol(symbol):
    """
    get the corresponding collumns for that symbol
    consider the following collumns for each symbol
     [OPEN-HIGH-LOW-CLOSE-TICKVOL-VOL-SPREAD]
    """
    global masterdf
    if len(symbols) > 1:
        ifirst_collumn = symbols[symbols==symbol].index[0]*7
    else:
        ifirst_collumn = 0
    return masterdf.iloc[:, ifirst_collumn:ifirst_collumn+7]


def simpleColumnNames():
    """
    Create Data Frame with all symbols loaded
    """
    global masterdf

    df = masterdf.copy()
    #df = df[:int(len(df)*percentdata*0.01)]
    # new collumn names otherwise create_indicators break
    # [OPEN-HIGH-LOW-CLOSE-TICKVOL-VOL]
    # O-H-L-C-T-V-S colum suffixes
    newnames = [ symbols[i]+'_'+masterdf.columns[j][0]
            for i in range(len(symbols)) for j in range(7) ]
    df.columns = newnames

    return df
