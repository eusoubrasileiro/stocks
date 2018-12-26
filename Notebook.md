
#### Support and resistence levels

##### Theoretical bases for support and resistence levels:

- Discrete economic hypotheses or scenarios produces deterministic finite price levels.
- Human emotional behavior, psychology of fear and greed, produces price levels according to Fibonacci.

Future behavior are easier to predict on resistance levels, according to `Wikipedia stock prediction`. Once you label and training using past data? Use Fibonacci retracement on volume. Most day trade or strategies used by traders are based on resistance levels on different time frames (my experience trading/reading thousand blogs)!

#### The multi-time-scale Market (**personal learning**)

The market is also multi-scale that is multi-time-frame. It's fractal in time. This characteristic can be explored by using different moving averages. The market maybe going up in the yearly scale, e. g. like Brazil recovering from Dilma+Lula's bad administration, but one local event (monthly scale) may produce a big draw down but respecting the up-trend. Collection of events are normally responsible for defining the different trends in different time-scales.

1. 200-day : yearly trend (e.g. 2016-2017 up-trend Brazil economic recovery going to Previdence reform)
2. 50-day : two month trend  
3. 34-day : one and half month trend (e.g. down-trend Joesly day dossier and Micher Temer survival story)
4. 21-day : one month trend (e.g. Joesly day dossier and Micher Temer survival story)

#### Hedge Trading

Strong tool is hedge trading! Rico has on Metatrader 5. Comment from USA trader: many positions open at the same  time buys and sells. He uses 72 hours limit expire time limit.  And only use take profit setup. According to him almost never loses money (special pre-crises december 2018 period of high-volatility). Tested in minicontratos (Bovespa) on the contract expire day, traded long on the latest and short the first with sucess profit on the difference.


#### Risk, leverage and number of stocks to buy/sell.

Quoting Quora answer about How-do-I-reduce-losses-in-day-trading-of-stocks.  

Avoid excessive leverage. Leverage may be important when day trading to maximize gains, but not in excess, you must learn about position sizing. Don’t risk more than 2% (some say 1%, up to you) of your equity on a single trade. For example, if you have an equity of 100k, then 2% risk would be 2,000. Now if you're buying a stock worth Rs. 100 and your stop loss is Rs.99, then your risk per share is Rs. 1, divide 2000 by 1, which is 2000, the number of shares you can buy at most.

#### More representative splits

Train-test-split : K-fold vs Sequential-Fold

Quotting article Random-testtrain-split-is-not-always-enough A trading strategy is always tested only on data that is entirely from the future of any data used in training. Nobody ever builds a trading strategy using a random subset of the days from 2014 and then claims it is a good strategy if it makes money on a random set of test days from 2014. Finance would happily use random test-train split — it is much easier to implement and less sensitive to seasonal effects — if it worked for them. But it does not work, due to unignorable details of their application domain, so they have to use domain knowledge to build more representative splits.

That was the base to write the `SequentialFolds` code.

#### About metrics and model evaluation

I would implement  ´Balanced accuracy score´ to correct for the effect
of  of inbalanced datasets from https://scikit-learn.org/stable/modules/model_evaluation.html#accuracy-score
But since my data-sets are huge and there almost no class-inbalance in Validation I don't see a reason to do so. All the rest is accounted in the cross-entropy loss or log-loss.

To take a look at Receiver operating characteristic (ROC)¶
To take a look at TimeSeriesSplit
To take a lookt at Rolling Window Analysis https://www.mathworks.com/help/econ/rolling-window-estimation-of-state-space-models.html

https://machinelearningmastery.com/backtest-machine-learning-models-time-series-forecasting/
Walk-forward validation is the gold standard of model evaluation. It is the k-fold cross validation of the time series world and is recommended for your own projects.
https://en.wikipedia.org/wiki/Walk_forward_optimization
