#define WM_STRAT_PGC
#include "strat_pg.h"

int
pg_long(struct market *mkt)
{
  struct pg_globals *glob = (struct pg_globals *)mkt->stratdata;

  struct candle *c0 = mkt->cint[0].c, *c1 = mkt->cint[1].c,
                *c2 = mkt->cint[2].c, *c3 = mkt->cint[3].c,
                *c4 = mkt->cint[4].c, *c5 = mkt->cint[5].c,
                *c6 = mkt->cint[6].c, *lc = mkt->cint[0].lc;

  int n0 = mkt->cint[0].lc_slot, n1 = mkt->cint[1].lc_slot,
      n2 = mkt->cint[2].lc_slot, n3 = mkt->cint[3].lc_slot,
      n4 = mkt->cint[4].lc_slot, n5 = mkt->cint[5].lc_slot,
      n6 = mkt->cint[6].lc_slot;

    if( 1 == 1 )
    {

      mkt->u1 = c2[n2].rsi_sma;
      mkt->u2 = c3[n3].rsi_sma;
      mkt->u3 = c4[n4].rsi_sma;

      c0[n0].test1 = c4[n4].high;
      if(c0[n0].test1 < c4[n4 - 1].high)
        c0[n0].test1 = c4[n4 -1].high;
      if(c0[n0].test1 < c4[n4 - 2].high)
        c0[n0].test1 = c4[n4 -2].high;
      if(c0[n0].test1 < c4[n4 - 3].high)
        c0[n0].test1 = c4[n4 -3].high;

      if( true

          && c4[n4].high_diff < mkt->ua[0]
          //&& c4[n4].high_diff < -5

          && c4[n4].sma1_2_diff > mkt->ua[1]
          //&& c4[n4].sma1_2_diff > 0.1

          && c4[n4].sma2_diff > mkt->ua[2]
          //&& c4[n4].sma2_diff > -5

          //&& c1[n0].t_sma1_5.dir == UP
          //&& c1[n0].t_sma1_5.count > 2
          //&& c0[n0].t_sma1_5.count < 15

        )
      {
        mkt->a.signal = "pg_long1";
        mkt->a.advice = ADVICE_LONG;
        mkt->a.price = c0[n0].close;

        glob->stoploss = -5;
        glob->target = mkt->minprofit * 1.002;
        glob->greed = 2;
        glob->maxexposure = 1500;

        //glob->shint_high = c4[n4].ci_high;
        //glob->shint_low = c4[n4].sma5 - (c4[n4].sma5 * .01);
      }
    }

  if(mkt->a.advice == ADVICE_LONG)
    return(true);
  return(false);
}

void
pg_update(struct market *mkt)
{
  struct pg_globals *glob = (struct pg_globals *)mkt->stratdata;

  if(mkt->cint[0].lc->rsi < 30)
    glob->cnum_rsiu = mkt->cint[0].lc->num;
}

void
pg_advice(struct market *mkt)
{
  struct pg_globals *glob = (struct pg_globals *)mkt->stratdata;

  struct candle *c0 = mkt->cint[0].c, *c1 = mkt->cint[1].c,
                *c2 = mkt->cint[2].c, *c3 = mkt->cint[3].c,
                *c4 = mkt->cint[4].c, *c5 = mkt->cint[5].c,
                *c6 = mkt->cint[6].c, *lc = mkt->cint[0].lc;

  int n0 = mkt->cint[0].lc_slot, n1 = mkt->cint[1].lc_slot,
      n2 = mkt->cint[2].lc_slot, n3 = mkt->cint[3].lc_slot,
      n4 = mkt->cint[4].lc_slot, n5 = mkt->cint[5].lc_slot,
      n6 = mkt->cint[6].lc_slot;

  if(mkt->cint[4].tc <= HISTSIZE)
    return;

  // SHORT
  if(mkt->state & MKS_LONG)
  {
    if(mkt->exposed > glob->maxexposure && c4[n4].sma2_angle < 0)
    {
      mkt->a.signal = "pg_max_exposed";
      mkt->a.advice = ADVICE_SHORT;
      mkt->a.price = lc->close;
    }
    
    else if(mkt->profit_pct < glob->stoploss)
    {
      mkt->a.signal = "pg_stoploss";
      mkt->a.advice = ADVICE_SHORT;
      mkt->a.price = lc->close;
    } 
    
    else if(glob->shint_low > 0 && lc->close <= glob->shint_low)
    {
      mkt->a.signal = "pg_hint_low";
      mkt->a.advice = ADVICE_SHORT;
      mkt->a.price = lc->close;
    }

    else if(glob->shint_high > 0 && lc->close >= glob->shint_high)
    {
      mkt->a.signal = "pg_hint_high";
      mkt->a.advice = ADVICE_SHORT;
      mkt->a.price = lc->close;
    }

    else if(mkt->profit_pct >= glob->greed)
    {
      mkt->a.signal = "pg_greed";
      mkt->a.advice = ADVICE_SHORT;
      mkt->a.price = lc->close;
    }
    
    /*
    else if(c0[n0].close < c1[n1].sma5)
    {
      mkt->a.signal = "pg_parachute";
      mkt->a.advice = ADVICE_SHORT;
      mkt->a.price = lc->close;
    } */

    else if(glob->target > 0 && lc->close >= glob->target) // minprofit achieved
    {
      //if(c0[n0].sma1 > c4[n4].sma1)
      //  return;
       //if(c0[n0].sma1_diff > 0.6)
       //  return;
      // if(c0[n0].asize > 1.5)
      //   return;
      //if(c2[n2].sma1_angle > 0.4 && c2[n2].close > c2[n2].sma1)
      //  return;

      //if(c0[n0].close < c0[n0].sma1)
      //  return;

      mkt->a.signal = "pg_minprofit";
      mkt->a.advice = ADVICE_SHORT;
      mkt->a.price = lc->close;
    }
  }

  // LONG
  else
    pg_long(mkt);

  if(mkt->a.advice == ADVICE_SHORT)
  {
    glob->shint_low = glob->shint_high = 0;
    glob->short_cnum = lc->num;
    glob->short_price = mkt->a.price;
    glob->profit_pct = mkt->profit_pct;
  }
}

int
pg_init(struct market *mkt)
{
  struct pg_globals *glob;

  if((mkt->stratdata = (void *)mem_alloc(sizeof(struct pg_globals))) == NULL)
    return(FAIL);

  memset(mkt->stratdata, 0, sizeof(struct pg_globals));

  logger(C_STRAT, INFO, "pg_init", "doc's playground and testing sandbox");

  glob = (struct pg_globals *)mkt->stratdata;

  glob->stoploss = -5;
  glob->target = 0;
  glob->greed = 5;
  glob->maxexposure = 2880;

  glob->cnum_rsiu = 0;

  return(SUCCESS);
}

int
pg_close(struct market *mkt)
{
  if(mkt->stratdata != NULL)
  {
    mem_free(mkt->stratdata, sizeof(struct pg_globals));
    mkt->stratdata = NULL;
  }
  return(SUCCESS);
}

void pg_set(struct market *mkt) { return; }

void pg_help(struct market *mkt) { return; }

void
strat_pg(struct strategy *strat)
{
  strncpy(strat->name, "pg", 49);
  strncpy(strat->desc, "Doc's playground and testing strat", 249);

  // required functions
  strat->init = pg_init;
  strat->close = pg_close;
  strat->help = pg_help;
  strat->set = pg_set;
  strat->update = pg_update;
  strat->advice = pg_advice;
}
