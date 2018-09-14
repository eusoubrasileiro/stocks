### Progress Report

May 2018. Started using code from Forex experiments focusing on Ibovespa stocks specially PETR4

1. Learned how to download stock using Metatrader 5 in *.csv format.
2. Using 60 minutes shift in time and ideas from Russian post on Metatrader started testing with Petrobras.
3. Wrote misleading code to make backtesting using `ExtraTreeClassifier` from `sktlearn`
4. Time for backtesting was too slow (altough I was multithreading) so I decided to test Tensorflow thinking about GPU use.
5. Created code for downloading ibovespa stocks data in minute time frame and converting it to pandas dataframe using Metatrader 5.
6. Tensorflow was fast but pytorch was easier and cleaner so I moved.
7. Created a efficient fast backtesting engine in Cython (can be ported to numba?!).
8. Results were promissing with 120 minutes time shift and local models using past 1 week data.
9. Wrote `ExpertAdvisor.mq5`. Backtesting inside Metatrader 5 results were increadible using predictions file. Also wrote `daimonPredictor.py` to create predictions to advisor in real time.
10. When backtesting on Rico Demo Account found the biggest errors of all, predictions created by pytorch code were shift in time to the past 120 minutes. So everything were wrong!

September 2018. Starting again. 

**Lessons learned:** 
- Write good unit tests, specially testing time of prediction. Use Python 3 API.
- Don't spend much time with prototype notebooks. That means you are losing focus and objective. Instead write python modules from notebooks using the knowledge learned. 

1. Wrote code to fit global NN on all 5 years data using 2 hours shift. Removed samples overlapping days, 2 hours in the morning and 2 hours before session end - day trade is the objective.  
 
