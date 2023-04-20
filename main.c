#define WM_MAINC
#include "main.h"

void
usage(void)
{
  banner();
 
  puts("Regular mode (bot)\n"
       "------------\n\n"
       "Usage: whenmoon configfile\n\n"
       "CandleMaker mode (download candle history for backtesting)\n"
       "----------------\n\n"
       "Usage: whenmoon -c exchange asset currency numcandles candlefile\n"
       "Where: exchange is one of: coinbase, kraken, or gemini\n"
       "       asset and currency are the market symbol\n"
       "       granularity (seconds) is one of: 60, 300, 900, 1800, 3600, 14400, 86400\n"
       "       numcandles is the total intervals to collect at specified granularity\n"
       "       outputfile is the full path including file to store text candle data\n\n"
       "Backtest mode\n"
       "-------------\n\n"
       "Usage: whenmoon -b strategy threads testcases logpath dataset [dataset ...]\n"
       "Where: strategy is one of: docdrow, macd, sma, pg TODO: make this dynamic\n"
       "       threads is the desired number of simultaneous tests\n"
       "       fee is the simulated exchange fee (percentage of fills)\n"
       "       startcur is starting currency balance\n"
       "       testcases is a simple text file with one testcase per line\n"
       "       logpath is the folder for market logs and candle plots\n"
       "       dataset is market history file created by CandleMaker\n"
       "Note:  multiple datasets are supported per backtest\n");
  exit(FAIL);
}

int
main(const int argc, const char *argv[])
{
  if(argc == 7 && (!strcasecmp(argv[1], "--candlemaker") || !strcasecmp(argv[1], "-c")))
    candlemaker(argv[2], argv[3], argv[4], atoi(argv[5]), argv[6]);

  else if(argc >= 7 && (!strcasecmp(argv[1], "--backtest") || !strcasecmp(argv[1], "-b")))
  {
    int threads = atoi(argv[3]);

    if(threads < 1)
      usage();

    backtest(argc, argv);
  }

  else if(argc == 2)
    whenmoon(argv[1]);

  else
    usage();
}
