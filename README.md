# whenmoon

    ▄▄▌ ▐ ▄▌ ▄ .▄▄▄▄ . ▐ ▄ • ▌ ▄ ·.              ▐ ▄ 
    ██· █▌▐███▪▐█▀▄.▀·•█▌▐█·██ ▐███▪▪     ▪     •█▌▐█
    ██▪▐█▐▐▌██▀▐█▐▀▀▪▄▐█▐▐▌▐█ ▌▐▌▐█· ▄█▀▄  ▄█▀▄ ▐█▐▐▌
    ▐█▌██▐█▌██▌▐▀▐█▄▄▌██▐█▌██ ██▌▐█▌▐█▌.▐▌▐█▌.▐▌██▐█▌
     ▀▀▀▀ ▀▪▀▀▀ · ▀▀▀ ▀▀ █▪▀▀  █▪▀▀▀ ▀█▄▀▪ ▀█▄▀▪▀▀ █▪

              Cryptocurrency Trading Bot
     Written by George Shearer (george@shearer.tech)


Hello!
------

Welcome to my little project. I only get about 2 hours a week to work on this thing. I essentially wanted a bot similar to Gekko but written in C for speed of backtesting. I also wanted to learn API's of various exchanges and hopefully a thing or two about trading as well.

This bot is currently not in a useabe state. In fact, you should never use it with real money. 

TODO: I'll be writing on a lengthy readme that will explain basic usage. This should build on any modern Linux. Currently, only coinbase pro is supported -- I will be adding Kraken next.

NOTE: Whenmoon is a "candle bot". In other words, trade decisions can not be made any faster than 1-minute frequency (the smallest supported candle size). It is not an HFT bot. If you are interested in such a thing, check out "freqtrade" here on github.

Regular mode (bot)
------------

Usage: whenmoon configfile

CandleMaker mode (download candle history for backtesting)
----------------

Usage: whenmoon -c exchange asset currency numcandles candlefile
Where: exchange is one of: coinbase, kraken, or gemini
       asset and currency are the market symbol
       granularity (seconds) is one of: 60, 300, 900, 1800, 3600, 14400, 86400
       numcandles is the total intervals to collect at specified granularity
       outputfile is the full path including file to store text candle data

Backtest mode
-------------

Usage: whenmoon -b strategy threads testcases logpath dataset [dataset ...]
Where: strategy is one of: docdrow, macd, sma, pg TODO: make this dynamic
       threads is the desired number of simultaneous tests
       fee is the simulated exchange fee (percentage of fills)
       startcur is starting currency balance
       testcases is a simple text file with one testcase per line
       logpath is the folder for market logs and candle plots
       dataset is market history file created by CandleMaker
Note:  multiple datasets are supported per backtest

