#define WM_STRAT_PGC
#include "strat_pg.h"

// doc's playground

// setup9: 7/27/22 -- 18 trades @ 100% prof $107 on BTC vs USD
// -----------------------------------------------------------------------

double
pg_short9(struct market *mkt)
{
  if(glob->short_state & PGSS1) // minprofit achieved
  {
    if(mkt->profit_pct >= mkt->u.greed)
      return(c0[0]->close);

    if(c0[0]->size >= 0.1)
      return(0);

    return(c0[0]->close);
  }
  return(0);
}

double
pg_long9(struct market *mkt)
{
  if( 1 == 1

      && c0[0]->last10 < -1

      && c2[0]->fish > c2[0]->fish_sig
      && c2[0]->sma1_2_diff > 0.2

      && c0[0]->fish > c0[0]->fish_sig // 2118 @ 71.39%
      && c0[1]->fish <= c0[1]->fish_sig
    )
  {
    return(c0[0]->close);
  }
  return(0);
}

double
pg_short2(struct market *mkt)
{
  int cint1 = glob->params[0], cint2 = glob->params[1];

  // SL: -2.5 / Greed: 7 / minprofit: 1
  // 76 / 81.5% / $337
  //if(glob->short_state & PGSS1 && c0[1]->close > c3[0]->sma5 && c0[0]->close <= c3[0]->sma5)
  //  return(c0[0]->close);

  c0[0]->test1 = (c0[0]->close - ci0->high) / ci0->high * 100;
  c0[0]->test2 = (c0[0]->close - ci1->high) / ci1->high * 100;
  c0[0]->test3 = (c0[0]->close - ci2->high) / ci2->high * 100;
  c0[0]->test4 = (c0[0]->close - ci3->high) / ci3->high * 100;
  c0[0]->test5 = (c0[0]->close - ci4->high) / ci4->high * 100;

  //if(c0[0]->close < c4[0]->sma2)
  //  return(c0[0]->close);

//  if(glob->short_state & PGSS2)
//  {
//    if(c0[0]->close < c4[0]->sma5)
//      return(c0[0]->close);
//  }

//  if(glob->havg_diff < 0)
//    return(c0[0]->close);

//  if(glob->lavg_diff < 0)
//    return(c0[0]->close);
//      && c0[0]->close < glob->low_avg

  //if(c4[0]->fish < c4[0]->fish_sig)
  //  return(c0[0]->close);

  //if(c4[0]->sma1_angle < 0)
  //  return(c0[0]->close);

  if(glob->short_state & PGSS1) // minprofit achieved
  {
    if(mkt->profit_pct >= mkt->u.greed)
      return(c0[0]->close);

    if(c0[0]->size >= 0.1)
      return(0);

    //if(mkt->profit_pct < mkt->u.minprofit)
    //  return(c0[0]->close);

    //if(c0[0]->fish > c0[0]->fish_sig && c0[0]->fish > c0[1]->fish)
    //  return(0);

    //if(c0[0]->sma1_diff > 0 && c0[0]->sma1_diff >= c0[1]->sma1_diff)
    //  return(0);

    //if(c0[0]->close > c0[0]->sma1)
    //  return(0);

    //if(c0[0]->sma5_diff > c0[1]->sma5_diff)
    //  return(0);

    //if(c0[0]->sma2_diff > c0[1]->sma2_diff)
    //   return(0);

    return(c0[0]->close);
  }

  return(0);
}

