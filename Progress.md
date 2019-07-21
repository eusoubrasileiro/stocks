
Progress Report

May 2018. Started using code from Forex experiments focusing on Ibovespa stocks specially PETR4 for day-trade robot.
1. Learned how to download stock using Metatrader 5 in .csv format.
2. Using 60 minutes shift in time and ideas from Russian post on Metatrader started testing with Petrobras. Used 8 symbols code is prepareData.py
3. Wrote misleading code to make backtesting using ExtraTreeClassifier from sktlearn
4. Time for backtesting was too slow (altough I was using multithreading) so I decided to test Tensorflow aiming GPU boost.
5. Wrote script ( DownloadRates.mq5 ) for downloading ibovespa stocks data using Metatrader 5 (1 minute time frame) and converting it to pandas dataframe ( meta5Ibov.py ).
6. Tensorflow was fast but Pytorch was easier and cleaner so I moved ( torchNN.py ).
7. Created a efficient fast backtesting engine backtesting.pyx (1 minute time-frame) in Cython - checked against Metatrader 5 backtesting tool.
8. Results were promissing with 120 minutes time shift and local models using past 1 week data.
9. Wrote ExpertAdvisor.mq5 . Backtesting inside Metatrader 5 results were increadible using predictions file. Also wrote daimonPredictor.py to create predictions to advisor in real time.
10. When backtesting on Rico Hedge Demo Account found the biggest errors of all, predictions created by pytorch code were shift in time to the past 120 minutes. So results were wrong! Need to study a mathematical model that works (P1, P10, P50, acceptable) for prediction.

September 2018. Starting again.

Lessons learned:
• Progress Report: Data science somewhere stated that 20% time should be used documenting.
• Write good unit tests, specially testing time of prediction. Use Python 3 API.
• Don't spend much time with prototype notebooks. That means you are losing focus and objective. Instead write python modules from notebooks using the knowledge learned.

Table of Definitions
￼￼￼￼
Algorithm trading pillars are:
1. Mathematical model (if used)  
2. Trading strategy (stop-loss-gain size, time, trailing stop etc.)  

Progress
1. Pytorch NN Global Model - Wrote code to fit global NN on 5 years data using 1:30 hours shift. Removed samples overlapping days, 90 minutes in the morning and 90 minutes before session end - avoiding contamination between days assumption for day trade. Trained with 1 year and tested on the next 6 months. After training 66/33 with cross-validation 30 samples P50 accuracy is 56%.  

• [x] Write class to train model BinaryNN  
• [x] Decent Early Stopping (with patiance default 5 epochs ignoring variances in loss less than 0.05%)  
• [x] Cross-validate model (K-fold). Test sklearn K-fold. Cannot use K-fold because cannot use future to train model.  
• [x] Also tested train_test_split from sklearn but it cannot be used for the same reason, mixing future with past when training the model.  

#### Build more representative splits for CV
Quotting article Random-testtrain-split-is-not-always-enough.

• [x] Wrote indexSequentialFolds to create folds for cross-validate the model, following the name no future-past mixing.  
• [x] Wrote draft of cross-validation function using sequential-folds.  
• [ ] Backtest 6 months predictions of global model.  
• [ ] Try to tune backtesting parameters like stop time etc.  
• [x] Is P50:56% accuracy enough for profiting? Not with the actual set-up of trade?  
• [ ] What accuracy pdf should my model produce so I can profit?

1. Too many degrees of freedom, too many variables to explore. Random-search (gut instinct is random?) has proved to be a good tool for parameter optimization. Furthermore I have no other way to decide which path is better. Vai na raça!

