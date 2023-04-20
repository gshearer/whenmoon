#ifndef WM_STRAT_DDC

// exported functions
extern void strat_dd(struct strategy *);

#else

// -------------------------------------------------------------------------

#include "common.h"
#include "init.h"
#include "mem.h"
#include "misc.h"

#include "strat_flags.h"

struct dd_globals
{
  int sleepcount, state, maxexposure, aisleep;
  float minprofit, greed, stoploss;
  double shint_low, shint_high;
};

static struct candle *c0, *c1, *c2, *c3, *c4, *c5, *c6;
static int n0, n1, n2, n3, n4, n5, n6;

// -------------------------------------------------------------------------

#endif
