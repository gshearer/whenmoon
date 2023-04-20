#ifndef WM_STRAT_SCOREC

// exported functions
extern void strat_score(struct strategy *);

#else

// -------------------------------------------------------------------------

#include "common.h"
#include "init.h"
#include "market.h"
#include "mem.h"
#include "misc.h"

#include "strat_flags.h"

struct score_globals
{
  int sleepcount, state, maxexposure, aisleep, score;
  float minprofit, greed, stoploss;
  double shint_low, shint_high;
  char umsg[100];
};

// -------------------------------------------------------------------------

#endif