• [ ] Sensibility Analyses and Optimal Parameters for lagged EMA Binary Strategy.  
• Direction decision is based on the majority (sum) of the future predicted minutes.  
• What is the sensibility to wrong predictions for this EMA trend strategy?  
• What accuracy should a forecast model have to use this strategy?  
• What are the optimal parameters for using this strategy?  
• Prototype: What direction - Ema trend up-down.ipynb .  
• [x] Ported backtestEngine to numba. Easier and cleaner.  
• [x] Created EstrategyTester class.  
• [x] Random Grid Search for optimal parameters lag time , clip percentage (decision) , min. profit , expected. variation  
• [x] Run ~2000 simulations with scenarios of 1 month, capital of 50k, 10 orders per day max., 2 orders per hours max. Assuming 3% frequency appearance of entry points by the forecast model.  
• [x] Varying variables have uniform distributions: lag=[60, 180], clip=[0.66, 0.96], var=[0.005, 0.025], minprofit=[70, 600]   
• [x] Saved metric variables: money[p0, p1, p50], accuracy,  
• [x] First evaluation variable to sort results: eval=3*acc+2*p0+p10+p50+(1.-minprofit/max(minprofit))  
• [x] Mixed random noise with correct binary direction to analyze what's the needed accuracy for a NN Model.  
• Assuming adding 30% random (wrong) directions, for example, corresponds to a model with a p50 of 70% of accuracy. Don't know how to approach it better.  
• [x] Discussion. I missed to save profit and average number of orders per day. Chosen parameters were producing too few predictions per day and low profit. Evaluation variable used doesn't account for profit. Not considering effect of wrong predictions during scenarios.  
• [x] Implement better reward-to-variability ratio than sharp and suitable for algorithm evaluation in month, week etc scenario. To compare to risk-free, like CDI/SELIC, investment. Sortino adapted to linear interest rate growth sortina with numba.  
• [x] Changed way expected variation (average volatility expected) is used to make stop/loss and stop/gain. Included reward-to-risk as a parameter in the Simulator default rwr=3 , stop=exvar/rwr , gain=exvar .  
• [x] Random Grid Search for optimal parameters lag time , clip percentage (decision) , min. profit , expected. variation , reward-to-risk-ratio   
• [x] Run ~6000 simulations same as above.  
• [x] Additional variables varying: rwr=[2, 7], rand=[0.05, 0.40] (wrong predictions). Changed exvar=[0.01, 0.07], minprofit=[300, 4300]  
• [x] Additional metric variables saved: final profit, average orders per day, sortino-adapted sortina   
• [x] Evaluation based only on bucketized sortina variable. Using seaborn factorplot due very spreaded variable space.  
• [x] Tunned parameters. clip < 0.8; mprof >> (as high as possible); exvar > 0.05; lag < 90; rand < 20% or NN acc > 80%. Choosen: clip 0.73, lag 71, exvar 0.05, mprof 1800. Ignored reward-risk-ratio not possible to find a best.  
• Too high lag values gave bad profit. Too high: mprof made a lot of nans, not enough money to make an order; clip, not sufficient entry points; lag, not sufficient entry points;  

• [x] Discussion. If you observe that in all cases simulated 60%++ of data is correct or more, it is almost abvious that the more money ( mprof ) or greater tolerance you have on stop loss ( exvar ) the more money you will make. Reward-risk-ratio can be ignored since the backtest engine probably closed the vast majority of orders by time. Interesting that 1800 gives an ~ 1.6% risk appetite for 50k capital.  
• [x] Screening better over these parameters including even 40% of wrong directions (90k scenarios) proved very profitable. Percentiles of profit: p0: 0.03, p1: 0.04, p10: 0.067, p50: 0.134118. Always profitable exactly what we want!  

1. Pytorch NN Local Models Assuming P50 56% validation accuracy isn't enough for proffiting. Did some tests moving from Global to Local models. Using a week to predict 90 minutes. Made some cross-validations using the draft code above, but added additional samples with size of prediction vector (90 minutes) to check accuracy of model just after the validation-set, simulating "future" data when on real-world use. Models were trainned, and a fine-tune on the model was done. Models were trained using all the samples available without a validation-set to supervise but for very few epochs. Idea is to displace the weights just a little bit using the most recent data available. Preliminar results suggests that's a promising methodology. To divide good from bad models (entry points) the best result found was using an 'average' of trainging and validation accuracies avg = np.sqrt(score0*score1) Parameters : [ntrain= 4 60 5 week, ntest = 90 1 30 hours, nacc = 90 1 30 hours, finetunning 3 epochs]. Final results were ~ : p0 0.70 p1 0.70 p10 0.76 p50 0.90 p90 0.95. for avg > 0.93/94 with 1.5% showing frequency on ~1600 simulations ~3.5 per day.  

