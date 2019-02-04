MT5EXPATH := /home/andre/.wine/drive_c/Program\ Files/Rico\ MetaTrader\ 5/MQL5/Experts/Advisors/
MT5CMPATH := /home/andre/.wine/drive_c/users/andre/Application\ Data/MetaQuotes/Terminal/Common/Files/

#####  USE THOSE BELLOW ON BASH. ON MAKEFILE DOESNT WORK!!!!
meta5:
	cd "/home/andre/.wine/drive_c/Program Files/MetaTrader 5/MQL5/Experts/Advisors"
	ln -s ~/Projects/stocks/mt5/*.mq* .
	# make symlinks from stocks folder to Metatrader folder
clean:
	cd "/home/andre/.wine/drive_c/Program Files/MetaTrader 5/MQL5/Experts/Advisors"
	find . -type l -exec unlink {} \;

daemon:
	python -m algos.mt5daemons.buybandsd.py

newdata:
	cp /home/andre/.wine/drive_c/Program\ Files/MetaTrader\ 5/MQL5/Files/* ~/Projects/stocks/data/

all: clean meta5
