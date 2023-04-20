#ifndef WM_EXCH

// provides these functions
extern struct exchange *exch_find(const char *);
extern void exch_accountUpdate(struct exchange *, const char *, const double, const double, const double);
extern int exch_candle(struct market *, struct candle *);
extern void exch_cleanup(void);
extern void exch_endpoll_accounts(struct exchange *, const int);
extern void exch_endpoll_data(const int, struct market *, const int);
extern void exch_endpoll_fees(struct exchange *, const int);
extern void exch_endpoll_getOrder(struct exchange *, const int);
extern void exch_endpoll_products(struct exchange *, const int, const int);
extern void exch_endpoll_orderCancel(struct order *);
extern void exch_endpoll_orderPost(struct order *);
extern void exch_endpoll_orders(struct exchange *, const int);
extern void exch_endpoll_ticker(struct exchange *, struct product *, const int);
extern int exch_exportProducts(const struct exchange *, const char *, const char *);
extern struct product *exch_findProduct(const struct exchange *, const char *, const char *);
extern int exch_getCandles(struct market *);
extern int exch_getOrder(struct exchange *, const char *);
extern void exch_hook(void);
extern void exch_init(void);
extern int exch_initExch(struct exchange *);
extern void exch_order(struct order *);
extern void exch_orderCancel(struct order *);
extern void exch_orderUpdate(struct order *);
extern void exch_productUpdate(struct exchange *, const struct product *);
extern int exch_trade(struct market *, const struct trade *);

// provides these variables
extern struct exchange exchanges[];

#else

#include "common.h"
#include "account.h"
#include "candle.h"
#include "init.h"
#include "irc.h"
#include "market.h"
#include "misc.h"
#include "order.h"

// exchanges supported
// -- Coinbase Pro
extern void cbp_register(struct exchange *);

static struct exchlist
{
  void (*exch_reg)(struct exchange *);
} exchs[] = {
  { cbp_register },
  { NULL }
};

// globals
struct exchange exchanges[EXCH_MAX];

static int cm_prog = 0;
#endif
