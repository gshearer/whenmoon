#define WM_STRAT_HODLC
#include "strat_hodl.h"

// SMA1 == 7, SMA2 == 25, SMA3 == 60,  SMA4 == 99,  SMA5 == 200
// CINTS: 0 == 1-min, 1 == 5-min, 2 == 15-min, 3 == 30-min, 4 == 1-hour, 5 == 4-hour, 6 == 1-day

void
hodl_advice(struct market *mkt, struct advice *adv)
{
  struct hodl_globals *glob = (struct hodl_globals *)mkt->stratdata;
  int cint = glob->params[0];
  struct candle_int *ci1 = &mkt->cint[cint];

  struct candle *cb[10] = {
    &ci1->c[ci1->idx[0]],
    &ci1->c[ci1->idx[1]],
    &ci1->c[ci1->idx[2]],
    &ci1->c[ci1->idx[3]],
    &ci1->c[ci1->idx[4]],
    &ci1->c[ci1->idx[5]],
    &ci1->c[ci1->idx[6]],
    &ci1->c[ci1->idx[7]],
    &ci1->c[ci1->idx[8]],
    &ci1->c[ci1->idx[9]]
  };

  double x;

  if(ci1->tc < HISTSIZE)
    return;

  if(mkt->state & MKS_LONG)
  {
    if(cb[0]->close > glob->high_price)
      glob->high_price = cb[0]->close;

    if(glob->state & HODL2)
    {
      if(cb[0]->close < glob->exit_price) // do nothing until we recover to exit price
        return;

      glob->state &= ~HODL2;
    }

   x = (cb[0]->close - glob->high_price) / glob->high_price * 100;

    if(x < glob->params[1])
    {
      glob->exit_price = cb[0]->close;
      sprintf(adv->signal, "TS: %.2f", x);
      adv->advice = ADVICE_SHORT;
      glob->state |= HODL1;
    }
  }

  else
  {
    if(glob->state & HODL1)
    {
      if(cb[0]->close > glob->exit_price)
      {
        adv->advice = ADVICE_LONG;
        glob->state &= ~HODL1;
        strcpy(adv->signal, "get back in");
      }

      else
      {
        x = (cb[0]->close - glob->exit_price) / glob->exit_price * 100;

        if(x < glob->params[2])
        {
          glob->state &= ~HODL1;
          glob->state |= HODL2;
          adv->advice = ADVICE_LONG;
          sprintf(adv->signal, "profit: %.2f", x);
        }
      }
    }

    else if(cb[0]->sma1_5_diff < -2)
    {
      adv->advice = ADVICE_LONG;
      strcpy(adv->signal, "begin");
    }
  }
}

void
hodl_update(struct market *mkt)
{
  return;
}

int
hodl_init(struct market *mkt)
{
  struct hodl_globals *hodl_glob;
  char *tmp;
  int x = 0;

  logger(C_STRAT, INFO, "hodl_init",
      "HODL strategy initialized for %s:%s-%s",
      mkt->exch->name, mkt->asset, mkt->currency);

  mkt->stratdata = (void *)mem_alloc(sizeof(struct hodl_globals));
  memset(mkt->stratdata, 0, sizeof(struct hodl_globals));

  hodl_glob = (struct hodl_globals *)mkt->stratdata;

  for(; x < 10; x++)
  {
    char buf[10];
    snprintf(buf, 9, "PGX%d", x);

    if((tmp = getenv(buf)) != NULL)
      hodl_glob->params[x] = strtod(tmp, NULL);
    else
      hodl_glob->params[x] = 0;
  }
  return(SUCCESS);
}

int
hodl_close(struct market *mkt)
{
  if(mkt->stratdata != NULL)
    mem_free(mkt->stratdata, sizeof(struct hodl_globals));

  logger(C_STRAT, INFO, "hodl_close",
      "hodl strategy closed for %s:%s:%s",
      mkt->exch->name, mkt->asset, mkt->currency);

  return(SUCCESS);
}

void
hodl_set(struct market *mkt)
{
  return;
}

void
hodl_help(struct market *mkt)
{
  return;
}

void
strat_hodl(struct strategy *strat)
{
  strncpy(strat->name, "hodl", 49);
  strncpy(strat->desc, "hodl mostly and chase stops", 249);

  // required functions
  strat->init = hodl_init;
  strat->close = hodl_close;
  strat->help = hodl_help;
  strat->set = hodl_set;
  strat->update = hodl_update;
  strat->advice = hodl_advice;
}
