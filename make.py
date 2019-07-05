#!/usr/bin/env python
import subprocess
from argparse import ArgumentParser

mt5advpath = r"/home/andre/.wine/drive_c/Program\ Files/Rico\ MetaTrader\ 5/MQL5/Experts/Advisors/"
mt5cmpath = r"/home/andre/.wine/drive_c/users/andre/Application\ Data/MetaQuotes/Terminal/Common/Files/"

parser = ArgumentParser()
parser.add_argument("-meta5", dest="meta5", default=False, action="store_true",
                    help="link all *.mql and *.mqh files to MetaTrader 5 Advisors Path")
parser.add_argument("-clean", dest="clean", default=False, action="store_true",
                    help="clean all symlinks")
parser.add_argument("-newdata", dest="newdata", default=False, action="store_true",
                    help="copy new Metatrader 5 binary files to data folder")

args = parser.parse_args()

if args.clean:
    subprocess.call("""cd '/home/andre/.wine/drive_c/Program Files/MetaTrader 5/MQL5/Experts/Advisors';
                        find . -type l -exec unlink \{\} \;""", shell=True)

if args.meta5:
    subprocess.call("""cd '/home/andre/.wine/drive_c/Program Files/MetaTrader 5/MQL5/Experts/Advisors';
                        find . -type l -exec unlink \{\} \;""", shell=True)
    # make symlinks from stocks folder to Metatrader folder
    subprocess.call("""cd '/home/andre/.wine/drive_c/Program Files/MetaTrader 5/MQL5/Experts/Advisors';
                ln -s /home/andre/Projects/stocks/mt5/*.mq* .""", shell=True)

if args.newdata:
    subprocess.call("""cd '/home/andre/.wine/drive_c/Program Files/MetaTrader 5/MQL5/Files';
                    cp * /home/andre/Projects/stocks/data """, shell=True)

# daemon:
# 	python -m algos.mt5daemons.buybandsd.py
