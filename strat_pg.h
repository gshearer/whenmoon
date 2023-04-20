#ifndef WM_STRAT_PGC

// exported functions
extern void strat_pg(struct strategy *);

#else

// -------------------------------------------------------------------------

#include "common.h"
#include "init.h"
#include "mem.h"
#include "misc.h"
#include "strat_flags.h"

struct pg_globals
{
  int state, maxexposure, short_cnum, cnum_rsiu;
  float target, greed, stoploss, profit_pct;
  double shint_low, shint_high, last_win, short_price;
  double test1, test2;
};

// -------------------------------------------------------------------------

#endif
