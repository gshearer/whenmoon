#define WM_STRAT_DDC
#include "strat_dd.h"

void
dd_setvars(struct market *mkt)
{
  c0 = mkt->cint[0].c;
  n0 = mkt->cint[0].lc_slot;

  c1 = mkt->cint[1].c;
  n1 = mkt->cint[1].lc_slot;

  c2 = mkt->cint[2].c;
  n2 = mkt->cint[2].lc_slot;

  c3 = mkt->cint[3].c;
  n3 = mkt->cint[3].lc_slot;

  c4 = mkt->cint[4].c;
  n4 = mkt->cint[4].lc_slot;

  c5 = mkt->cint[5].c;
  n5 = mkt->cint[5].lc_slot;

  c6 = mkt->cint[6].c;
  n6 = mkt->cint[6].lc_slot;
}

void
dd_update(struct market *mkt)
{
  struct dd_globals *glob = (struct dd_globals *)mkt->stratdata;

  if(mkt->profit_pct >= glob->minprofit)
    glob->state |= SS1;
}

void
dd_advice(struct market *mkt)
{
  struct dd_globals *glob = (struct dd_globals *)mkt->stratdata;

  dd_setvars(mkt);

  if(mkt->cint[4].tc < HISTSIZE)
    return;

  if(glob->sleepcount)
  {
    glob->sleepcount--;
    return;
  }

  // SHORT
  if(mkt->state & MKS_LONG)
  {
    if(mkt->exposed > glob->maxexposure)
    {
      mkt->a.signal = "dd_max_exposed";
      mkt->a.advice = ADVICE_SHORT;
      mkt->a.price = c0[n0].close;
      //glob->sleepcount = glob->aisleep;
    } 
    
    else if(mkt->profit_pct < glob->stoploss)
    {
      mkt->a.signal = "dd_stoploss";
      mkt->a.advice = ADVICE_SHORT;
      mkt->a.price = c0[n0].close;
      glob->sleepcount = glob->aisleep;
    } 
    
    else if(glob->shint_low > 0 && c0[n0].close <= glob->shint_low)
    {
      mkt->a.signal = "dd_hint_low";
      mkt->a.advice = ADVICE_SHORT;
      mkt->a.price = c0[n0].close;
      glob->sleepcount = glob->aisleep;
    }

    else if(glob->shint_high > 0 && c0[n0].close >= glob->shint_high)
    {
      mkt->a.signal = "dd_hint_high";
      mkt->a.advice = ADVICE_SHORT;
      mkt->a.price = c0[n0].close;
    }

    else if(mkt->profit_pct >= glob->greed)
    {
      mkt->a.signal = "dd_greed";
      mkt->a.advice = ADVICE_SHORT;
      mkt->a.price = c0[n0].close;
    }

    else if(mkt->profit_pct >= glob->minprofit)
    {
      if(c0[n0].high > c0[n0].close && c0[n0].asize >= 0.1)
        return;

      if(c0[n0].close > c0[n0 - 1].close)
        return;

      mkt->a.signal = "dd_minprofit";
      mkt->a.advice = ADVICE_SHORT;
      mkt->a.price = c0[n0].close;
    }
  }

  // LONG
  else
  {
    if( !(glob->state & SS2)

        && c4[n4].high_age > 3
        && c4[n4].high_diff < -1
        && c4[n4].low_diff > 2
        && c4[n4].rsi > 25
        && c4[n4].rsi < 80

        && c3[n3].high_age > 3
        && c3[n3].rsi_sma > 30
        && c3[n3].rsi < 70

        && c1[n1].high_age > 3
        && c1[n1].rsi_sma > 30
        && c1[n1].fish < -0.2

        && ( c0[n0].high_diff > -5 || (c0[n0].high_diff <= -5 && c0[n0].high_age > 40))

        && c0[n0].sma5_diff < 0.5
        && c0[n0].asize > -0.5
        && c0[n0].high_age > 20
        && c0[n0].last5 < -0.2

      )
    {
      mkt->a.signal = "dd_long1";
      mkt->a.advice = ADVICE_LONG;
      mkt->a.msg = NULL;
      mkt->a.price = c0[n0].close;
      glob->stoploss = -5;
      glob->minprofit = 0.4;
      glob->greed = 2;
      glob->maxexposure = 2880;
      glob->aisleep = 160;
      glob->shint_high = c4[n4].ci_high;
      glob->shint_low = c4[n4].ci_low;
    }
  }

  if(mkt->a.advice == ADVICE_SHORT)
  {
    glob->shint_low = glob->shint_high = 0;
  }

  else if(mkt->a.advice == ADVICE_LONG)
    glob->state &= ~SS1;
}

int
dd_init(struct market *mkt)
{
  struct dd_globals *dd_glob;

  if((mkt->stratdata = (void *)mem_alloc(sizeof(struct dd_globals))) == NULL)
    return(FAIL);

  memset(mkt->stratdata, 0, sizeof(struct dd_globals));

  logger(C_STRAT, INFO, "dd_init", "DocDROW is stalking %s:%s-%s",
      mkt->exch->name, mkt->asset, mkt->currency);

  return(SUCCESS);
}

int
dd_close(struct market *mkt)
{
  if(mkt->stratdata != NULL)
  {
    mem_free(mkt->stratdata, sizeof(struct dd_globals));
    mkt->stratdata = NULL;
  }
  return(SUCCESS);
}

void dd_set(struct market *mkt) { return; }

void dd_help(struct market *mkt) { return; }

void
strat_dd(struct strategy *strat)
{
  strncpy(strat->name, "dd", 49);
  strncpy(strat->desc, "DocDROW Copyright (C) 2022 Shearer Tech", 249);

  // required functions
  strat->init = dd_init;
  strat->close = dd_close;
  strat->help = dd_help;
  strat->set = dd_set;
  strat->update = dd_update;
  strat->advice = dd_advice;
}