double
pg_long3(struct market *mkt)
{
  struct candle **ct = c2;
  struct candle_int *ci;
  double temp1, temp2;

  // 1 5 2 -5 10 5 0 none none none

  switch((int)glob->params[0])
  {
    case 0: ct = c0; ci = ci0; break;
    case 1: ct = c1; ci = ci1; break;
    case 2: ct = c2; ci = ci2; break;
    case 3: ct = c3; ci = ci3; break;
    case 4: ct = c4; ci = ci4; break;
    case 5: ct = c5; ci = ci5; break;
    case 6: ct = c6; ci = ci6; break;
  }

  switch((int)glob->params[1])
  {
    case 0: temp1 = ct[0]->last3; break;
    case 1: temp1 = ct[0]->last5; break;
    case 2: temp1 = ct[0]->last10; break;
    case 3: temp1 = ct[0]->last30; break;
    case 4: temp1 = ct[0]->last60; break;
    case 5: temp1 = ct[0]->last120; break;
    case 6: temp1 = ct[0]->all; break;
  }

  switch((int)glob->params[2])
  {
    case 0: temp2 = c0[0]->last3; break;
    case 1: temp2 = c0[0]->last5; break;
    case 2: temp2 = c0[0]->last10; break;
    case 3: temp2 = c0[0]->last30; break;
    case 4: temp2 = c0[0]->last60; break;
    case 5: temp2 = c0[0]->last120; break;
    case 6: temp2 = c0[0]->all; break;
  }

  /*
   * -90.25 70.01 621 266 1774 1000.00000000000 97.49846546832 0.03744314275 -902.50153453168 SL: -5 MP: 0.8 GR: 5 [VARS] 1 2 0 -100 10 0 0 none none none
-90.25 70.01 621 266 1774 1000.00000000000 97.49846546832 0.03744314275 -902.50153453168 SL: -5 MP: 0.8 GR: 5 [VARS] 1 2 0 -100 100 0 0 none none none
-90.44 69.99 618 265 1766 1000.00000000000 95.55892527657 0.03702840333 -904.44107472343 SL: -5 MP: 0.8 GR: 5 [VARS] 1 2 0 -5 10 0 0 none none none
-90.44 69.99 618 265 1766 1000.00000000000 95.55892527657 0.03702840333 -904.44107472343 SL: -5 MP: 0.8 GR: 5 [VARS] 1 2 0 -5 100 0 0 none none none
-90.42 70.31 611 258 1738 1000.00000000000 95.77966375589 0.03758161052 -904.22033624411 SL: -5 MP: 0.8 GR: 5 [VARS] 1 2 1 -100 10 0 0 none none none
*/

  if(ci->tc < HISTSIZE)
    return(0);

  c0[0]->test1 = (c0[0]->close - ci1->low) / ci1->low * 100;
  c0[0]->test2 = (c0[0]->close - ci1->high) / ci1->high * 100;
  c0[0]->test3 = (c0[0]->close - ct[0]->sma2) / ct[0]->sma2 * 100;
  c0[0]->test4 = (c0[0]->close - ct[0]->sma5) / ct[0]->sma5 * 100;

  if( 1 == 1

      && ct[0]->rsi > 30
      && ct[0]->rsi < 75

      && ct[0]->rsi_sma > ct[7]->rsi_sma

      && ct[0]->sma1_2_diff > 0
      //&& ct[0]->sma1_2_diff > ct[1]->sma1_2_diff

      && ct[0]->sma5_diff > glob->params[3]
      && ct[0]->sma5_diff < glob->params[4]

      && ci->fish[ci->tc_fish].dir == UP
      && ci->fish[ci->tc_fish].count > 2
      //&& ci->fish[ci->tc_fish].count < 10

      && temp1 > glob->params[5]
      && temp2 < glob->params[6]

      //&& ci0->fish[ci0->tc_fish].dir == UP
      //&& ci0->fish[ci0->tc_fish].count > 2
      //&& ci0->fish[ci0->tc_fish].count < 10
    )
  {
    return(c0[0]->close);
  }
  return(0);
}

