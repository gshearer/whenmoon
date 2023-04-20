#ifndef WM_MARKETC
// exported functions
extern void market_ai(struct market *, const int);
extern void market_buy(struct market *);
extern void market_candle(struct market *, const struct candle *);
extern void market_close(struct market *);
extern void market_cleanup(void);
extern void market_conf(struct market *);
extern struct market *market_find(const struct exchange *, const char *, const char *);
extern void market_hook(void);
extern void market_init(void);
extern void market_log(const struct market *);
extern void market_mklog(struct market *);
extern int market_open(struct exchange *, const int, const char *, const char *, FILE *, struct strategy *, const double, const double, time_t);
extern int market_score(const struct market *, const int);
extern void market_sell(struct market *);
extern void market_sleep(struct market *, const int);
extern void market_trade(struct market *);

// exported variables
extern struct market markets[];

#else

#include <sys/stat.h>

#include "common.h"
#include "colors.h"
#include "account.h"
#include "candle.h"
#include "exch.h"
#include "init.h"
#include "irc.h"
#include "mem.h"
#include "misc.h"
#include "order.h"
#include "strat.h"
#include "tulip.h"

#include <math.h>

// markets that whenmoon is actively stalkng :-)
struct market markets[MARKET_MAX];

#endif
