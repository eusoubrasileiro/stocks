
### Support and resistence levels

#### Theoretical bases for support and resistence levels:

- Discrete economic hypotheses or scenarios produces deterministic finite price levels.
- Human emotional behavior, psychology of fear and greed, produces price levels according to Fibonacci.

Future behavior are easier to predict on resistance levels, according to `Wikipedia stock prediction`. Once you label and training using past data? Use Fibonacci retracement on volume. Most day trade or strategies used by traders are based on resistance levels on different time frames (my experience trading/reading thousand blogs)!

- [x] Todo: Implement candle pattern next-day predictor. Implement next day prediction up or down based on 1 century historical data. Remember to set to unknown class classified with less than 0.6 certainty. Probably not an up or an down.
  - Implemented and got interesting preliminary results using dow-jones one century of historical daily Open, High, Low, Close and Volume. The data preparation I believe was essential. I describe bellow:
  1. Removed nan values or all equal O, H, L, C values.
  2. 'Corrected' inflation by applying log function on all.
  3. 'Corrected' the noise effects, 'long wave-lengths'. What I don't care are monthly, yearly time-scale variations of price by removing a EMA of 15 days. I am only interested on daily to 5/6 days price variations.
  4. Calculated Up or Down binary class based on the Close price
  5. Scaled that with `RobustScaler` clipping (10%, 90%) already creates mean 0. and stdev 1.
  6. Made feature vector using 5 previous days and concatenating it with a total size of 25.
  7. Create target vector/class with the following day direction 1 or 0.
  8. Trainned Torch binary classifier with different scenarios:
    1. 3 to 6 layers
    2. 1024 to 200 neurons in each layer
    3. 10, 12, 5 years for training
    4. 1 year, 6 months for validation
    5. different start year/month/day for each training/validation window
    5. Many tests with different parameters: patience, learning rate, optimizer, gamma, number of epochs.
  9. Results averaged from 58% to 75% on training set. Very Encouraging! Certainty much better than candle pattern.
  10. Tried to use a cut-off  to overcome the problem of having an unprobable prediction, too close to 0.5 for class 0 or 1. Results were interesting but need to test more. Another possible reason for this working is the 3rd class not presend that is when did not go Up neither Down.
  11. Need to properly cross-validade it and maybe do a grid-search for parameters, network topology, number of previous days to use etc. Need also focus on the recent years data.
  12. Need to test if this works for Petrobras 2000 to 2018 data I have. Or Bovespa from 1993 to today from Metatrader 5.
  13. New tests using EMA of 2/3 days and 22 previous has given an average from 58 to 80% of accuracy on the validation set. Now using `QuantileTransform` since it already clips outliers.

- [x] After some tentatives found better use WIN future contracts with 2 hours time-frame, corrected by discount downloaded with Metatrader 5. Altough data just from 2013, very realiable and exactly what I want to operate with real money.
  - Found similar results as above, best using 2 hours EMA/SMA.
  - Note: depending on the window/fold size, training and validation samples different results can be achieved in the cross-validation. Also when prediction further ahead of the window/fold the accuracy also varies a lot. It seams that farther from the fold window worse is the prediction accuracy. That need to be better understood.  
    1. Using 5 years for training set and 0.9 ratio for validation, ~ 132 days. I run CV on entire data-set using local classifiers with [256, 256, 256, 32] layers-neurons, drop-out 0.55, learn rate 1e-3, patiance 10. Training with 5 epochs, batch size 32, scoring in 0.5 epochs, gama 0.92.
    2. Predicted the next hour direction accuracy using cutoff(clip) on probabilities of 0.85 and without cutoff. Found average accuracy of 75% on entire data-set. Clipped version had better percentiles with [0, 1, 10, 50, 90] for average of one week of [0.45, 0.6, 0.65, 0.8, 0.9]
    3. Evaluating also with local model cross-validation, training scores using 'model score = sqrt(acc train*acc valid)' had a pearson of 0.23 (weak positive correlation) on the unclipped version averaged by week accuracy. But yeat using a threadshould (based on the joint-plot kde) of 0.77 on the `model score` percentiles went from [0.4, 0.5, 0.6, 0.75, 0.85] to [0.5, 0.55, 0.65, 0.75, 0.85], again unclipped version. With a reduction to 43% of all predictions. So this `model score` has got potential!
    4. Made another experiment. Using 2 months for training and ~4 days for validation for local classifiers. I run CV on the entire data-set using local classifiers with same parameters as in 1 and 2. Found average accuracy of 75% on entire data-set only for the clipped version. Clipped version had better percentiles with [0, 1, 10, 50, 90] for average of one week of [0.4, 0.43, 0.65, 0.75, 0.85].
    5. Evaluating also with local model cross-validation, training scores using 'model score = sqrt(acc train*acc valid)' had a pearson of 0.46 (positive correlation) on the unclipped version averaged by week accuracy. Using a threadshould (based on the joint-plot kde) of 0.77 on the `model score` percentiles went from [0.15 0.3  0.4  0.55 0.75] to [0.3  0.4  0.5  0.65 0.8 ], again unclipped version. With a reduction to 20% of all predictions.
    6. Dicussion: comparing 4/5 with 2/3 results it seams that 2 months plus 4 days of validation is far inferior than the 5 years approach. One hypotheses is that 5 years contain more lessons from the past, on the same line the model may be too simple. 

#### The multi-time-scale Market (**personal learning**)

The market is also multi-scale that is multi-time-frame. It's fractal in time. This characteristic can be explored by using different moving averages. The market maybe going up in the yearly scale, e. g. like Brazil recovering from Dilma+Lula's bad administration, but one local event (monthly scale) may produce a big draw down but respecting the up-trend. Collection of events are normally responsible for defining the different trends in different time-scales.

1. 200-day : yearly trend (e.g. 2016-2017 up-trend Brazil economic recovery going to Previdence reform)
2. 50-day : two month trend  
3. 34-day : one and half month trend (e.g. down-trend Joesly day dossier and Micher Temer survival story)
4. 21-day : one month trend (e.g. Joesly day dossier and Micher Temer survival story)

### Hedge Trading

Strong tool is hedge trading! Rico has on Metatrader 5. Comment from USA trader: many positions open at the same  time buys and sells. He uses 72 hours limit expire time limit.  And only use take profit setup. According to him almost never loses money (special pre-crises december 2018 period of high-volatility). Tested in minicontratos (Bovespa) on the contract expire day, traded long on the latest and short the first with sucess profit on the difference.
