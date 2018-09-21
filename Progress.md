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

1.`Pytorch NN Global Model` - Wrote code to fit global NN on 5 years data using 1:30 hours shift. Removed samples overlapping days, 90 minutes in the morning and 90 minutes before session end - avoiding contamination between days assumption for day trade. Trained with 1 year and tested on the next 6 months. After training 66/33 with cross-validation 30 samples P50 accuracy  is 56%. 

- [x] Write class to train model `BinaryNN`
- [x] Decent Early Stopping (with patiance default 5 epochs ignoring variances in loss less than 0.05%)
- [x] Cross-validate model (K-fold). Test `sklearn` K-fold. Cannot use K-fold because cannot use future to train model. 
- [x] Also tested `train_test_split` from `sklearn` but it cannot be used for the same reason, mixing future with past when training the model. 
 > Quotting article *Random-testtrain-split-is-not-always-enough*
 > A trading strategy is always tested only on data that is entirely from the future of any data used in training. Nobody ever 
 > builds a trading strategy using a random subset of the days from 2014 and then claims it is a good strategy if it makes
 > money on a random set of test days from 2014. Finance would happily use random test-train split — it is much easier to 
 > implement and less sensitive to seasonal effects — if it worked for them. But it does not work, due to unignorable 
 > details of their application domain, so they have to use domain knowledge ***to build more representative splits***.
- [x] Wrote `indexSequentialFolds` to create folds for cross-validate the model, following the name no future-past mixing.
- [X] Wrote draft of cross-validation function using sequential-folds.
- [ ] Backtest 6 months predictions of global model.
- [ ] Try to tune backtesting parameters like stop time etc.
- [ ] Is P50:56% accuracy enough for profiting?
- [ ] What accuracy pdf should my model produce so I can profit?

2. Too many degrees of freedom, too many variables to explore. Random-search (gut instinct is random?) has proved to be a good tool for parameter optimization. Furthermore I have no other way to decide which path is better. Vai na raça!

- [ ] Sensibility Analyses. What direction take based on moving average trend. What performs best?
- [ ] Mix random noise with correct moving average trend direction to analyze what's the needed accuracy of the model.

3. `Pytorch NN Local Models`  Assuming P50:56% validation accuracy isn't enough for proffiting. Did some tests moving from Global to Local models fitting. Using months or weeks to predict days or minutes. Besides that made some cross-validations using the draft code above, but added additional samples with size of prediction vector (90 minutes) to check accuracy of model when on real-world use. Those samples were used to measure accuracy on data just after the validation data, simulating "future" data. Two models were trainned, the second using half the data for training/validation it gave a P50:72%  accuracy. That suggests that validation accuracy alone can be used to divide good from bad predictions (1000 simulations), used cut-off of 95% on the first validation score. Parameters : [ntrain= 4*60*5 week, ntest = 90 1:30 hours, nacc = 90 1:30 hours]. 

Thoughts:
Generalization of the general parameters for the **Model** can be done by cross-validation on sequential folds.
Efficience of Application/Use of the **Model** on local is another subject/matter/problem. 
Following that logic, it seems that, a better way to evalute (cross-validate) the effective *local* generalization of the model is to use this 3 split on each fold (train, test, accuracy). But the 3rd piece of the fold cannot (accuracy) be used to control/early-stop the training. 
Training accuracy can also be used to divide which predictions are best. Although you can have a high accuracy in evaluation it is possible to have low accuracy on training due early stopping. 

- [ ] Hyperperameter `GridSearchCV` for `number layers, train-score ratio, train+score size` for start.
- [x] Create method `fineTune` to train the model without validation samples. 


 
