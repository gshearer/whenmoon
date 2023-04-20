#ifndef WM_STRAT_SMAC

// exported functions
extern void strat_sma(struct strategy *);

#else

// -------------------------------------------------------------------------

#include "common.h"
#include "init.h"
#include "mem.h"
#include "misc.h"

// this is where you'll keep per-market globals if you need any
struct sma_globals
{
  // the advice struct is defined in common.h -- it allows us to
  // pass direction to the bot when your strategy is asked for advice
  struct advice a;

  // this is our only user parameter in this example. 
  int cint;
};

// -------------------------------------------------------------------------

#endif