1. Risk, leverage and number of stocks to buy/sell. Quoting Quora answer about How-do-I-reduce-losses-in-day-trading-of-stocks.  

• [x] The formula for number of stocks did not used risk-appetite but minimal profit. That do not account for capital and the default settings for a 50k capital was a risk-appetite really low of 0.2%. Probably that is the reason for having so low profit even when accuracy was above 50% on many past experiments. I've seen algorithms with accuracy reported on Infomoney of 48% or less being profitable by having a 3:1 or more risk-to-reward rate. Also if could not achieve the defined minprofit order wasn't made. Need to change code to use a risk-appetite riskap in decimal and capital to define number of stocks. Low volatility causing low profit should be dealt separately.

1. Understand better the simulator. Don't know how it performs with 20% or 70% correct entries.  
• [ ] Write test case for Simulator using some of the EMA trend on real data.  
• [ ] Analyze Simulator sensibility to risk-appetite, reward-risk-ratio and expected-variation.  

#### Milestone: moved from Minutes to Day/Hour time-frame (November/2018)
##### Reason: it's simpler to predict one sample in the future than many (several minutes)

- [x] Todo: Implement candle pattern next-day predictor. Implement next day prediction up or down based on 1 century historical data. Remember to set to unknown class classified with less than 0.6 certainty. Probably not an up or an down.
  - Implemented and got preliminary results using dow-jones one century of historical daily Open, High, Low, Close and Volume. The data preparation I describe bellow:
  1. Removed nan values or all equal O, H, L, C values.
  2. 'Corrected' inflation by applying log function on all.
  3. 'Corrected' the noise effects, 'long wave-lengths'. What I don't care are monthly, yearly time-scale variations of price by removing a EMA of 2 days. I am only interested on daily to 1 days price variations.
  4. Calculated Up or Down binary class based on the Close price.
    4.1 Also use another class that was the up/down of the difference to the SMA. I calculated the SMA for 2 days over Close Price. Calculated the residues by removing the Close prices from it. If the residues[i+1] > residues[i] -> class 1. If the residues[i+1] <= residues[i] -> class 0
  5. Scaled that with `RobustScaler` clipping (10%, 90%) already creates mean 0. and stdev 1.
  6. Made feature vector using 21 previous days and concatenating it with a total size of 21*5.
  7. Create target vector/class with the following day residues direction 1 or 0.
  8. Trainned Torch binary classifier with different scenarios:
    1. 3 to 6 layers
    2. 1024 to 200 neurons in each layer
    3. 10, 12, 5 years for training
    4. 1 year, 6 months for validation
    5. different start year/month/day for each training/validation window
    5. Many tests with different parameters: patience, learning rate, optimizer, gamma, number of epochs.
  9. Results averaged from 48% to 56% on training set. As expected. There is no such thing as a candle pattern.
  10. Tried to use a cut-off  to overcome the problem of having an unprobable prediction, too close to 0.5 for class 0 or 1. Results are not clear need to test more.
  11. Need to properly cross-validade it and maybe do a grid-search for parameters, network topology, number of previous days to use etc. Need also focus on the recent years data.
  12. Need to test if this works for Petrobras 2000 to 2018 data I have. Or Bovespa from 1993 to today from Metatrader 5.
  13. New tests using EMA of 2/3 days and 21 previous has given an average from 48 to 56% of accuracy on the validation set. Now using `QuantileTransform` since it already clips outliers.
  14. Conclusion again: There is no such thing as a candle pattern!

