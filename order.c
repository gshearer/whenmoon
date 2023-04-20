#define WM_ORDERC
#include "order.h"

const char *
order_state(const int state)
{
  if(state & ORS_PAPER)
    return("paper");
  if(state & ORS_PENDING)
    return("pend");
  if(state & ORS_OPEN)
    return("open");
  if(state & ORS_CLOSED)
    return("done");
  return("?");
}

const char *
order_type(const int type)
{
  switch(type)
  {
    case ORT_LIMIT:
      return("limit");
    case ORT_MARKET:
      return("market");
    default:
      return("unknown");
  }
}

struct order *
order_find(const struct exchange *exch, const char *id)
{
  int x = 0;
  for(; x < ORDERS_MAX; x++)
  {
    if(orders[x].type && orders[x].exch == exch && !strncasecmp(orders[x].id, id, 99))
      return(&orders[x]);
  }
  return(NULL);
}

int
order_findfree(void)
{
  int x = 0;
  for(;orders[x].state && x < ORDERS_MAX; x++);
  return( ( x == ORDERS_MAX ) ? -1 : x );
}

struct market *
order_findmkt(const struct order *o)
{
  int x = 0;

  for(; x < MARKET_MAX; x++)
  {
    struct market *mkt = &markets[x];
    if((mkt->state & MKS_ACTIVE) && (mkt->state & MKS_ORDER) && mkt->o != NULL && mkt->o == o)
      return(mkt);
  }
  return(NULL);
}

// called when exchange sends order update with status closed
// or when paper trader matches price
void
order_close(struct order *o)
{
  const char *asset = strchr(o->product, '-');

  asset = (asset == NULL) ? o->product : asset + 1;

  logger(C_ORD, INFO, "order_close",
      "[%s:%s] order closed id: %s price: %.11f ask-size: %.7f filled-sized: %.7f fee: %.7f %s",
      o->exch->name, o->product, (o->state & ORS_PAPER) ? "paper" : o->id,
      o->price, o->size, o->filled, o->fee, asset);

  irc_broadcast(
      "[%s:%s] order closed id: %s price: %.11f ask-size: %.7f filled-sized: %.7f fee: %.7f %s",
      o->exch->name, o->product, (o->state & ORS_PAPER) ? "paper" : o->id,
      o->price, o->size, o->filled, o->fee, asset);

  // if this order belongs to a market, notify the market it closed
  if(o->mkt != NULL)
    market_trade(o->mkt);
  else
    logger(C_ORD, WARN, "order_close", "no market for order: %s", o->id);

  // remove order from orders[]
  memset((void *)o, 0, sizeof(struct order));
}

// args: current order, update order
int
order_diff(struct order *co, const struct order *ou)
{
  int change = false;

  if(co->filled != ou->filled)
  {
    change = true;
    logger(C_ORD, INFO, "order_diff", "order %s:%s update filled: %.11f (old: %.11f)",
        ou->exch->name, ou->product, ou->filled, co->filled);
    irc_broadcast("order %s:%s update filled: %.11f (old: %.11f)",
        ou->exch->name, ou->product, ou->filled, co->filled);
  }
  return(change);
}

// inserts new order into the global array and returns pointer to entry
struct order *
order_new(void)
{
  int x = order_findfree();
  struct order *o;

  if(x == -1)
  {
    logger(C_ORD, INFO, "order_new", "too many orders (%d)", ORDERS_MAX);
    return(NULL);
  }

  o = &orders[x];
  memset((void *)o, 0, sizeof(struct order));
  o->create = o->change = global.now;
  o->array = x;

  return(o);
}

// called by exch.c when an order update comes in
void
order_update(struct order *ou)
{
  struct order *o = order_find(ou->exch, ou->id);
  int changed = false;

  if(ou->state & ORS_CLOSED)
  {
    if(o != NULL)
    {
      ou->array = o->array;
      ou->mkt = o->mkt;
      memcpy((void *)o, (void *)ou, sizeof(struct order));
      order_close(o);
    }
    else
    {
      logger(C_ORD, WARN, "order_update", "order %s closed for unknown market", ou->id);
      order_close(ou);
    }
  }

  // 905664eb-09ad-43df-9092-4ff87a695743 (ADA-USD) from active on cbp

  else if(o == NULL)
  {
    if((o = order_new()) == NULL)
      return;
    ou->array = o->array;
    memcpy((void *)o, (void *)ou, sizeof(struct order));

    logger(C_ORD, INFO, "order_new", "detected new order %s:%s (%s %s)",
        o->exch->name, o->product,
        (o->state & ORS_LONG) ? "long" : "short", order_type(o->type));

    irc_broadcast("detected new order %s:%s (%s %s)",
        o->exch->name, o->product,
        (o->state & ORS_LONG) ? "long" : "short", order_type(o->type));
  }

  else
  {
    double filled = o->filled;

    changed = order_diff(o, ou);
    ou->array = o->array;
    ou->mkt = o->mkt;
    memcpy((void *)o, (void *)ou, sizeof(struct order));
    o->state &= ~ORS_UPDATE;

    if(changed)
    {
      o->change = global.now;
      
      if(o->filled != filled && o->mkt != NULL)
        o->mkt->order_candles = 0; // reset order wait candles since we just got some filled
    }

    logger(C_ORD, DEBUG3, "order_update", "processed order update for %s:%s", o->exch->name, o->product);
  }
}

// this makes all current orders stale so that incoming poll can be compared
void
order_mkstale(struct exchange *exch)
{
  int x = 0;
  for(; x < ORDERS_MAX; x++)
  {
    if(
        orders[x].type &&                    // order is active if type != 0
        orders[x].exch == exch &&            // exchange matches
        !(orders[x].state & ORS_PENDING) &&  // hasn't yet been seen by exch
        !(orders[x].state & ORS_PAPER))      // don't make paper orders stale
      orders[x].state |= ORS_STALE;
  }
}

// called by exch.c after api has updated orders
void
order_checkStale(struct exchange *exch)
{
  int x = 0;
  for(; x < ORDERS_MAX; x++)
  {
    struct order *o = &orders[x];

    if(o->type && o->state & ORS_STALE)
    {
      logger(C_ORD, WARN, "order_checkStale",
          "[%s:%s] order #%s was not in previous update",
          o->exch->name, o->product, o->id);

      exch_getOrder(o->exch, o->id);
    }
  }
}
