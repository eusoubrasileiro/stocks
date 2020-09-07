"""
Creates Ticks and 1M Bars cleaned (*.csv) Metatrader 5 format.

--file / -f: file path of Tick file (*.csv)
    Metatrader 5->View->Symbols->
    [Select desired symbol - must be colored yellow]
    Ticks->Export Ticks

-mqltick / -m: [optional]
    additionally creates binary file with Tick in
    Metatrader 5 `MqlTick struct` format
    suffix *_mql5tick.bin

Output:
  Creates 2 *.csv files on same folder path of input file:
   - Ticks [inputfilename]_tickmt5.csv
   - 1M Bars [inputfilename]_m1barmt5.csv

- Can be used to create clean ticks for further processing.
- Used for backtesting 'Every tick based on real tick'.

Metatrader 5 backtesting problems solved using this:

- To fix creation of fake ticks by Metatrader 5 (everytick)
- Mt5 creates new ticks because of those
  using the stupid 'EveryTick' methodology
- MT5 creates fake ticks because of empty bars (in the end of day)

Implementation Details:

- Will remove
  - start of day ticks without volume - only requotes
  - end of day ticks without volume - only requotes
- After market also data cannot be included
Because there is a gap between end of market and after market
- get minute bounds of days when there is the last non 0 volume bar
- use those bounds to clip Ticks already cleaned-up

Reminder:
- OHLC are on executed prices (deals) that means tick.last when volume > 0
Otherwise bars would be crazy up and down on ask or bid nonsense.
"""
#
#

import pandas as pd
import numpy as np
import datetime
import argparse
import util
import os

parser = argparse.ArgumentParser()
parser.add_argument("--file", "-f", type=str, required=True)
parser.add_argument("-mqltick", "-m", dest="mqltick", default=False, action="store_true",
                    help="also creates binary file of array of mt5 mqltick suffix _mql5tick.bin")
args = parser.parse_args()
tickfileinput = args.file
tickfileinput = os.path.abspath(tickfileinput) # get the absolute file path in case

ticks, bars = util.ticksnbars(tickfileinput, verbose=True)
outfilename = tickfileinput.split('.')[0]
print("Writting files...")
util.writecsv_tickmt5(ticks, outfilename)
util.writecsv_m1barmt5(bars, outfilename)
if args.mqltick:
    print("Writting mqltick Binary Array")
    util.writebin_mql5ticks(ticks, outfilename)
print('Done')
