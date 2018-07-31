"""
Load *.mt5bin data exported from Metatrader 5 as binary Mqlrates Array
The entire data for PETR4 for example has 65 MB from 2008 to 2018.

Minute data.

There might be missing minutes, no workaround found to recover every minute.
There will be allways some minute bar missing.

$DOL data starts only on 2013 in XP servers

"""

import pandas as pd
from scipy import stats
import glob
import datetime
import numpy as np
import os
from pathlib import Path

def Report_Missing(df):
    """print the number of missing minutes in the df data set"""
    # Calculate last minute of operation for each day in `df`
    datetimes = pd.DataFrame(df.index.values, columns=['datetime'])
    datetimes['date'] = datetimes.datetime.apply(lambda x: x.date())
    datetimes['time'] = datetimes.datetime.apply(lambda x: x.time())
    #datetimes.head(1)
    date_last_trade_time = datetimes.groupby('date').max().apply(
    (lambda x: datetime.datetime.combine(x[0], x[1])),
    axis=1)
    date_first_trade_time = datetimes.groupby('date').min().apply(
    (lambda x: datetime.datetime.combine(x[0], x[1])),
    axis=1)
    missing_count = 0
    for first_trade_time, last_trade_time in zip(date_first_trade_time, date_last_trade_time):
        dt = (last_trade_time-first_trade_time)
        timerange = first_trade_time+np.arange(0, 1+dt.seconds/60, 1)*datetime.timedelta(minutes=1)
        currentminutes = len(df[df.index.isin(timerange)])
        expectedminutes = len(timerange)
        if currentminutes < expectedminutes:
            #raise Exception('Missing data at ', first_trade_time.date(), ' missing ',
            #                expectedminutes-currentminutes, ' minutes')
            missing_count += expectedminutes-currentminutes
    print('percentage of missing minute data {: .2%}'.format(missing_count/len(df)))


def RemoveDays(minutes=4*60):
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
SYMBOLS = None
""" pandas dataframe all symbols loaded stored here """
masterdf = None


def Set_Data_Path(_path_data_bundle, _path_bin_data):
    """ set data and binary data path before loading or if already loaded """
    global path_bin_data
    global path_data_bundle
    global masterdf
    global SYMBOLS
    path_data_bundle = _path_data_bundle
    path_bin_data = _path_bin_data

    ### Try to load already created data from data bundle folder
    masterdf_filepath = os.path.join(path_data_bundle,'masterdf.pickle')
    symbols_filepath  = os.path.join(path_data_bundle,'SYMBOLS.pickle')

    file = Path(masterdf_filepath)
    if file.is_file():
        masterdf = pd.read_pickle(masterdf_filepath)
        print('master data loaded size (minutes)', len(masterdf))

    else:
        print('must load metatrader 5 *.mt5bin files')
        return

    file = Path(symbols_filepath)
    if file.is_file():
        SYMBOLS = pd.read_pickle(symbols_filepath)
        print('Symbols lodaded:')
        print(SYMBOLS)
    else:
        print('couldnt find symbols file')


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

def Load_Meta5_Binary(filename):
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
    df.time = df.time.apply(lambda x: datetime.datetime.utcfromtimestamp(x))
    return df.set_index('time') # set index as datetime

def  Load_Meta5_Data(verbose=True, suffix='M1.mt5bin'):
        """
        Load All *.bin files in the current path_bin_data folder
        Print all symbols loaded.
        """

        global SYMBOLS
        global masterdf

        if (not(masterdf is None)) or (not(SYMBOLS is None)):
            print('Data already loaded')
            return masterdf

        # move to path_bin_data
        os.chdir(path_bin_data)

        # read all suffixed files in this folder
        symbols = []
        dfsymbols = []
        for filename in glob.glob('*'+suffix):
            dfsymbols.append(Load_Meta5_Binary(filename))
            symbol = filename.split(suffix)[0]
            symbols.append(symbol)

        SYMBOLS = pd.Series(symbols)
        indexes = [pd.DataFrame(index=df.index) for df in dfsymbols]
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
        masterdf.drop('S', axis=1, inplace=True) # useless so far S=Spread

        RemoveDays() # remove useless days for training less than xx minutes

        if verbose:
                print('Symbols lodaded:')
                print(SYMBOLS)
                Report_Missing(masterdf)

        # move to data_bundle_path and save data
        os.chdir(path_data_bundle)
        SYMBOLS.to_pickle('SYMBOLS.pickle')
        masterdf.to_pickle('masterdf.pickle')

        return masterdf


# collum mapping which collums are from which symbol
def get_symbol(symbol):
    """
    get the corresponding collumns for that symbol
    consider the following collumns for each symbol
     [OPEN-HIGH-LOW-CLOSE-TICKVOL-VOL]
    """
    global masterdf
    ifirst_collumn = SYMBOLS[SYMBOLS==symbol].index[0]*6
    return masterdf.iloc[:, ifirst_collumn:ifirst_collumn+6]


def FixedColumnNames():
    """
    Create Data Frame for random forest up down prediction
    targetquote is the target symbol
    #percentdata how much data to use default 15%
    prediction will train and work on 'CLOSE' default
    """
    global masterdf

    df = masterdf.copy()
    #df = df[:int(len(df)*percentdata*0.01)]
    # new collumn names otherwise create_indicators break
    # [OPEN-HIGH-LOW-CLOSE-TICKVOL-VOL]
    # O-H-L-C-T-V colum suffixes
    newnames = [ SYMBOLS[i]+'_'+masterdf.columns[j][0]
            for i in range(len(SYMBOLS)) for j in range(6) ]
    df.columns = newnames
    # we will work with close price just because we want.. no reason whatsoever
    # just add one collum with the target variable
    #df['target'] = get_symbol(targetquote)[prediction_on]

    return df
