#define WM_STRAT_SCOREC
#include "strat_score.h"

void
score_update(struct market *mkt)
{
  struct score_globals *glob = (struct score_globals *)mkt->stratdata;

  int cint = 0, score = 0, possible = 0;

  glob->score = 0;

  if(mkt->cint[4].tc < HISTSIZE)
    return;

  for(; cint < CINTS_MAX; cint++)
  {
    score += mkt->cint[cint].lc->score;
    possible += CINT_SCORE_MAX;
  }

  glob->score = ((float)score / possible) * 100;
}

void
score_advice(struct market *mkt)
{
  struct score_globals *glob = (struct score_globals *)mkt->stratdata;

  struct candle *c0[] = { mkt->cint[0].lc, &mkt->cint[0].c[mkt->cint[0].tc - 2] };
  struct candle *c1[] = { mkt->cint[1].lc, &mkt->cint[1].c[mkt->cint[1].tc - 2] };

  if(!(global.state & WMS_BT))
    snprintf(glob->umsg, 99, "score: %d", glob->score);

  if(glob->sleepcount)
  {
    glob->sleepcount--;
    return;
  }

  if(mkt->cint[4].tc < HISTSIZE)
    return;

  // SHORT
  if(mkt->state & MKS_LONG)
  {
    if(mkt->exposed > glob->maxexposure)
    {
      mkt->a.signal = "score_max_exposed";
      mkt->a.advice = ADVICE_SHORT;
      mkt->a.price = c0[0]->close;
      //glob->sleepcount = glob->aisleep;
    } 
    
    else if(mkt->profit_pct < glob->stoploss)
    {
      mkt->a.signal = "score_stoploss";
      mkt->a.advice = ADVICE_SHORT;
      mkt->a.price = c0[0]->close;
      glob->sleepcount = glob->aisleep;
    } 
    
    else if(glob->shint_low > 0 && c0[0]->close <= glob->shint_low)
    {
      mkt->a.signal = "score_hint_low";
      mkt->a.advice = ADVICE_SHORT;
      mkt->a.price = c0[0]->close;
      glob->sleepcount = glob->aisleep;
    }

    else if(glob->shint_high > 0 && c0[0]->close >= glob->shint_high)
    {
      mkt->a.signal = "score_hint_high";
      mkt->a.advice = ADVICE_SHORT;
      mkt->a.price = c0[0]->close;
    }

    else if(mkt->profit_pct >= glob->greed)
    {
      mkt->a.signal = "score_greed";
      mkt->a.advice = ADVICE_SHORT;
      mkt->a.price = c0[0]->close;
    }

    else if(mkt->profit_pct >= glob->minprofit) // minprofit achieved

    //else if(glob->score < mkt->ua[3] && mkt->exposed > 10)
    {
      if(glob->score > 25)
          return;
      mkt->a.signal = glob->umsg;
      mkt->a.advice = ADVICE_SHORT;
      mkt->a.price = c0[0]->close;
    }
  }

  // LONG
  else
  {
    c0[0]->test1 = glob->score;

    if( 1 == 1
        && glob->score >= 1
        && glob->score <= 15
      )
    {
      mkt->a.signal = glob->umsg;
      mkt->a.advice = ADVICE_LONG;
      mkt->a.price = c0[0]->close;
      glob->stoploss = -5;
      glob->minprofit = 1;
      glob->greed = 3;
      glob->maxexposure = 2880;
      glob->aisleep = 160;
      //glob->shint_high = c4[0]->ci_high;
      //glob->shint_low = c4[0]->ci_low;
    }
  }


  if(mkt->a.advice == ADVICE_SHORT)
  {
    glob->shint_low = glob->shint_high = 0;
  }
}

int
score_init(struct market *mkt)
{
  struct score_globals *score_glob;

  if((mkt->stratdata = (void *)mem_alloc(sizeof(struct score_globals))) == NULL)
    return(FAIL);

  memset(mkt->stratdata, 0, sizeof(struct score_globals));

  logger(C_STRAT, INFO, "dd_init", "DocDROW is stalking %s:%s-%s",
      mkt->exch->name, mkt->asset, mkt->currency);

  return(SUCCESS);
}

int
score_close(struct market *mkt)
{
  if(mkt->stratdata != NULL)
  {
    mem_free(mkt->stratdata, sizeof(struct score_globals));
    mkt->stratdata = NULL;
  }
  return(SUCCESS);
}

void score_set(struct market *mkt) { return; }

void score_help(struct market *mkt) { return; }

void
strat_score(struct strategy *strat)
{
  strncpy(strat->name, "score", 49);
  strncpy(strat->desc, "score based", 249);

  // required functions
  strat->init = score_init;
  strat->close = score_close;
  strat->help = score_help;
  strat->set = score_set;
  strat->update = score_update;
  strat->advice = score_advice;
}
