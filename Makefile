MT5EXPATH = /home/andre/.wine/drive_c/Program\ Files/Rico\ MetaTrader 5/MQL5/Experts/Advisors/
MT5CMPATH = /home/andre/.wine/drive_c/users/andre/Application\ Data/MetaQuotes/Terminal/Common/Files/

meta5:
	for f in /home/andre/Projects/stocks/mt5/*.mq*; do ln -s $f "/home/andre/.wine/drive_c/Program Files/Rico MetaTrader 5/MQL5/Experts/Advisors/"`basename $f`; done
	# make symlinks from stocks folder to Metatrader folderW
clean:
	for f in `echo ${MT5EXPATH}*.mq*`; do unlink $f; done

# test is
# python -m algos.mt5daemons.buybandsd.py

all: clean meta5