double
pg_long2(struct market *mkt)
{
  int x = 0, y;
  double low_avg = 0, high_avg = 0;
  struct trend *t[3];

  y = ci0->tc_sma5;
  for(; x < 3; x++, y = (y) ? y - 1 : TREND_MAX - 1)
    t[x] = &ci0->sma5[y];
 
  for(x = 0; x < 10; x++)
  {
    high_avg += c3[x]->high;
    low_avg += c3[x]->low;
  }

  high_avg /= 10;
  low_avg /= 10;

  c0[0]->test1 = (c4[0]->size + c4[1]->size + c4[2]->size + c4[3]->size) / 4;
  c0[0]->test2 = (c4[0]->high - c0[0]->close) / c0[0]->close * 100;
  c0[0]->test3 = (c0[0]->close - low_avg) / low_avg * 100;
  c0[0]->test4 = (c0[0]->close - glob->newest_low_sma) / glob->newest_low_sma * 100;
  c0[0]->test5 = (c0[0]->close - glob->newest_high_sma) / glob->newest_high_sma * 100;

  //if( c0[0]->num == 71617 )   return(c0[0]->close); else return(0);

  if( 1 == 1

      //&& c0[0]->rsi < 70
      //&& c1[0]->rsi < 70
      //&& c2[0]->rsi < 70
      //&& c3[0]->rsi < 70
      //&& c4[0]->rsi < 70

      //&& c4[0]->sma4_5_diff < glob->params[0]
      //&& c4[0]->sma4_5_diff > glob->params[1]

      //&& c4[0]->sma1_5_diff > -5
      //&& c4[0]->sma1_5_diff < 5
      //&& c4[0]->sma1_2_diff > 0

      //&& c1[0]->high_diff < -1

      // 50 1 1 -1 0 0.5
      //&& c4[0]->vg4 > 50
      //&& c4[0]->last3 > 1

      //&& c4[0]->size > 1
      //&& c4[1]->size > -1
      //&& c4[2]->size > 0

      //&& c4[0]->close > c4[1]->close
      //&& c4[1]->close > c4[2]->close

      //&& c0[0]->test1 > 0.5

      //&& c0[0]->test2 < 0.5

      //&& c0[0]->sma1_2_diff >= 0
      //&& c0[1]->sma1_2_diff < 0

      //&& c1[0]->size > -1
      //&& c2[0]->size > -1
      //&& c3[0]->size > -1
      //&& c4[0]->size > -1

      //&& c1[0]->sma1 > c1[4]->sma1
      //&& c1[0]->sma1_angle > 0
      //&& c1[0]->fish > c1[0]->fish_sig
      //&& c1[0]->sma1_2_diff > c1[1]->sma1_2_diff
      //&& c1[0]->sma1_2_diff > 0
      //&& c1[0]->rsi > 50
      //&& c1[0]->rsi > c1[1]->rsi
      //&& c1[0]->rsi_sma > c1[1]->rsi_sma
      //&& c1[0]->rsi >= c1[0]->rsi_sma
      //&& c1[0]->macd_h > 0
      //&& c1[0]->macd_h > c1[1]->macd_h
      //&& c1[0]->sma5_diff > glob->params[0]

      //&& c2[0]->sma1 > c2[4]->sma1
      //&& c2[0]->sma1_angle > 0
      //&& c2[0]->fish > c2[0]->fish_sig
      //&& c2[0]->sma1_2_diff > c2[1]->sma1_2_diff
      //&& c2[0]->sma1_2_diff > 0
      //&& c2[0]->rsi > 50
      //&& c2[0]->rsi > c2[1]->rsi
      //&& c2[0]->rsi_sma > c2[1]->rsi_sma
      //&& c2[0]->rsi >= c2[0]->rsi_sma
      //&& c2[0]->macd_h > 0
      //&& c2[0]->macd_h > c2[1]->macd_h
      //&& c2[0]->sma5_diff > glob->params[1]

      //&& c3[0]->sma1 > c3[4]->sma1
      //&& c3[0]->sma1_angle > 0
      //&& c3[0]->fish > c3[0]->fish_sig
      //&& c3[0]->sma1_2_diff > c3[1]->sma1_2_diff
      //&& c3[0]->sma1_2_diff > 0
      //&& c3[0]->rsi > 50
      //&& c3[0]->rsi > c3[1]->rsi
      //&& c3[0]->rsi_sma > c3[1]->rsi_sma
      //&& c3[0]->rsi >= c3[0]->rsi_sma
      //&& c3[0]->macd_h > 0
      //&& c3[0]->macd_h > c3[1]->macd_h
      //&& c3[0]->sma5_diff > glob->params[2]

      //&& c4[0]->sma1 > c4[4]->sma1
      //&& c4[0]->sma1_angle > 0
      //&& c4[0]->fish > c4[0]->fish_sig
      //&& c4[0]->sma1_2_diff > c4[1]->sma1_2_diff
      //&& c4[0]->sma1_2_diff > 0
      //&& c4[0]->rsi > 50
      //&& c4[0]->rsi > c4[1]->rsi
      //&& c4[0]->rsi_sma > c4[1]->rsi_sma
      //&& c4[0]->rsi >= c4[0]->rsi_sma
      //&& c4[0]->macd_h > 0
      //&& c4[0]->macd_h > c4[1]->macd_h
      //&& c4[0]->sma5_diff > glob->params[3]

      //&& c0[0]->sma5_diff < -0.4       // 1906 / 82.79
      //&& c0[0]->size > -0.5            // 1910 / 83.14
      //&& c0[0]->rsi_sma > 32           // 1918 / 83.21

      //&& c4[0]->vg1 > 50               // 1342 / 84.35

      //&& c0[0]->sma2_diff > 0
      //&& c0[0]->last10 < -0.6
      //&& c0[0]->rsi_sma < 30
      //&& c0[0]->vg1 > 50
      //&& c0[0]->rsi_sma > c0[1]->rsi_sma
      //&& c0[0]->test1 < -0.5
      //&& c0[0]->close <= glob->newest_low_sma

      && c0[0]->sma5_diff < -1
      //&& c0[0]->size > 0
      && c0[0]->vsma1_5_diff > 0
      && c0[0]->fish > c0[0]->fish_sig
      && c0[1]->fish <= c0[1]->fish_sig
    )
  {
    return(c0[0]->close);
  }
  return(0);
}

