PY := python
MT5EXPATH := /home/andre/.wine/drive_c/Program\ Files/Rico\ MetaTrader\ 5/MQL5/Experts/Advisors
MT5CMPATH := /home/andre/.wine/drive_c/users/andre/Application Data/MetaQuotes/Terminal/Common/Files

data:
	ln -s /home/andre/PycharmProjects/stocks/data/collumns_selected.txt $(MT5CMPATH)
	ln -s /home/andre/PycharmProjects/stocks/data/stocks_stats_2018.csv $(MT5CMPATH)
mt5:
	rm $(MT5EXPATH)/*.mq*
	ln -s /home/andre/PycharmProjects/stocks/*.mq* $(MT5EXPATH)
	ln -s /home/andre/PycharmProjects/stocks/Tools/*.mq* $(MT5EXPATH)
cython:
	$(PY) setup.py build_ext --inplace
	#python setup.py build_ext --inplace --cython
	# make symlinks from stocks folder to Metatrader folder