- [x] After some tentatives found better use WIN future contracts with 1-2 hours time-frame, corrected by discount downloaded with Metatrader 5. Altough data just from 2013, very realiable and exactly what I want to operate with real money. `Pytorch NN Global - Candle Pattern - WIN.ipynb`
  - Found similar results (48-56% accuracy) as above, best using 2 hours SMA.
  - Note: depending on the window/fold size, training and validation samples different results can be achieved in the cross-validation. Also when prediction further ahead of the window/fold the accuracy also varies a lot. It seams that farther from the fold window worse is the prediction accuracy. That need to be better understood.  
    1. Using 5 years for training set and 0.9 ratio for validation, ~ 132 days. I run CV on entire data-set using local classifiers with [256, 256, 256, 32] layers-neurons, drop-out 0.55, learn rate 1e-3, patiance 10. Training with 5 epochs, batch size 32, scoring in 0.5 epochs, gama 0.92.
    2. Predicted the next hour direction accuracy using cutoff(clip) on probabilities of 0.85 and without cutoff.
    3. Evaluating also with local model cross-validation, training scores using 'model score = sqrt(acc train*acc valid)' had a pearson of 0.23 (weak positive correlation) on the unclipped version averaged by week accuracy. But yeat using a threadshould (based on the joint-plot kde).
    4. Made another experiment. Using 2 months for training and ~4 days for validation for local classifiers. I run CV on the entire data-set using local classifiers with same parameters as in 1 and 2.
    6. Dicussion: That must be studied better. Separate good from bad predictions for define entry points are not that easy. I believe there  must be used more inputs! Besides that param GridSearchCV would be helpfull.

- [x] Open Question - For the above two points cannot understand why random data gives better results 60% or 85% of training and validation of simple NN than correct data set. Something is wrong with data preparation I believe but could not close this question for certain. The answer maybe should deserve a topic.
   - The explanation is over-fitting. The number of parameters on the model is much greater than the number of training samples. Neural Networks are yet very powerful with the use of regularization such as drop-out, early-stop etc but in this specific case the regularization were not being effective that's why accuracies so high on training and validation due over fitting. Increasing drop-out probability in between layers or decreasing the number of neurons by lays both make accuracy decrease. Awesome post about the topic on stats stackexchange `relationship-between-model-over-fitting-and-number-of-parameters/320387#320387`.

**Highlighted Thoughts for Binary NN forecast of day, minutes or hours**  
• Generalization of the general parameters for the Model can be done by cross-validation on sequential folds.  
• Training accuracy can also be used to divide which predictions are best. Although you can have a high accuracy on validation-set it is possible to have low accuracy on training due early stopping or randomness.  
• [ ] Calculate entropy of data to assist division of bad and good predictions. Low entropy is bad?  
• [ ] Hyperperameter random? GridSearchCV for number layers, train size, train-score ratio, fine-tune nepochs, classifier nepochs for start fix 90 minutes for accuracy. That will guide less overfiting and too many parameters on model and others. Fundamental!  
• [x] Create method fineTune to train the model without validation-set and early-stop control.  
• [ ] Maybe changing how orders are placed, stop/loss, reduction, move stop/loss up, could improve accuracy.  

- [ ] Need to backtest on Metatrader 5 the best results found above. Altough I could try to backtest on my engine that would not work well for the 2H time-frame, stop-loss would be hitted too often and unrealistically.

- [ ] Rewrite backtest engine to support callback function on event `OnChange` making it support whatever time-frame of historical data. Also need to uncouple the predictions vector leting it be in whatever time-frame.

#### Milestone: moved from everything above to minute time-frame Saulo's algorithm (November/2018)
##### Reason: it was giving fake impressive results

- The algorithm is based on classification of signals on bollinger bands. Using historical data first we analyze if there is a buy or sell signal from the bollinger band, a hold signal (no-buy nor-sell) we classify as class-0. Then we analyze if that buy signal and subsequent sell signal really made profit. If they did they are really buy and sell signals and are classified accordingly with 1, 2  classes, otherwise they are classified as hold 0. The set-up of the algorithm is only for long positions but I made tests changing to short positions.
- Translate code for Saulo's free course is given in `Saulo_ml4t_traininig_clean.py`
- Prototypes are `Sklearn Saulo Daimon WIN` and `Sklearn Saulo WIN`.
- Code was entirely reorganized and `buybandsd.py` daemon was created.

#### Milestone : predictions were contaminated by future, had to start again. Precision for class 1 of 90% were far too much.

- Restarted testing real precision of classifier without future contamination. Bellow a summary of parameters test with a data window of 300 for training and making 30k random predictions on the entire data available for different symbols. 30k upon analysis seams to be the required amount of samples to not have a biased estimate of the precision probability distribution, even with a 2% percent `stdev`.

- [x] Random Forest don't need standardization. Decisions trees are like that. So removed that. Side effect `joblib` started to work o jupyter notebook cell on Windows. Another side effect it's running much faster and allows now a very small window of data.
- [ ] need to calculate number of possible scenarios based on data window.

