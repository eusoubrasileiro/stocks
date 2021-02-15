## Stocks Prediction
System for Stock Inefficiency Identification and Movement Prediction

Several tentatives by my head, lots of learning and a little experience on financial market.
Have used from sklearn, tensorflow to pytorch. Binary options, future contracts and stocks.
Pandas and written a bunch of code starting in April 2018.
Have used Metatrader 5 since I figure out my own backtest engine was too much risk and after too many failures.
An then failures was all I had so far.

Much and much more than I have learned by myself is written on the book:

[`Advances in Financial Machine Learning`](https://www.amazon.com.br/dp/B079KLDW21/ref=cm_sw_r_tw_dp_x_X.J.EbX413CTZ)

I would have save thousands of hours if I had started from it.
But here we are and now that is my reference and somewhat goal.

a bit more of self learning wisdom:  
1. Never play too long on a prototype otherwise you are lost!  
  Prototype notebooks are ideas that should soon be transformed in an independent python module for backtesting.
2. Write unit tests! (lessons learned months wasted)
  Simple and nailing the failure.

### Summary of book (`Advances in Financial Machine Learning`) guided implementation:

Book snippet examples are in Python (Pandas mostly).
The idea of working with Dollar Bars (2.3.1.4) made it mandatory to use C++ language due performance reasons. 
Also C++ using [`pybind11`](https://github.com/pybind/pybind11) makes Python integration awesome. Why Python integration?
Because while you develop you can research and explore ideas using jupyter notebooks (following the book). 

So evertyhing 

- Chapter 2 - Financial Data Structures
  - 2.3.1.4 Dollar Bars
    - C++: `MoneyBars class/struct`
  - 2.5.2.1 The CUSUM Filter
    - C++: `CCumSum\CCumSumSADF classes`
- Chapter 3 - Labelling
  - 3.3 Computing Dynamic Thresholds
    Volatility estimates to define stop-loss and take-profit.
    - C++:  Return on Bars `CMbReturn class`. Stdev of Returns `CStdevMbReturn`
    inheriting also `CTaSTDDEV`.
  - 3.4 The Tripple Barrier Method
  - 3.5 Learning Size and Side
    - C++ : `labelling.cpp`, `events.h`, `indicators.h`
- Chapter 4 - Sample Weights
  - 4.3 Number of concurrent labels
    Uniqueness of a sample.
    Determination of sample weight by absolute return attribution.     
  - Python :
- Chapter 5 - Fractionally Differentiated Features
  - 5.4 The Method
  - 5.5 Implementation
    - C++ : `CFracDiff class` uses Libtorch API (CUDA)
- Chapter 7 - Cross-Validation in Finance
  - 7.4 A solution Purged k-fold CV
  - Python :
- Chapter 17 - Structural Breaks
  - 17.4 Explosiveness Tests
  - 17.4.2 Supremum Augmented Dickey-Fuller
  - C++ : a approximated version `CSADF class` GPU optimized w. Libtorch API (CUDA)


