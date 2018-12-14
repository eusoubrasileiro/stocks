
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
  9. Results averaged from 58% to 75% on validation set. Very Encouraging! Certainty much better than candle pattern.
  10. Tried to use a cut-off  to overcome the problem of having an unprobable prediction, too close to 0.5 for class 0 or 1. Results were interesting but need to test more. Another possible reason for this working is the 3rd class not presend that is when did not go Up neither Down.
  11. Need to properly cross-validade it and maybe do a grid-search for parameters, network topology, number of previous days to use etc. Need also focus on the recent years data.
  12. Need to test if this works for Petrobras 2000 to 2018 data I have. Or Bovespa from 1993 to today from Metatrader 5.


#### The multi-time-scale Market (**personal learning**)

The market is also multi-scale that is multi-time-frame. It's fractal in time. This characteristic can be explored by using different moving averages. The market maybe going up in the yearly scale, e. g. like Brazil recovering from Dilma+Lula's bad administration, but one local event (monthly scale) may produce a big draw down but respecting the up-trend. Collection of events are normally responsible for defining the different trends in different time-scales.

1. 200-day : yearly trend (e.g. 2016-2017 up-trend Brazil economic recovery going to Previdence reform)
2. 50-day : two month trend  
3. 34-day : one and half month trend (e.g. down-trend Joesly day dossier and Micher Temer survival story)
4. 21-day : one month trend (e.g. Joesly day dossier and Micher Temer survival story)

### Hedge Trading

Strong tool is hedge trading! Rico has on Metatrader 5. Comment from USA trader: many positions open at the same  time buys and sells. He uses 72 hours limit expire time limit.  And only use take profit setup. According to him almost never loses money (special pre-crises december 2018 period of high-volatility). Tested in minicontratos (Bovespa) on the contract expire day, traded long on the latest and short the first with sucess profit on the difference.
