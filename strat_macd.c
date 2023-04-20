#define WM_STRAT_MACDC
#include "strat_macd.h"

// Simple strategy using MACD indicator on 1-hour candles

// SMA1 == 7, SMA2 == 25, SMA3 == 60,  SMA4 == 99,  SMA5 == 200
// CINTS: 0 == 1-min, 1 == 5-min, 2 == 15-min, 3 == 30-min, 4 == 1-hour, 5 == 4-hour, 6 == 1-day

void
macd_update(struct market *mkt)
{
  // your globals are allocated to 'stratdata' as a void * by your init
  struct macd_globals *glob = (struct macd_globals *)mkt->stratdata;

  // the "cint" array under the market structure is your "interval view"
  // cint[0] == 1-minute candles
  // cint[1] == 5-minute candles
  // cint[2] == 15-minute candles
  // cint[3] == 30-min candles
  // cint[4] == 1-hour candles
  // cint[5] == 4-hour
  // cint[6] == 1-day
  //
  // In this example, use param1 is a number between 0 and 6, so the user
  // can control which "view" this strategy acts on (per market too)

  struct candle_int *ci = &mkt->cint[glob->param1]; // user configured candle interval

  // the "idx" array under candle struct is your candle history stored
  // in order of newest to oldest, so 0 is the very latest candle received
  // so I refer to it in this example as 'cc' (current candle)
  // HISTSIZE defines history.. so the oldest known candle in this 'view"
  // is cint[HISTSIZE-1]

  struct candle *cc = &ci->c[ci->idx[0]];   // current candle
  struct candle *pc1 = &ci->c[ci->idx[1]];  // previous candle (1)
  struct candle *pc2 = &ci->c[ci->idx[2]];  // previous candle (2)
  struct candle *pc3 = &ci->c[ci->idx[3]];  // previous candle (3)
  struct candle *pc4 = &ci->c[ci->idx[4]];  // previous candle (4)

  glob->a.advice = ADVICE_NONE;
  glob->a.price = glob->a.pct = 0;

  // how to know if your candle view has seen enough candles for your longest
  // indicator (in this case, SMA-200) to be computed. I consider this
  // "full history" meaning indicator math can be used. if we don thave
  // enough history we dont want to make decisions.
  if(ci->tc < HISTSIZE)
    return;

  // we are long
  if(mkt->state & MKS_LONG)
  {
    // this example has no exit strategy -- which means that whenmoon's
    // built-ins (such as stoploss, trailing stoploss, greed, user interaction, etc)
    // will be the only way we go short
  }

  // we are short, look for opportunity to buy
  else
  {
    if(
          cc->sma5_diff <= glob->param2               // price is under user requirement
       && pc3->macd_h > pc4->macd_h   // macd improves for one candle
       && pc2->macd_h > pc3->macd_h   // two
       && pc1->macd_h > pc2->macd_h   // three
       && cc->macd_h > pc1->macd_h    // four

       && pc1->macd < 0                               // previous candle macd was under 0
       && cc->macd >= 0                               // macd golden cross :-)
      )
    {
      glob->a.advice = ADVICE_LONG;
      strncpy(glob->a.signal, "macd up", 49);
    }
  }
}

void
macd_advice(struct market *mkt, struct advice *give_advice)
{
  struct macd_globals *macd_glob = (struct macd_globals *)mkt->stratdata;
  struct advice *our_advice = &macd_glob->a;

  give_advice->advice = our_advice->advice;
  give_advice->price = our_advice->price;
  give_advice->pct = our_advice->pct;
  strcpy(give_advice->signal, our_advice->signal);
}

void
macd_set(struct market *mkt)
{
  return;
}

void
macd_help(struct market *mkt)
{
  return;
}

int
macd_init(struct market *mkt)
{
  char *tmp;

  // this is defined in the header file
  struct macd_globals *macd_glob;

  logger(C_STRAT, INFO, "macd_init", "macd strategy initialized for %s:%s:%s", mkt->exch->name, mkt->asset, mkt->currency);

  mkt->stratdata = (void *)mem_alloc(sizeof(struct macd_globals));
  memset(mkt->stratdata, 0, sizeof(struct macd_globals));

  macd_glob = (struct macd_globals *)mkt->stratdata;
  macd_glob->a.advice = ADVICE_NONE;

  // which interval to operate on (0 through CINTS_MAX)
  if((tmp = getenv("MACD_PARAM1")) != NULL)
    macd_glob->param1 = strtod(tmp, NULL);
  if(macd_glob->param1 < 0 || macd_glob->param1 >= CINTS_MAX)
    macd_glob->param1 = 0;

  // how far below MA_200 we must be to be looking for entry
  if((tmp = getenv("MACD_PARAM2")) != NULL)
    macd_glob->param2 = strtod(tmp, NULL);
  if(macd_glob->param2 >= 0)
    macd_glob->param2 = -5;

  return(SUCCESS);
}

int
macd_close(struct market *mkt)
{
  if(mkt->stratdata != NULL)
    mem_free(mkt->stratdata, sizeof(struct macd_globals));
  logger(C_STRAT, INFO, "macd_close", "macd strategy closed for %s:%s:%s", mkt->exch->name, mkt->asset, mkt->currency);
  return(SUCCESS);
}

void
strat_macd(struct strategy *strat)
{
  strncpy(strat->name, "macd", 49);
  strncpy(strat->desc, "example strategy using macd indicator", 249);

  // required functions
  strat->init = macd_init;
  strat->close = macd_close;
  strat->help = macd_help;
  strat->set = macd_set;
  strat->update = macd_update;
  strat->advice = macd_advice;
}