// short & long play 1
// ---------------------------------------------------

/*
Backtest Summary
stoploss: -2.50 greed: 5.00 minprofit: 2.00
exchange: cbp product: BTC-USD
strategy: pg (playground and testing sandbox)
first candle: 2021-01-08T00:00:00.0Z
trades: 40 wins: 19 losses: 1
win loss avg: 95.00%
start currency: 1000.00000000000 end: 1451.93921889878
fees: 0.00225244734 BTC
profit: 451.93921889878 (45.19%)
*/

double
pg_short1(struct market *mkt)
{
  // SL: -2.5 / Greed: 7 / minprofit: 1
  // 76 / 81.5% / $337
  if(glob->short_state & PGSS1 && c0[1]->close > c3[0]->sma5 && c0[0]->close <= c3[0]->sma5)
    return(c0[0]->close);

  if(mkt->profit_pct > mkt->u.minprofit)
  {
    if(c0[0]->sma1_diff > 0)
      return(0);

    if(c0[0]->rsi > c0[1]->rsi)
      return(0);

    if(c4[0]->rsi < c4[1]->rsi)
      return(c0[0]->close);

    if(c0[0]->close < c1[0]->sma2)
      return(c0[0]->close);
  }

  if(mkt->profit_pct < -2.5)
  {
    glob->price = c0[0]->close;
    return(c0[0]->close);
  }

//  if(mkt->profit_pct > 1)
//    return(c0[0]->close);

  return(0);
}

double
pg_long1(struct market *mkt)
{
  struct trend *tr[TREND_MAX];
  int cint = glob->params[0];
  struct candle_int *ci = &mkt->cint[cint];

  c0[0]->test1 = (c0[0]->close - ci0->low) / ci0->low * 100;
  c0[0]->test2 = (c0[0]->close - ci1->low) / ci1->low * 100;
  c0[0]->test3 = (c0[0]->close - ci2->low) / ci2->low * 100;
  c0[0]->test4 = (c0[0]->close - ci3->low) / ci3->low * 100;
  c0[0]->test5 = (c0[0]->close - c4[0]->close) / c4[0]->close * 100;

 // sma1/2: -0.13 sma1/4: -0.52 sma1/5: -0.36 sma2/5: -0.23 sma3/5: 0.01 sma4/5: 0.17
  if( 1 == 1

      // wc: 24 / 75 / $67

      // 14 / 85% / $64
      // 24 / 75 / $74

      //&& ci4->tc >= 575
      //&& ci4->tc <= 710

      && c3[0]->sma1_5_diff < 1

      && c4[0]->sma1_2_diff > -1
      && c4[0]->macd_h > c4[1]->macd_h

      && c4[0]->vsma1_2_diff < 0

      && c4[0]->sma1 > c4[1]->sma1

      && c4[0]->vg1 > 50
      && c4[0]->sma1_5_diff < 15
      && c4[0]->sma1_5_diff > -5

      && c0[0]->sma1_5_diff < -0.2
      && c0[0]->low_diff < 1

      && c0[0]->sma1_diff > 0
      && c0[0]->sma1_2_diff > c0[1]->sma1_2_diff
      && c0[1]->sma1_2_diff > 0
      && c0[2]->sma1_2_diff <= 0
    )
  {
    return(c0[0]->close);
  }
  return(0);
}

