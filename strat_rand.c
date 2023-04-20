#define WM_STRAT_RANDC
#include "strat_rand.h"

// this strategy issues advice randomly :)
// this is used for testing only

void rand_update(struct market *mkt) { return; }

void rand_set(struct market *mkt) { return; }

void rand_help(struct market *mkt) { return; }

void
rand_advice(struct market *mkt)
{
  int x = rand() % 100;

  mkt->a.pct = 0;
  mkt->a.price = 0;
  mkt->a.advice = ADVICE_NONE;

  if(mkt->state & MKS_LONG)
  {
    if(x < 5)
    {
      mkt->a.signal = "random";
      mkt->a.advice = ADVICE_SHORT;
    }
  }

  else
  {
    if(x < 2)
    {
      mkt->a.signal = "random";
      mkt->a.advice = ADVICE_LONG;
    }
  }
}

int
rand_init(struct market *mkt)
{
  logger(C_STRAT, INFO, "rand_init", "rand strategy initialized for %s:%s:%s",
    mkt->exch->name, mkt->asset, mkt->currency);
  return(SUCCESS);
}

int
rand_close(struct market *mkt)
{
  logger(C_STRAT, INFO, "rand_close", "rand strategy closed for %s:%s:%s",
      mkt->exch->name, mkt->asset, mkt->currency);
  return(SUCCESS);
}

void
strat_rand(struct strategy *strat)
{
  strncpy(strat->name, "rand", 49);
  strncpy(strat->desc, "give advice randomly", 249);

  // required functions
  strat->init = rand_init;
  strat->close = rand_close;
  strat->help = rand_help;
  strat->set = rand_set;
  strat->update = rand_update;
  strat->advice = rand_advice;
}
