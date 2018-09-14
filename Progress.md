### Progress Report

May 2018. Started using code from Forex experiments focusing on Ibovespa stocks specially PETR4

1. Learned how to download stock using Metatrader 5 in *.csv format.
2. Using 60 minutes shift in time and ideas from Russian post on Metatrader started testing with Petrobras.
3. Wrote misleading code to make backtesting using `ExtraTreeClassifier` from `sktlearn`
4. Time for backtesting was too slow (altough I was multithreading) so I decided to test Tensorflow.
5. Created code for downloading ibovespa stocks data in minute time frame and converting it to pandas dataframe using Metatrader 5.
6. Tensorflow was fast but pytorch was easier and cleanner so I moved.
7. Created a efficient fast backtesting engine in Cython (can be ported to numba?!).
 
