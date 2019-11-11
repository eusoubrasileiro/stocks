# Making Bars 1M from Ticks to be used on backtesting
# 'Every tick based on real tick'
#
# Creates *.csv to be loaded on MT5 Symbol:
#    - 1M Bars
#    - Ticks
#
# - To fix creation of fake ticks by Metatrader 5 (everytick)
#   - Will remove
#       - start of day ticks without volume - only requotes
#       - end of day ticks without volume - only requotes
#  - Mt5 creates new ticks because of those
#       using the stupid 'EveryTick' methodology
#
#  - After market also data cannot be included
#  - MT5 also creates fake ticks because of empty bars (in the end of day)
#     - created because there is a gap between end of market and after market
#     - get minute bounds of days when there is the last non 0 volume bar
#     - use those bounds to clip Ticks already cleaned-up
#
#   Note:
#   OHLC are on executed prices (deals) that means tick.last when volume > 0
#   Otherwise bars would be crazy up and down on ask or bid nonsense.
#
#

import pandas as pd
import numpy as np
import datetime
import argparse
import util

parser = argparse.ArgumentParser()
parser.add_argument("--file", "-f", type=str, required=True)
parser.add_argument("-mqltick", "-m", dest="mqltick", default=False, action="store_true",
                    help="also creates binary file of array of mt5 mqltick suffix _mql5tick.bin")
args = parser.parse_args()
tickfileinput = args.file

ticks, bars = util.ticksnbars(tickfileinput, verbose=True)
outfilename = tickfileinput.split('.')[0]
print("Writting files...")
util.writecsv_tickmt5(ticks, outfilename)
util.writecsv_m1barmt5(bars, outfilename)
if args.mqltick:
    print("Writting mqltick Binary Array")
    util.writebin_mql5ticks(ticks, outfilename)
print('Done')
