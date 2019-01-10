import unittest
import numpy as np
import time
import os
datapath = r"C:\Users\alferreira\Documents\stocks\data"
testingpath = r"C:\Users\alferreira\Documents\stocks\algos\tests"

if __name__ == '__main__':
    unittest.main()

class Tests(unittest.TestCase):
    def command_line(self):
        structsize = 8*7+4
        fin = open(os.path.join(datapath,'WIN@M1.mt5bin'), 'rb')
        bars = fin.read(structsize*4200)
        for i in range(50000):
            bar = fin.read(structsize)
            bars = bars+bar
            bars = bars[structsize:]
            while(True):
                try:
                    fout = open(os.path.join(testingpath, 'WIN@RTM1.mt5bin'), 'wb')
                    result = fout.write(bars)
                    if result == len(bars):
                        break
                except:
                    continue
            fout.close()
            time.sleep(1)
        fin.close()
