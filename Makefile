PY := python

cython:
	$(PY) setup.py build_ext --inplace
	#python setup.py build_ext --inplace --cython
	ln -s /home/andre/PycharmProjects/stocks/Tools/*.mq* /home/andre/.wine/drive_c/Program\ Files/Rico\ MetaTrader\ 5/MQL5/Experts/Advisors/
