#ifndef WM_STRAT_HODLC

// exported functions
extern void strat_hodl(struct strategy *);

#else

#include "common.h"
#include "init.h"
#include "mem.h"
#include "misc.h"

#define HODL1          (1<<0)
#define HODL2          (1<<1)

struct hodl_globals
{
  int state;
  double exit_price, high_price;
  double params[10];
};

#endif