- [x] **Random Search of params**
    What best gives precision on class 1  
     - `tsignals` are crossing signals **ajdusted** for correct class (hold or buy or sell)
     - `bbwindow` size of second bb window. Others are multiples e. g.
         [ 21, 21x0.5=10, 21x1.5=31, 21x2=42 etc ]
     - `batchn` number of previous samples of a signal that are used to assembly the feature X vector for training.
     - `precision` is the multiclass precision of class 1

- need to update this table here precision is not right due low number of samples

 | batchn | nbands | bbwindow | tsignals |  precision | clip-pb.  |
 | ------ | ------ | -------- | -------  | ---------- | --------- |
 |   21   |    6   |    21    |    90    |     29%    |           |
 |   21   |    6   |    21    |    80    |     41%    |           |
 |   21   |    6   |    21    |    84    |     45%    |           |
 |   21   |    3   |    21    |    36    |     48%    |           |
 |   32   |    6   |    16    |    84    |     42%    |    50%    |

- Unfortunantly nothing above 50% has more than 1 entry per day. `clip-pb` used a threadshould of 0.56 otherwise there would not be predictions.

- [x] Made a little effort for cross-validate the model before predicting. Not very encouraging.

- [x] Implement bollinger bands class with only 2 classes

**Things to do**

- [ ] create a adequate metric to evaluate predictions quality specially focusing on class 1. Maybe use precision of class 1 is easier.
- [ ] classify somehow with cross-validation the quality of the model
- [ ] make a adequate grid-search of params using a suitable metric
- [ ] verify if that metric for model-quality can be used to classify good from bad predictions.
- [ ] certify the implementation of close by time in metatrader 5. Its not a big problem due stop-loss based on stdev working.
- [x] fix mql5 code not executing predictions. Be carefull in what symbol you use for testing, that was the problem.
- [ ] make metatrader backtest on data from real daemon jupyter notebook predictions.
- [x] Set stop and gain in WIN points. Using Standard Deviation (sigma) of last 60 minutes to set stop as 3*sigma (percentil 89%) and stop gain as 3*sigma*1.25.
- [x] Implemented trailing stop based on EMA variation on last 5 minutes. If ema5 now is bigger than in the last minute. Use that difference to increment the stop-loss and stop-profit.   

Learning to remember: Lost 20k due no stop loss and not sure that prediction was good.

#### Milestone : try Simple or Naive Strategies since many people with non-mathematical background reported (on-media) profit with robots

- [x] Implemented `NaiveExpert` based on `prototypes\Numpy Naive Buy - WIN` using pattern of 5 minutes up trend before to buy and surf on the uptrend.  Made backtest on metatrader 5 and it's clear that you cannot garantee that will buy on the `Open` and sell on the `Open` using the close by time is clearly otherwise. The metatrader 5 backtesting engine is really robust using 1 minute data OHLC to create aproximated Tick's using bar patterns. The accuracy went from 90% to 45% on real backtesting on metatrader 5. So better really use worst case buy scenario on a 1 minute backtesting engine `buy on high and sell on low`. Another conclusion a new backtest engine based on ticks would help not waste time tasting naive strategies.

-[ ] Implement `NaiveSupports` based on volume at price using Gaussian Mixture Models. Use maximum data of 21-days for support and resistences calculation. Calculate supports and resistences at every 5 minutes? and place stop or limit orders accordingly. Should use  Additionally maybe should consider using bigger positions where the trajectory is clear and small positions were there are many support and resistences close-by.

#### Milestone : based on algorithm trading pillars totally refactoring of backtesting Engine

One pillar of algorithm trading is the *Trading strategy* and without a realiable backtest engine nothing can be done about that.
Many results above point to the need of a more realistic backtest engine supported by Python. The reason is because writting code im mql5 every-time I think in a new strategy is just impossible.

