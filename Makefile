PY := python

cython:
	$(PY) setup.py build_ext --inplace
	#python setup.py build_ext --inplace --cython
	# make symlinks from stocks folder to Metatrader folder
	rm /home/andre/.wine/drive_c/Program\ Files/Rico\ MetaTrader\ 5/MQL5/Experts/Advisors/*.mq*
	ln -s /home/andre/PycharmProjects/stocks/Tools/*.mq* /home/andre/.wine/drive_c/Program\ Files/Rico\ MetaTrader\ 5/MQL5/Experts/Advisors/
