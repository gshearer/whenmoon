#ifndef WM_ORDERC

// exported functions
extern const char *order_type(const int);
extern const char *order_state(const int);
extern void order_checkStale(struct exchange *);
extern void order_close(struct order *);
extern int order_findfree(void);
extern void order_update(const struct order *);
extern void order_mkstale(struct exchange *);
extern struct order *order_new(void);

// exported variables
extern struct order orders[];

#else

#include "common.h"
#include "exch.h"
#include "init.h"
#include "irc.h"
#include "market.h"
#include "misc.h"

// globals
struct order orders[ORDERS_MAX];

#endif
