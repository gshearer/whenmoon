#define WM_CMC
#include "cm.h"

void
cm_write_candles(const struct market *mkt)
{
  int x = 0;

  for(; x < CINTS_MAX; x++)
  {
    logger(C_CM, INFO, "cm_write_candles", "%d candles to write for int #%d", mkt->cstore_cnt[x], x);
    if(fwrite((void *)mkt->cstore[x], sizeof(struct candle), mkt->init_candles / mkt->cint[x].length, mkt->log) != mkt->init_candles / mkt->cint[x].length)
    {
      logger(C_CM, CRIT, "cm_write_candles", "error writing candle data: %s", strerror(errno));
      return;
    }
    logger(C_CM, INFO, "cm_write_candles", "wrote %d candles for int #%d", mkt->init_candles / mkt->cint[x].length, x);
  }
}

void
candlemaker(const char *exchname, const char *asset, const char *currency, const int numcandles, const char *outfile)
{
  int x = 0, mkt_init = 0;
  struct exchange *exch;
  FILE *fp;

  banner();
  puts("CandleMaker says: Make a wish!\n");

  init_globals();
  set_signals();
  exch_init();
  curl_init();
  market_init();

  if((exch = exch_find(exchname)) == NULL)
  {
    printf("Unsuppored exchange: %s\n", exchname);
    exit(FAIL);
  }

  global.conf.logchans = C_MAIN | C_MISC | C_EXCH | C_MKT | C_CM;
  global.conf.logsev = INFO;
  global.state = WMS_CM;

  if(exch_initExch(exch) == FAIL)
    exit_program("candlemaker", FAIL);

  if((fp = fopen(outfile, "wb")) == NULL)
  {
    logger(CRIT, C_CM, "candlemaker", "unable to open output file '%s': %s\n", strerror(errno));
    exit_program("candlemaker", FAIL);
  }

  while(!(global.state & WMS_END))
  {
    struct timespec r1, r2;

    update_clock();
    curl_hook();
    exch_hook();
    //sleep(1);
    usleep(250000);

    if(!(exch->state & EXS_PUB))
    {
      if(++x == 30)
      {
        logger(CRIT, C_CM, "candlemaker", "timeout waiting for exchange init");
        fclose(fp);
        exit_program("candlemaker", FAIL);
      }
    }

    else if(!mkt_init++)
    {
      if((x = market_open(exch, numcandles, asset, currency, fp, NULL, 0, 0, 0)) == -1)
      {
        fclose(fp);
        logger(CRIT, C_CM, "candlemaker", "Unable to access market\n");
        exit_program("candlemaker", FAIL);
      }

      else
      {
        struct market *mkt = &markets[x];
        struct bt_hdr bth;
        int zz = 0;

        memset((void *)&bth, 0, sizeof(struct bt_hdr));

        // in CM mode, "raw" candle data is downloaded from the exchange and
        // stored in mkt->candles. exch_pch() then dumps that store into
        // market_candle() in order of oldest to newest. Market then processes
        // candle data into indicators and other ints. Lastly, processed candle
        // data is stored into these arrays (one for each cint), in order, and
        // fwrite()'d to file specified by user for backtesting
        for(; zz < CINTS_MAX; zz++)
        {
          mkt->cstore[zz] = (struct candle *)mem_alloc(sizeof(struct candle) * mkt->init_candles / mkt->cint[zz].length);
          memset((void *)mkt->cstore[zz], 0, sizeof(struct candle) * mkt->init_candles / mkt->cint[zz].length);
        }

        bth.soa = 6809;
        bth.cst_size = sizeof(struct candle);
        bth.init_candles = mkt->init_candles;
        strcpy(bth.whenmoon, "whenmoon");
        strncpy(bth.asset, mkt->asset, 19);
        strncpy(bth.currency, mkt->currency, 19);
        strncpy(bth.exchange, exch->name, 14);
        bth.start = mkt->start;

        mkt->log = fp;

        // TODO: error checking :-)
        fwrite((void *)&bth, sizeof(struct bt_hdr), 1, fp);
      }
    }
  }
  cm_write_candles(&markets[x]);
  exit_program("candlemaker", SUCCESS);
}