void
pg_setvars(struct market *mkt)
{
  int x;

  glob = (struct pg_globals *)mkt->stratdata;

  ci0 = &mkt->cint[0];
  ci1 = &mkt->cint[1];
  ci2 = &mkt->cint[2];
  ci3 = &mkt->cint[3];
  ci4 = &mkt->cint[4];
  ci5 = &mkt->cint[5];
  ci6 = &mkt->cint[6];

  for(x = 0; x < 10; x++)   c0[x] = &ci0->c[ci0->idx[x]];
  for(x = 0; x < 10; x++)   c1[x] = &ci1->c[ci1->idx[x]];
  for(x = 0; x < 10; x++)   c2[x] = &ci2->c[ci2->idx[x]];
  for(x = 0; x < 10; x++)   c3[x] = &ci3->c[ci3->idx[x]];
  for(x = 0; x < 10; x++)   c4[x] = &ci4->c[ci4->idx[x]];
  for(x = 0; x < 10; x++)   c5[x] = &ci5->c[ci5->idx[x]];
  for(x = 0; x < 10; x++)   c6[x] = &ci6->c[ci6->idx[x]];

//  for(x = 0, y = ci0->tc_hl; x < TREND_MAX; x++, y = (y) ? y - 1 : TREND_MAX - 1)
//    hl0[x] = &ci0->hl[y];
}

void
pg_advice(struct market *mkt, struct advice *adv)
{
  double price;

  pg_setvars(mkt);

  if(ci4->tc < HISTSIZE * 2)
    return;

  if(mkt->state & MKS_LONG)
  {
    if((price = pg_short2(mkt)))
    {
      adv->advice = ADVICE_SHORT;
      adv->price = price;
      strcpy(adv->signal, "short2");
      glob->short_state = glob->long_state = 0;
      memset((void *)&glob->tv, 0, sizeof(double) * 20);
    }
  }

  else
  {
    if((price = pg_long3(mkt)))
    {
      adv->advice = ADVICE_LONG;
      adv->price = price;
      strcpy(adv->signal, "long3");
      glob->short_state = glob->long_state = 0;
      memset((void *)&glob->tv, 0, sizeof(double) * 20);
    }
  }
}

void
pg_update(struct market *mkt)
{
  int x = 0, y = mkt->cint[0].tc_hl;
  struct trend *t;

  pg_setvars(mkt);

  t = &ci4->sma5[ci4->tc_sma5];

  if(mkt->profit_pct > mkt->u.minprofit)
    glob->short_state |= PGSS1;

  if(t->dir == UP && t->count > 10)
    glob->short_state |= PGSS2;

  if(c0[0]->low_diff < 1)
    glob->long_state |= PGLS1;

  glob->high_avg = glob->low_avg = 0;
  glob->prev_high_sma = glob->prev_low_sma = 0;

  glob->trend_lows = glob->trend_highs = 0;
  glob->newest_high = glob->newest_low = 0;

  for(; x < TREND_MAX; x++, y = (y) ? y - 1 : TREND_MAX - 1)
  {
    t = &mkt->cint[0].hl[y];

    if(x == 1)
    {
      glob->newest_high_sma = t->high_sma;
      glob->newest_low_sma = t->low_sma;
      glob->newest_high = t->high;
      glob->newest_low = t->low;
    }
    else if(x == 9)
    {
      glob->oldest_high_sma = t->high_sma;
      glob->oldest_low_sma = t->low_sma;
    }

    if(x)
    {
      if(glob->prev_high_sma && glob->prev_high_sma > t->high_sma)
        glob->trend_highs++;
      if(glob->prev_low_sma && t->low_sma > glob->prev_low_sma)
        glob->trend_lows++;

      glob->high_avg += t->high;
      glob->low_avg += t->low;
      glob->prev_high = t->high;
      glob->prev_low = t->low;

      glob->prev_high_sma = t->high_sma;
      glob->prev_low_sma = t->low_sma;
    }
  }

  glob->oldest_high_sma = glob->prev_high_sma;
  glob->oldest_low_sma = glob->prev_low_sma;

  glob->high_avg /= (TREND_MAX - 1);
  glob->low_avg /= (TREND_MAX - 1);
  glob->hla_diff = (glob->high_avg - glob->low_avg) / glob->low_avg * 100;

  glob->havg_diff = (glob->high_avg - c0[0]->close) / c0[0]->close * 100;
  glob->lavg_diff = (c0[0]->close - glob->low_avg) / glob->low_avg * 100;

  if(c4[0]->last3 > 1)
    glob->tv[0] = c4[0]->close;
}