- [x] New engine works in Hedge Mode and supports `Buy Stop` and `Sell Stop` orders
- [x] Implement `Buy Limit` and `Sell Limit` and fixed wrong entries of `Buy Stop` and `Sell Stop` orders
- [ ] Restric wrong params for pending orders `Buy Stop`, `Sell Stop`, `Buy Limit` and `Sell Limit` ?
- [x] code in C library `metaEngice.c` compiled as shared library imported by `metaEngine.py`
- [x] `metaEngine.c` code supporting build on Windows (`*.dll`) or Linux (`*.so`)
- [x] `metaEngine.py` access directly the global variables (in memory) exported by the shared library C code.
That is done using `ctypes`
- [x] old back-test engine were renamed as `_old` since their reliability was questionable
- [ ] refactor strategyTester old `stester`
- [ ] console program to make some unit tests `metaEngine.c` ?\

#### Milestone : first real profitable EA on back-test on Metatrader 5

- [x] Implement `NaiveGapExpert` based on analysis that ~93% of gaps smaller than 230 points close for `WIN@` mini-bovespa (prototype `Gaps Closing Patterns - WIN`) Used 4 pivot points (R1, R2, S1 and S2) formula on each of the last 5 days and placed limit orders on support and resistances closer to the open-price. Limit orders where limited to 2 per gap. Metatrader 5 backtest gave first EA profitable with 8 expected pay-off and sharp of 0.08. Back-tested also with `WIN@D` and `WIN@N` with similar results.     
    - [x] Need to remove limit orders if gap is reached before they are triggered
    - [x] Built C++ dll from Armadillo library compiling with mingw targeting x86-64
    - [x] Wrote simple mql5 unit test script for Armadillo C++ library `dll` wrapping
    - [x] Remove support-resistances equal. Used unique function from Armadillo library "cpparm.dll"
    - [x] Fixed wrong usage of `CopyRates` and `CopyOpen` including for previous day pivots calculation
    1. When using `CopyRates(sname, PERIOD_D1, 0, 5, rates)` for calculation of pivots the actual day index 0 gets included (with ohlc all equal to open-price). Somehow that makes the pivots more closer and make sharp go to 0.10 and the expected pay-off to 9.3 . We can think as we modify the formula of pivots as consequence the stops and limits are placed closer to our entries. We can see on graph better results also after 2017 (where we have a big up trend?). Long trades won goes to 100% 269. Loss trades were 15%.  436 Trades 1245 Deals
    2. When removing the repeated support-resistances (w. unique) expected pay-off went to 4.5. Loss trades went to 34%. 443 Trades 1122 Deals
    3. When using `CopyRates` just with the last 5 days and unique support-resistances the expected pay-off goes to 7.88 sharp goes to 0.06 and Loss Trades to 43%. Total Trades 335 and 867 Deals. Seams to be accordingly explained above much less entries probably due support-resistances too far-apart.
    - It's expected that once histogram of volume at price are used for defining support-resistance zones better results than those points 1, 2 and 3 above will be reached.
    - [x] Use trailing stops, certainly will improve something on profit.
        - [x] Found trailing stop classes like `CTrailingPSAR` but found it difficult to use without the expert class. Many nice ready-made stuff done there (Experts, Signals etc.) that I need to explore.
        - [x] Used previously created code for bbands based on EMA. Sharp went to 0.16 and pay-off to 8.42. Loss trades 22%. 429 Total Trades 1440 Deals. Using also camarilla and 0 to 6 last days without using `Unique`. Long trades won also 100%. Maximum draw-down relative of 4.5%
    - [x] Use support and resistances based histograms of price. Did not improve results. Sharp, pay-off all went worse. It seams that you get too dependent on the previous days. Levels never reached will not be taking in to account. Might be good when market is trending up and down but not for all history.
    - [x] Included camarilla support and resistances using 6 previous days without removing repeated points with unique. Sharp went to 0.11 and pay-off to 5.6. 328 trades and 1108 deals. Loss trades 23%. Draw-down maximum relative 6.7%
    - [ ] Include size of order based on strength of resistance-support.
    - [ ] Remove support-resistances too closer apart from array of pivots
    - [x] Make it generic for stocks or other instruments
        - Tested on PETR4 entire-history sharp of 0.1
    - [ ] Use different stop-loss for each limit orders smaller
    - [ ] Just found trading today that resistances older than 5 days was reached. Maybe test using last 1 year of support and resistances and clip histogram by last 5 day min-max?
    - [ ] Use different size of positions for different support/resistences?
