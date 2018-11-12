
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

Algorithm trading can be divided:
1. Mathematical model (if used)
2. Trading strategy (stop-loss-gain size, time, trailing stop etc.)

Progress
1. Pytorch NN Global Model - Wrote code to fit global NN on 5 years data using 1:30 hours shift. Removed samples overlapping days, 90 minutes in the morning and 90 minutes before session end - avoiding contamination between days assumption for day trade. Trained with 1 year and tested on the next 6 months. After training 66/33 with cross-validation 30 samples P50 accuracy is 56%.

• [x] Write class to train model BinaryNN
• [x] Decent Early Stopping (with patiance default 5 epochs ignoring variances in loss less than 0.05%)
• [x] Cross-validate model (K-fold). Test sklearn K-fold. Cannot use K-fold because cannot use future to train model.
• [x] Also tested train_test_split from sklearn but it cannot be used for the same reason, mixing future with past when training the model.

Quotting article Random-testtrain-split-is-not-always-enough A trading strategy is always tested only on data that is entirely from the future of any data used in training. Nobody ever builds a trading strategy using a random subset of the days from 2014 and then claims it is a good strategy if it makes money on a random set of test days from 2014. Finance would happily use random test-train split — it is much easier to implement and less sensitive to seasonal effects — if it worked for them. But it does not work, due to unignorable details of their application domain, so they have to use domain knowledge to build more representative splits.


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
• [x] Random Grid Search for optimal parameters lag time , clip percentage (decision) , min. profit , expected. variation , reward-to-risk-ratio ,
• [x] Run ~6000 simulations same as above.
• [x] Additional variables varying: rwr=[2, 7], rand=[0.05, 0.40] (wrong predictions). Changed exvar=[0.01, 0.07], minprofit=[300, 4300]
• [x] Additional metric variables saved: final profit, average orders per day, sortino-adapted sortina 
• [x] Evaluation based only on bucketized sortina variable. Using seaborn factorplot due very spreaded variable space.
• [x] Tunned parameters. clip < 0.8; mprof >> (as high as possible); exvar > 0.05; lag < 90; rand < 20% or NN acc > 80%. Choosen: clip 0.73, lag 71, exvar 0.05, mprof 1800. Ignored reward-risk-ratio not possible to find a best.
• Too high lag values gave bad profit. Too high: mprof made a lot of nans, not enough money to make an order; clip, not sufficient entry points; lag, not sufficient entry points;

• [x] Discussion. If you observe that in all cases simulated 60%++ of data is correct or more, it is almost abvious that the more money ( mprof ) or greater tolerance you have on stop loss ( exvar ) the more money you will make. Reward-risk-ratio can be ignored since the backtest engine probably closed the vast majority of orders by time. Interesting that 1800 gives an ~ 1.6% risk appetite for 50k capital.
• [x] Screening better over these parameters including even 40% of wrong directions (90k scenarios) proved very profitable. Percentiles of profit: p0: 0.03, p1: 0.04, p10: 0.067, p50: 0.134118. Always profitable exactly what we want!

1. Pytorch NN Local Models Assuming P50 56% validation accuracy isn't enough for proffiting. Did some tests moving from Global to Local models. Using a week to predict 90 minutes. Made some cross-validations using the draft code above, but added additional samples with size of prediction vector (90 minutes) to check accuracy of model just after the validation-set, simulating "future" data when on real-world use. Models were trainned, and a fine-tune on the model was done. Models were trained using all the samples available without a validation-set to supervise but for very few epochs. Idea is to displace the weights just a little bit using the most recent data available. Preliminar results suggests that's a promising methodology. To divide good from bad models (entry points) the best result found was using an 'average' of trainging and validation accuracies avg = np.sqrt(score0*score1) Parameters : [ntrain= 4 60 5 week, ntest = 90 1 30 hours, nacc = 90 1 30 hours, finetunning 3 epochs]. Final results were ~ : p0 0.70 p1 0.70 p10 0.76 p50 0.90 p90 0.95. for avg > 0.93/94 with 1.5% showing frequency on ~1600 simulations ~3.5 per day.

More Thoughts
• Generalization of the general parameters for the Model can be done by cross-validation on sequential folds.
• Training accuracy can also be used to divide which predictions are best. Although you can have a high accuracy on validation-set it is possible to have low accuracy on training due early stopping or randomness.
• [ ] Calculate entropy of data to assist division of bad and good predictions. Low entropy is bad?
• [ ] Hyperperameter random? GridSearchCV for number layers, train size, train-score ratio, fine-tune nepochs, classifier nepochs for start fix 90 minutes for accuracy. That will guide less overfiting and too many parameters on model and others. Fundamental!
• [x] Create method fineTune to train the model without validation-set and early-stop control.
• [ ] Maybe changing how orders are placed, stop/loss, reduction, move stop/loss up, could improve accuracy.

1. Risk, leverage and number of stocks to buy/sell. Quoting Quora answer about How-do-I-reduce-losses-in-day-trading-of-stocks .

Avoid excessive leverage. Leverage may be important when day trading to maximize gains, but not in excess, you must learn about position sizing. Don’t risk more than 2% (some say 1%, up to you) of your equity on a single trade. For example, if you have an equity of 100k, then 2% risk would be 2,000. Now if you're buying a stock worth Rs. 100 and your stop loss is Rs.99, then your risk per share is Rs. 1, divide 2000 by 1, which is 2000, the number of shares you can buy at most.


• [x] The formula for number of stocks did not used risk-appetite but minimal profit. That do not account for capital and the default settings for a 50k capital was a risk-appetite really low of 0.2%. Probably that is the reason for having so low profit even when accuracy was above 50% on many past experiments. I've seen algorithms with accuracy reported on Infomoney of 48% or less being profitable by having a 3:1 or more risk-to-reward rate. Also if could not achieve the defined minprofit order wasn't made. NEed to change code to use a risk-appetite riskap in decimal and capital to define number of stocks. Low volatility causing low profit should be dealt separately.

1. Understand better the simulator. Don't know how it performs with 20% or 70% correct entries.
• [ ] Write test case for Simulator using some of the EMA trend on real data.
• [ ] Analyze Simulator sensibility to risk-appetite, reward-risk-ratio and expected-variation.

More random thoughts / Ideas

Future behavior are easier to predict on resistance levels? Once you label and training using past data? Use Fibonacci retracement on volume. Most day trade or strategies used by traders are based on resistance levels on different time frames (my experience reading thousand blogs)!