int
pg_init(struct market *mkt)
{
  char *tmp;
  int x = 0;

  logger(C_STRAT, INFO, "pg_init",
      "Doc's Playground(tm) is stalking %s:%s-%s",
      mkt->exch->name, mkt->asset, mkt->currency);

  mkt->stratdata = (void *)mem_alloc(sizeof(struct pg_globals));
  memset(mkt->stratdata, 0, sizeof(struct pg_globals));

  glob = (struct pg_globals *)mkt->stratdata;

  memset((void *)glob, 0, sizeof(struct pg_globals));

  for(; x < 10; x++)
  {
    char buf[10];
    snprintf(buf, 9, "PGX%d", x);

    if((tmp = getenv(buf)) != NULL)
      glob->params[x] = strtod(tmp, NULL);
    else
      glob->params[x] = 0;
  }
  return(SUCCESS);
}

int
pg_close(struct market *mkt)
{
  if(mkt->stratdata != NULL)
    mem_free(mkt->stratdata, sizeof(struct pg_globals));

  logger(C_STRAT, INFO, "pg_close",
      "pg strategy closed for %s:%s:%s",
      mkt->exch->name, mkt->asset, mkt->currency);

  return(SUCCESS);
}

void
pg_set(struct market *mkt)
{
  return;
}

void
pg_help(struct market *mkt)
{
  return;
}

void
strat_pg(struct strategy *strat)
{
  strncpy(strat->name, "pg", 49);
  strncpy(strat->desc, "playground and testing sandbox", 249);

  // required functions
  strat->init = pg_init;
  strat->close = pg_close;
  strat->help = pg_help;
  strat->set = pg_set;
  strat->update = pg_update;
  strat->advice = pg_advice;
}

