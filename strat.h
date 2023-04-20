#ifndef WM_STRATC

// exported functions
extern struct strategy *strat_find(const char *);
extern void strat_init(void);

// exported variables
extern struct strategy strategies[STRAT_MAX];

#else

#include "common.h"
#include "misc.h"

// add your strategy header file here

#include "strat_dd.h"
#include "strat_pg.h"
#include "strat_rand.h"
#include "strat_score.h"

// and your init function here
struct stratlist
{
  void (*strat_reg)(struct strategy *);
} strats[] = {
  { strat_dd },
  { strat_pg },
  { strat_rand },
  { strat_score },
  { NULL } // null terminated array
};

// -------------------------------------------------------------------------

// globals
struct strategy strategies[STRAT_MAX];

#endif
