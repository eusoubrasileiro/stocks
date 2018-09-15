### Progress Report

May 2018. Started using code from Forex experiments focusing on Ibovespa stocks specially PETR4 for day-trade robot.

1. Learned how to download stock using Metatrader 5 in *.csv format.
2. Using 60 minutes shift in time and ideas from Russian post on Metatrader started testing with Petrobras. Used 8 symbols code is `prepareData.py`
3. Wrote misleading code to make backtesting using `ExtraTreeClassifier` from `sktlearn`
4. Time for backtesting was too slow (altough I was using multithreading) so I decided to test Tensorflow aiming GPU boost.
5. Wrote script (`DownloadRates.mq5`) for downloading ibovespa stocks  data using Metatrader 5  (1 minute time frame) and converting it to pandas dataframe (`meta5Ibov.py`).
6. Tensorflow was fast but Pytorch was easier and cleaner so I moved (`torchNN.py`).
7. Created a efficient fast backtesting engine  `backtesting.pyx` (1 minute time-frame) in Cython (should be ported to numba?!) - checked against Metatrader 5 backtesting tool. 
8. Results were promissing with 120 minutes time shift and local models using past 1 week data.
9. Wrote `ExpertAdvisor.mq5`. Backtesting inside Metatrader 5 results were increadible using predictions file. Also wrote `daimonPredictor.py` to create predictions to advisor in real time.
10. When backtesting on Rico Hedge Demo Account found the biggest errors of all, predictions created by pytorch code were shift in time to the past 120 minutes. So results were wrong! Need to study a mathematical model that works (P1, P10, P50, acceptable) for prediction.

September 2018. Starting again. 

**Lessons learned:** 
- Progress Report. 
- Write good unit tests, specially testing time of prediction. Use Python 3 API.  
- Don't spend much time with prototype notebooks. That means you are losing focus and objective. Instead write python modules from notebooks using the knowledge learned.  

1. Wrote code to fit global NN on 5 years data using 2 hours shift. Removed samples overlapping days, 2 hours in the morning and 2 hours before session end - avoiding contamination between days assumption for day trade. After training 80/20 (accuracy of 55% on validation) used the model to predict on the 5 years. Results after backtesting were 33% accuracy.
 