/*
double
pg_long1(struct market *mkt)
{
  struct trend *tr[TREND_MAX];
  struct candle_int *ci = ci0;

  double high = 0, highest = 0, lasthigh = 0, low = 0, lowest = 0, hldiff, diff, diff2, diff3;
  int x = 0, y, ups = 0, downs = 0;

  y = (ci->tc_hl) ? ci->tc_hl - 1 : TREND_MAX - 1;

  // HEY DONTFORGET TO TRY A "GETOUT" FLAG --- if we see a big dip below 0, but stoploss not hit, let's get out if we go positive 

  for(; x < TREND_MAX; x++, y = (y) ? y - 1 : TREND_MAX - 1)
  {
    high += ci->highlow[y].high;
    low += ci->highlow[y].low;

    if(lowest == 0 || ci->highlow[y].low < lowest)
      lowest = ci->highlow[y].low;
    if(highest == 0 || ci->highlow[y].high > highest)
      highest = ci->highlow[y].high;
  }

  high /= TREND_MAX;
  low /= TREND_MAX;
  hldiff = (high - low) / low * 100;

  diff = (c0[0]->close - low) / c4[0]->low * 100;
  diff2 = (c0[0]->close - ci1->high) / ci1->high * 100;
  diff3 = (c0[0]->close - ci->low) / ci->low * 100;

      // stoploss: -2 greed: 5 fee: 0.1
      // 3110 / 66.30% / -50.79% 
      && hldiff > 1.5
      && diff2 < 15
      && c0[0]->rsi_sma < 55
      && c0[0]->rsi_sma > 20
      && c0[0]->last3 > -5

  if( 1 == 1
      && diff < -2.25
      && c0[0]->vg1 > 50
      && c0[0]->vg2 > 50
      && c0[0]->macd_h > 0
      && c0[1]->macd_h <= 0
    )
    glob->long_state |= PGLS1;


#36721 [ 2021-02-02T12:00:00.0Z (1-min) ]
c: 34588.45 (-0.35) o: 34710.0 h: 34710.0 l: 34575.41 v: 57.48166289 s: -0.35
L3: -0.27 L5: -0.51 L10: -0.92 L30: -1.14 L60: -1.67 L200: 1.11
h: -2.98 l: 1.18 m: -0.94 d: 4.28 ath: -17.62 atl: 20.38 m: -2.18 d: 46.13
sma1: -0.37 (-0.14) sma2: -0.78 sma3: -1.21 sma4: -1.40 sma5: -1.46 (0.01)
sma1/2: -0.41 sma1/4: -1.04 sma1/5: -1.09 sma2/5: -0.68 sma3/5: -0.26 sma4/5: -0.06
vsma1/2: 38.46 vsma1/4: 77.31 vsma1/5: 17.58 vsma2/5: -15.08 vsma3/5: -31.25 vsma4/5: -33.69
vg1: 17.36 vg2: 34.59 vg3: 40.53 vg4: 44.33 vg5: 56.05
adx: 0 rsi: 21.35 (34.58) macd_h: 34575.41 fish: 0.00 fish_sig: 0.00

#38000 [ 2021-02-03T09:19:00.0Z (1-min) ]
c: 35590.65 (-0.23) o: 35683.66 h: 35683.66 l: 35575.0 v: 25.74905033 s: -0.26
L3: -0.37 L5: -0.07 L10: -0.74 L30: -1.41 L60: -1.24 L200: -2.71
h: -3.45 l: 0.04 m: -1.74 d: 3.62 ath: -15.23 atl: 23.87 m: 0.65 d: 46.13
sma1: -0.20 (-0.14) sma2: -0.78 sma3: -1.05 sma4: -1.35 sma5: -2.11 (-0.02)
sma1/2: -0.58 sma1/4: -1.15 sma1/5: -1.91 sma2/5: -1.33 sma3/5: -1.07 sma4/5: -0.76
vsma1/2: 57.23 vsma1/4: 11.46 vsma1/5: 29.59 vsma2/5: -17.58 vsma3/5: 11.36 vsma4/5: 16.27
vg1: 10.44 vg2: 16.32 vg3: 34.45 vg4: 28.30 vg5: 36.69
adx: 0 rsi: 28.70 (33.48) macd_h: 35575.0 fish: 0.00 fish_sig: 0.00


#36721 [ 2021-02-02T12:00:00.0Z (1-min) ]
c: 34588.45 (-0.35) o: 34710.0 h: 34710.0 l: 34575.41 v: 57.48166289 s: -0.35
L3: -0.27 L5: -0.51 L10: -0.92 L30: -1.14 L60: -1.67 L200: 1.11
h: -2.98 l: 1.18 m: -0.94 d: 4.28 ath: -17.62 atl: 20.38 m: -2.18 d: 46.13
sma1: -0.37 (-0.14) sma2: -0.78 sma3: -1.21 sma4: -1.40 sma5: -1.46 (0.01)
sma1/2: -0.41 sma1/4: -1.04 sma1/5: -1.09 sma2/5: -0.68 sma3/5: -0.26 sma4/5: -0.06
vsma1/2: 38.46 vsma1/4: 77.31 vsma1/5: 17.58 vsma2/5: -15.08 vsma3/5: -31.25 vsma4/5: -33.69
vg1: 17.36 vg2: 34.59 vg3: 40.53 vg4: 44.33 vg5: 56.05
adx: 0 rsi: 21.35 (34.58) macd_h: 34575.41 fish: 0.00 fish_sig: 0.00

#36722 [ 2021-02-02T12:01:00.0Z (1-min) ]
c: 34594.41 (0.02) o: 34590.0 h: 34590.0 l: 34572.63 v: 15.53749867 s: 0.01
L3: -0.33 L5: -0.39 L10: -0.78 L30: -1.17 L60: -1.56 L200: 1.10
h: -2.96 l: 1.20 m: -0.92 d: 4.28 ath: -17.61 atl: 20.40 m: -2.16 d: 46.13
sma1: -0.28 (-0.15) sma2: -0.73 sma3: -1.16 sma4: -1.37 sma5: -1.45 (0.01)
sma1/2: -0.45 sma1/4: -1.09 sma1/5: -1.17 sma2/5: -0.73 sma3/5: -0.29 sma4/5: -0.08
vsma1/2: 32.30 vsma1/4: 73.27 vsma1/5: 15.54 vsma2/5: -12.67 vsma3/5: -31.95 vsma4/5: -33.32
vg1: 28.49 vg2: 36.66 vg3: 40.16 vg4: 44.86 vg5: 56.23
adx: 0 rsi: 22.36 (33.04) macd_h: 34572.63 fish: 0.00 fish_sig: 0.00

#36723 [ 2021-02-02T12:02:00.0Z (1-min) ]
c: 34710.61 (0.34) o: 34600.71 h: 34600.71 l: 34593.49 v: 21.37575148 s: 0.32
L3: 0.35 L5: 0.08 L10: -0.17 L30: -0.68 L60: -1.40 L200: 1.40
h: -2.63 l: 1.50 m: -0.61 d: 4.24 ath: -17.33 atl: 20.81 m: -1.83 d: 46.13
sma1: 0.08 (-0.10) sma2: -0.37 sma3: -0.81 sma4: -1.02 sma5: -1.13 (0.01)
sma1/2: -0.45 sma1/4: -1.10 sma1/5: -1.20 sma2/5: -0.76 sma3/5: -0.32 sma4/5: -0.10
vsma1/2: 37.33 vsma1/4: 85.70 vsma1/5: 24.01 vsma2/5: -9.70 vsma3/5: -32.21 vsma4/5: -33.22
vg1: 40.31 vg2: 40.62 vg3: 43.08 vg4: 45.21 vg5: 56.44
adx: 0 rsi: 38.90 (31.49) macd_h: 34593.49 fish: 0.00 fish_sig: 0.00

#651 [ 2021-02-04T02:00:00.0Z (1-hour) ]
c: 37955.2 (-0.70) o: 38223.25 h: 38223.25 l: 37525.89 v: 1685.25040445 s: -0.70
L3: -0.49 L5: 1.70 L10: 1.94 L30: 6.38 L60: 14.50 L200: 17.73
h: -1.59 l: 30.18 m: 12.08 d: 32.29 ath: -9.60 atl: 32.10 m: 7.34 d: 46.13
sma1: 0.74 (0.82) sma2: 3.02 sma3: 6.92 sma4: 9.30 sma5: 12.06 (0.17)
sma1/2: 2.26 sma1/4: 8.50 sma1/5: 11.23 sma2/5: 8.78 sma3/5: 4.81 sma4/5: 2.52
vsma1/2: 33.19 vsma1/4: 62.11 vsma1/5: 6.72 vsma2/5: -19.87 vsma3/5: -30.44 vsma4/5: -34.17
vg1: 83.24 vg2: 69.12 vg3: 59.76 vg4: 51.76 vg5: 50.58
adx: 0 rsi: 70.19 (68.04) macd_h: 37525.89 fish: 0.00 fish_sig: 0.00

  // sma1/2: -0.13 sma1/4: -0.52 sma1/5: -0.36 sma2/5: -0.23 sma3/5: 0.01 sma4/5: 0.17
  if( 1 == 1

      //&& c4[0]->vg1 > 80
      //&& c4[0]->vg2 > 60
      //&& c4[0]->vg3 > 50
      //&& c4[0]->vg4 > 50
      //&& c4[0]->vg5 > 50

      //&& c4[0]->rsi > 40

      //&& c4[0]->sma1_2_diff > 2
      //&& c4[0]->sma1_4_diff > 5

      //&& c4[0]->sma1_5_diff > 0
      //&& c4[0]->sma2_5_diff > 0
      //&& c4[0]->sma3_5_diff > 0
      //&& c4[0]->sma4_5_diff > 0

      && ci4->tc >= 575
      && ci4->tc <= 710

      //&& c1[0]->sma1_angle > -0.5
      //&& c2[0]->sma1_angle > 0
      //&& c3[0]->sma1_angle > 0
      //&& c4[0]->sma1_angle > 0

      && c0[0]->sma1_5_diff > 0
      && c0[1]->sma1_5_diff <= 0

      //&& c0[0]->last3 > glob->params[0]

      //&& c0[0]->last60 < glob->params[1]
      //&& c0[0]->last60 > -10

      //&& c4[0]->sma1_angle > 0 

      //&& c0[0]->size < 0

      //&& ci4->macd[ci4->tc_macd].dir == UP
      //&& c4[0]->macd_h > c4[1]->macd_h

      //&& c0[0]->sma1 

      //&& c0[2]->sma1_2_diff < 0
      //&& c0[1]->sma1_2_diff >= 0
      //&& c0[0]->sma1_2_diff > c0[1]->sma1_2_diff

      //&& c0[0]->sma1_5_diff < -0.5

      //&& c0[0]->last60 < -1

    )
  {
    mkt->temp1 = hldiff;
    mkt->temp2 = diff;
    mkt->temp3 = diff2;
    mkt->temp4 = diff3;
    mkt->temp5 = 0;

    glob->long_state = 0;
    //mkt->temp4 = cd[0]->last60;
    //mkt->temp5 = ce[0]->last60;

    //mkt->temp2 = cb[0]->macd_h;
    //mkt->temp3 = cb[0]->sma1_5_diff;
    //mkt->temp4 = cb[0]->sma4_5_diff;
    //mkt->temp5 = cb[0]->all;
 
    glob->price = c0[0]->close;
    return(c0[0]->close);
  }
  return(0);
}
*/
