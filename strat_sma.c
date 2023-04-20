#define WM_STRAT_SMAC
#include "strat_sma.h"

// sample strategy that uses SMA's to determine exit & entry

int
sma_init(struct market *mkt)
{
  struct sma_globals *sma_glob;
  char *tmp;

  logger(C_STRAT, INFO, "sma_init", "sma strategy initialized for %s:%s:%s", mkt->exch->name, mkt->asset, mkt->currency);

  mkt->stratdata = (void *)mem_alloc(sizeof(struct sma_globals));
  memset(mkt->stratdata, 0, sizeof(struct sma_globals));

  sma_glob = (struct sma_globals *)mkt->stratdata;
  sma_glob->a.advice = ADVICE_NONE;

  // which interval to use?
  // 0 == 1-min, 1 == 5-min, 2 == 15-min,  3 == 30-min,  4 == 1-hour,  5 == 4-hour,  6 == 1-day
  tmp = getenv("SMAINT"); // environment var

  if(tmp != NULL)
    sma_glob->cint = atoi(tmp);
  else
    sma_glob->cint = 4;   // default to 1-hour candles

  return(SUCCESS);
}

int
sma_close(struct market *mkt)
{
  if(mkt->stratdata != NULL)
    free(mkt->stratdata);
  logger(C_STRAT, INFO, "sma_close", "sma strategy closed for %s:%s:%s", mkt->exch->name, mkt->asset, mkt->currency);
  return(SUCCESS);
}

void
sma_advice(struct market *mkt, struct advice *give_advice)
{
  struct sma_globals *sma_glob = (struct sma_globals *)mkt->stratdata;
  struct advice *our_advice = &sma_glob->a;

  give_advice->advice = our_advice->advice;
  strcpy(give_advice->signal, our_advice->signal);
}

void
sma_set(struct market *mkt)
{
  return;
}

void
sma_help(struct market *mkt)
{
  return;
}

// sma1 == 7 candles, sma2 == 25, sma3 == 60, sma4 == 99, sma5 == 200
void
sma_update(struct market *mkt)
{
  struct sma_globals *glob = (struct sma_globals *)mkt->stratdata;
  int cint = glob->cint;

  struct candle_int *ci = &mkt->cint[cint];
  struct candle *cc = &ci->c[ci->idx[0]];   // current candle
  struct candle *pc1 = &ci->c[ci->idx[1]];  // previous candle (1)
  struct candle *pc2 = &ci->c[ci->idx[2]];  // previous candle (2)

  glob->a.advice = ADVICE_NONE;

  if(ci->tc < HISTSIZE)
    return;

  if(mkt->state & MKS_LONG)
  {
    if(
          pc2->sma2 > pc2->sma4    // sma-25 is better than sma-99 2 candles ago
       && pc1->sma2 <= pc1->sma4   // sma-25 has touched or crossed sma-99 1 candle ago
       && cc->sma2 < cc->sma4      // and it persisted through current candle
      )
    {
      glob->a.advice = ADVICE_SHORT;
      strncpy(glob->a.signal, "SMA25/99 death cross", 49);
    }
  }

  else
  {
    if(
          pc2->sma1 < pc2->sma5
       && pc2->sma2 < pc2->sma5
       && pc2->sma3 < pc2->sma5
       && pc2->sma4 < pc2->sma5    // all four SMA's are below sma-200 2 candles ago
       && pc1->sma2 >= pc1->sma4   // sma-25 touches or crosses sma-99 1 candle ago
       && cc->sma2 > cc->sma4      // golden cross persisted
      )
    {
      glob->a.advice = ADVICE_LONG;
      strncpy(glob->a.signal, "SMA25/99 golden cross", 49);
    }
  }
}

void
strat_sma(struct strategy *strat)
{
  strncpy(strat->name, "sma", 49);
  strncpy(strat->desc, "simple moving average strategy", 249);

  // required functions
  strat->init = sma_init;
  strat->close = sma_close;
  strat->help = sma_help;
  strat->set = sma_set;
  strat->update = sma_update;
  strat->advice = sma_advice;
}
