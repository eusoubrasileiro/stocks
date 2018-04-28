"""
Load *.CSV data exported from Metatrader 5 symbols

Minute data.

There might be missing minutes, no workaround found to recover every minute. 
There will be allways some minute bar missing.

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


def Load_MetaCSV_Clean(filename, report_missing=True):
    """Load Metatrader 5 csv file data"""
    df = pd.read_csv(filename, sep='<>', delimiter='\t')
    df.columns = df.columns.str.replace('<', '').str.replace('>', '').str.strip()
    df['DATETIME'] = df['DATE']+' '+df['TIME']
    df.drop(columns=['TIME', 'DATE'], inplace=True)
    df.DATETIME = df.DATETIME.apply(lambda x: datetime.datetime.strptime(x, "%Y.%m.%d %H:%M:%S"))
    df.set_index('DATETIME', inplace=True)

    if report_missing:
        Report_Missing(df)

    return df

"""path to data already loaded"""
path_data_bundle = ""
"""path to *.csv metatrader 5 minute data"""
path_csv_data = ""
""" all symbols loaded (stocks or currency) """
SYMBOLS = None 
""" all symbols loaded stored here """
masterdf = None


def Set_Data_Path(pathdatabundle, pathcsvdata):
    global path_csv_data
    global path_data_bundle
    global masterdf
    global SYMBOLS
    path_data_bundle = pathdatabundle
    path_csv_data = pathcsvdata

    ### Try to load already created data from data bundle folder
    masterdf_filepath = os.path.join(path_data_bundle,'masterdf.pickle')
    symbols_filepath  = os.path.join(path_data_bundle,'SYMBOLS.pickle')

    file = Path(masterdf_filepath)
    if file.is_file():
        masterdf = pd.read_pickle(masterdf_filepath)
        print('master data loaded size (minutes)', len(masterdf))

    else:
        print('must load metatrader 5 *.csv files')
        return

    file = Path(symbols_filepath)
    if file.is_file():
        SYMBOLS = pd.read_pickle(symbols_filepath)
        print('Symbols lodaded:')
        print(SYMBOLS)
    else:
        print('couldnt find symbols file')



def  Load_Meta5_Data(verbose=True):
        """
        Load All *.csv files in the current path_csv_data folder
        Print all symbols loaded.
        """

        global SYMBOLS
        global masterdf

        if (not(masterdf is None)) or (not(SYMBOLS is None)):
            print('Data already loaded')
            return masterdf

        # move to path_csv_data
        os.chdir(path_csv_data)
        # parse and fix all saved data from metatrader on this (.) folder turning then
        # each in a dataframe
        dfsymbols=[]
        for filename in glob.glob('*'):
            symbol = filename.split('_M1')[0]
            dfsymbols.append((symbol, Load_MetaCSV_Clean(filename)))    

        SYMBOLS = pd.Series([pair[0] for pair in dfsymbols])

        # get only the intersecting indexes == time records
        indexes = [pd.DataFrame(index=dfsymbol[1].index) for dfsymbol in dfsymbols]
        inner_index = indexes[0]
        for index in indexes[1:]:
            inner_index = inner_index.join(index, how='inner')
        #print([len(index) for index in indexes])
        # apply inner_index on all data -- all indexes not in the inner list of indexes are dropped
        for symbol, dfsymbol in dfsymbols:
            dfsymbol.drop(dfsymbol.index[~dfsymbol.index.isin(inner_index.index)], inplace=True)
        #print([len(dfsymbol[1]) for dfsymbol in dfsymbols])


        ### The Master DataFrame with 
        #### AMAZING MULTINDEX FOR LABELS --- will be left for the future for while
        #iterables = [symbols,['OPEN', 'HIGH', 'LOW', 'CLOSE', 'TICKVOL', 'VOL', 'SPREAD']]
        #index = pd.MultiIndex.from_product(iterables, names=['SYMBOL', 'ATRIB',])
        #index
        #masterdf.columns = index
        #masterdf.head(1)
        #masterdf.drop([0, 1], axis=0, level=0)
        #masterdf.drop('SPREAD', axis=1, level=1).head(1)

        masterdf = pd.concat([dfsymbol[1] for dfsymbol in dfsymbols], axis=1)
        masterdf.drop('SPREAD', axis=1, inplace=True) # useless so far

        if verbose:
                print('Symbols lodaded:')
                print(SYMBOLS)

        # move to data_bundle_path and save data
        os.chdir(path_data_bundle)	
        SYMBOLS.to_pickle('SYMBOLS.pickle')
        masterdf.to_pickle('masterdf.pickle')

        return masterdf


# collum mapping which collums are from which symbol
def get_symbol(symbol):
    """
    get the corresponding collumns for that symbol
    consider the following collumns for each symbol [OPEN-HIGH-LOW-CLOSE-TICKVOL-VOL]
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
    newnames = [ SYMBOLS[i]+'_'+masterdf.columns[j][0] for i in range(len(SYMBOLS)) for j in range(6) ]
    df.columns = newnames
    # we will work with close price just because we want.. no reason whatsoever
    # just add one collum with the target variable
    #df['target'] = get_symbol(targetquote)[prediction_on]

    return df
