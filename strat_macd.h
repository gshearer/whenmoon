#ifndef WM_STRAT_MACDC

// exported functions
extern void strat_macd(struct strategy *);

#else

// -------------------------------------------------------------------------

#include "common.h"
#include "init.h"
#include "mem.h"
#include "misc.h"

struct macd_globals
{
  struct advice a;
  int param1;
  double param2;
};

// -------------------------------------------------------------------------

#endif
