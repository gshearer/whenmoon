#define WM_EXCH
#include "exch.h"

void
exch_fail(const struct exchange *exch)
{
  int x = 0;

  for(; x < MARKET_MAX; x++)
  {
    struct market *mkt = &markets[x];

    if((mkt->state & MKS_ACTIVE) && mkt->exch == exch)
      market_close(mkt);
  }
}

void
exch_status(struct exchange *exch, const int val, const int state)
{
  switch(state)
  {
    case SUCCESS:
      if(!(exch->state & val))
      {
        logger(C_EXCH, INFO, "exch_status", "%s %s api has been enabled", exch->name, (val == EXS_PUB) ? "public" : "private");
        exch->state |= val;
      }
      break;
    case FAIL:
      if(exch->state & val)
      {
        logger(C_EXCH, INFO, "exch_status", "%s %s api has been disabled", exch->name, (val == EXS_PUB) ? "public" : "private");
        exch->state &= ~val;
        exch_fail(exch);
      }
      break;
  }
}

struct product *
exch_findProduct(struct exchange *exch, const char *asset, const char *currency)
{
  int x = 0;

  for(; x < PRODUCTS_MAX && exch->products[x].currency[0]; x++)
  {
    if(!strcasecmp(asset, exch->products[x].asset) && !strcasecmp(currency, exch->products[x].currency))
      return(&exch->products[x]);
  }
  return(NULL);
}

int
exch_exportProducts(const struct exchange *exch, const char *currency, const char *path)
{
  FILE *fp = fopen(path, "w");
  int x = 0, y = 0;

  if(fp == NULL)
  {
    logger(C_EXCH, CRIT, "exch_exportproducts", "Unable to open '%s' for write: %s", path, strerror(errno));
    return(FAIL);
  }

  for(; x < PRODUCTS_MAX && exch->products[x].currency[0]; x++)
  {
    if(!strcasecmp(exch->products[x].currency, currency))
    {
      if(y++)
        fputc(',', fp);
      fprintf(fp, "%s:%s%s", exch->name, exch->products[x].asset, exch->products[x].currency);
    }
  }
  fclose(fp);
  return(SUCCESS);
}

// close current candle and copy previous candle if empty
void
exch_closeCan(struct candle *cc, const struct candle *pc)
{
  if(cc->open)
  {
    cc->vwp /= cc->volume;
    logger(C_EXCH, DEBUG3, "exch_closeCan", "accepted %d trades", cc->trades);
  }
  else // empty candle
  {
    cc->volume = 0;
    cc->trades = 0L;

    logger(C_EXCH, DEBUG3, "exch_closeCan", "#%lu is empty", cc->num);

    if(pc == NULL)
    {
      logger(C_MKT, CRIT, "exch_closeCan", "NO PREVIOUS CANDLE --- bug");
      cc->close = cc->open = cc->high = cc->low = 0;
    }
    else
    {
      cc->open = pc->open;
      cc->close = pc->close;
      cc->high = pc->high;
      cc->low  = pc->low;
      cc->vwp  = pc->vwp;
    }
  }
}

struct exchange *
exch_find(const char *name)
{
  int x = 0;

  for(;exchanges[x].name[0] != '\0' && strcasecmp(exchanges[x].name, name); x++);
  return((exchanges[x].name[0] == '\0') ? NULL : &exchanges[x]);
}

// submit order to exchange
void
exch_order(struct order *o)
{
  logger(C_EXCH, INFO, "exch_order", "creating order %s:%s (%s %s)",
      o->exch->name, o->product, order_type(o->type),
      (o->state & ORS_LONG) ? "long" : "short");
  irc_broadcast("[order create] %s:%s (%s %s)",
      o->exch->name, o->product, order_type(o->type),
      (o->state & ORS_LONG) ? "long" : "short");

  o->state |= ORS_PENDING;
  o->exch->orderPost(o);
}

// cancel order
void
exch_orderCancel(struct order *o)
{
//  if(o->state & ORS_PENDING)
//  {
//    logger(C_EXCH, WARN, "exch_orderCancel", "attempt to cancel pending order #%d (%s:%s)", o->array, o->exch->name, o->product);
//    irc_broadcast("order #%d is still pending", o->array, o->exch->name, o->product);
//    return;
//  }

  logger(C_EXCH, INFO, "exch_orderCancel", "canceling order #%d %s:%s", o->array, o->exch->name, o->product);

  irc_broadcast("canceling order #%d (%s:%s)", o->array, o->exch->name, o->product);

  o->state |= ORS_DELETING;
  o->exch->orderCancel(o);
}

void
exch_orderUpdate(struct order *o)
{
  logger(C_EXCH, DEBUG3, "exch_orderUpdate", "process order update for %s:%s <%s>", o->exch->name, o->product, o->id);
  order_update(o);
}

void
exch_accountUpdate(struct exchange *exch, const char *currency, double balance, double available, double hold)
{
  account_update(exch, currency, balance, available, hold);
}

struct product *
exch_newProd(struct exchange *exch)
{
  int x = 0;
  for(; x < PRODUCTS_MAX && exch->products[x].currency[0] != '\0'; x++);
  return((x < PRODUCTS_MAX) ? &exch->products[x] : NULL);
}

// TODO: this function does not check for products removed only added
void
exch_productUpdate(struct exchange *exch, const struct product *p)
{
  struct product *n = exch_findProduct(exch, p->asset, p->currency);

  if(n)
  {
    if(strcmp(n->status, p->status))
    {
      irc_broadcast("%s:%s-%s new status: %s (%s)",
          exch->name, n->asset, n->currency, n->status, n->status_msg);
      logger(C_EXCH, INFO, "exch_productUpdate", "%s:%s-%s new status: %s (%s)",
          exch->name, n->asset, n->currency, n->status, n->status_msg);
    }

    if((n->flags & PFL_DISABLED) && !(p->flags & PFL_DISABLED))
    {
      irc_broadcast("%s:%s-%s trading disabled by exchange", exch->name, n->asset, n->currency);
      logger(C_EXCH, INFO, "exch_productUpdate", "%s:%s-%s trading disabled by exchange", exch->name, n->asset, n->currency);
    }

    else if(!(n->flags & PFL_DISABLED) && (p->flags & PFL_DISABLED))
    {
      irc_broadcast("%s:%s-%s trading enabled by exchange", exch->name, n->asset, n->currency);
      logger(C_EXCH, INFO, "exch_productUpdate", "%s:%s-%s trading enabled by exchange", exch->name, n->asset, n->currency);
    }
  }

  else if((n = exch_newProd(exch)) != NULL)
  {
    memcpy((void *)n, (void *)p, sizeof(struct product) - sizeof(p->ticker));
    // TODO: event
    if(exch->state & EXS_PUB)
    {
      irc_broadcast("New product detected: %s:%s:%s", exch->name, n->asset, n->currency);
      logger(C_EXCH, INFO, "exch_productUpdate", "New product detected: %s:%s:%s", exch->name, n->asset, n->currency);
    }
  }
  else
    logger(C_EXCH, WARN, "exch_productUpdate", "%s has reached maximum products (%d)", exch->name, PRODUCTS_MAX);
}

// process candle history
// this function will loop through recently downloaded candle history and
// insert empty candles as necessary before sending them to the market
void
exch_pch(struct market *mkt)
{
  time_t cc_time = 0L, stop_time = candle_time(global.now - mkt->gran, mkt->gran, false);
  struct candle *pc = NULL;
  int x = mkt->c_array - 1, sc = 0;

  logger(C_EXCH, INFO, "exch_pch", "[%s] downloaded %d candles",
      mkt->name, mkt->c_array);

  if(!mkt->c_array)
  {
    logger(C_EXCH, CRIT, "exch_pch", "called with no candles?");
    market_close(mkt);
    return;
  }

  // submit candle history in order of oldest to newest
  // note that this loops backwards through the array because they were stored
  // in order of newest to oldest -- this produces output of oldest to newest
  // which is how the market algo expects candles to be delivered
  for(;cc_time < stop_time && sc < mkt->init_candles && x > -1; x--)
  {
    struct candle *c = &mkt->candles[x];

    c->num = sc + 1;

    // if the cc_time isn't set yet, that means this is the first iteration
    // in this loop -- so this is the oldest known candle which becomes
    // the real market start time. Empty candles can cause first candles
    // to be skipped so it's not necessary the start the user requested
    if(!cc_time)
    {
      mkt->start = cc_time = c->time;
      pc = NULL; // no previous candle - this is the first (oldest)
    }
    else
      pc = &mkt->candles[x + 1];

    // insert empties if necessary
    while(cc_time < stop_time && sc < mkt->init_candles && (c == NULL || (c->time > cc_time && sc < mkt->init_candles)))
    {
      struct candle ec;

      memcpy((void *)&ec, (void *)pc, sizeof(struct candle));

      ec.num = ++sc;
      ec.time = cc_time;
      cc_time += mkt->gran;   // next (older) window
      market_candle(mkt, &ec);
      mkt->empty_candles++;
    }

    // time matches, good to submit
    if(c != NULL && c->time == cc_time && c->time < stop_time)
    {
      c->num = ++sc;
      market_candle(mkt, c);
      pc = c;
      cc_time += mkt->gran;
    }
    else
      logger(C_EXCH, CRIT, "exch_pch", "candle still doesnt match time - bug");
  }

  logger(C_EXCH, INFO, "exch_pch", "[%s] submitted %d (%d empty) candles", mkt->name,
      sc, mkt->empty_candles);
}


// exchange sends trades in order of newest to oldest
void
process_trade(struct market *mkt, const struct trade *t)
{
  struct candle *c = mkt->cc;

  //mkt->ptid = mkt->tid;
  //mkt->tid = t->id;
  //if(!mkt->ftid)
  //  mkt->ftid = t->id;

  // if this trade is newer than any we've seen so far in this candle
  // update close
  if(t->time > mkt->ltt)
  {
    mkt->ltt = t->time;
    c->close = t->price;
  }

  // if this trade is older than any we've seen so far in this candle
  // update open
  else if(t->time < mkt->ott)
  {
    mkt->ott = t->time;
    c->open = t->price;
  }

  if(c->open) // candle already open
  {
    c->high = (t->price > c->high) ? t->price : c->high;
    c->low = (t->price < c->low) ? t->price : c->low;
    c->open = t->price; // oldest trade is always the open
    c->volume += t->size;
    c->vwp += (t->price * t->size);
    c->trades++;
  }

  else // candle is new
  {
    c->open = c->high = c->low = c->close = t->price;
    c->volume = t->size;
    c->vwp = t->price * t->size;
    c->trades = 1;
    mkt->ltt = mkt->ott = t->time;
  }
}

// exchanges that require trades to be processed into candles
// to keep market data uptodate call this function
int
exch_trade(struct market *mkt, const struct trade *t)
{
  mkt->state &= ~MKS_NEWEST;

  if(t->time >= mkt->cc_time && t->time <= mkt->cc_time + (mkt->gran - 1))
    process_trade(mkt, t);

  // trade happened outside current candle window 
  else
  {
    char str[50], str2[50];

    if(t->time > mkt->cc_time + (mkt->gran - 1)) // skip future trades
    {
      logger(C_EXCH, DEBUG4, "exch_trade", "skipping future trade %lu", t->id);
      return(SUCCESS);
    }

    if(!mkt->cc->trades)
      mkt->empty_candles++;

    mkt->cc->num = mkt->num_candles + 1;

    // trade is older than candle time - close & move to next int
    exch_closeCan(mkt->cc, mkt->pc);

    time2str(str, mkt->cc_time, false);
    time2str(str2, t->time, false);

    logger(C_EXCH, DEBUG3, "exch_trade", "closed cc_time: %s rej trade: %s", str, str2);

    if(!mkt->cc->open)
      logger(C_EXCH, CRIT, "exch_trade", "zero open: %lu pc: %0.9f", mkt->cc->num, mkt->pc->open);

    // submit candle to market which will advance the cc_time
    market_candle(mkt, mkt->cc);

    mkt->cc->open = 0; // clear memory for next candle
    mkt->cc->time = mkt->cc_time;

    // signal the exchange API to get the newest data next poll
    mkt->state |= MKS_NEWEST;

    return(FAIL); // tell exch api to stop processing current poll
  }
  return(SUCCESS);
}

// this function just simply stores candles downloaded via exchange API
// note that these candles are received in order of newest to oldest
// and there may be missing candles (empty intervals)
int
exch_candle(struct market *mkt, struct candle *c)
{
  struct candle *cc = &mkt->candles[mkt->c_array];

  mkt->state &= ~MKS_NEWEST; // newest is only used on first api call

  if(c->time > mkt->cc_time)
  {
    logger(C_EXCH, DEBUG3, "exch_candle", "skipping newer candle");
    return(SUCCESS);
  }

  // if we see a candle older than our market start time then we know we have
  // enough data to calculate all initially requested candles
  // this ends learning mode
  if(c->time < mkt->start || mkt->c_array >= mkt->init_candles)
  {
    mkt->state &= ~MKS_MOREDATA & ~MKS_LEARNING;

    // two flags applied here: 
    // MKS_NEWEST sets up the exchange api to poll the newest data
    // the next time we get a request. Since we're not learning
    // anymore. 
    // MKS_ENDLEARN tells the endpoll function that learning mode
    // just ended and the history needs to be sent to the market
    mkt->state |= MKS_NEWEST | MKS_ENDLEARN;

    return(FAIL); // not a fail but this tells the exchange api to stop
  }

  memcpy((void *)cc, (void *)c, sizeof(struct candle));
  mkt->c_array++;

  if(global.state & WMS_CM)
  {
    float cm_comp = (float)mkt->c_array / mkt->init_candles * 100;
    int round = (int)cm_comp % 5;

    if(cm_prog == 0 && round == 0)
    {
      cm_prog = 1;
      printf("%.2f%% complete (candles left: %d)\n",
          cm_comp, mkt->init_candles - mkt->c_array);
    }

    if(round != 0)
      cm_prog = 0;
  }

  return(SUCCESS);
}

// this is the only private-api function that will ignore EXS_PRIV
// because it's the first one to get called and must enable it
// others won't be tried if this isn't successful
int
exch_getAccounts(struct exchange *exch)
{
  if(!(exch->state & EXS_INIT))
  {
    logger(C_EXCH, WARN, "exch_getAccounts", "uninitialized exchange: %s", exch->name);
    exch->t_accounts = global.now - (EXT_ACCOUNTS - CURL_RETRYSLEEP);
    return(FAIL);
  }

  exch->t_accounts = global.now;
  logger(C_EXCH, DEBUG3, "exch_getAccounts",
      "requesting account balances for: %s", exch->name);
  account_mkstale(exch);
  return(exch->getAccounts());
}

int
exch_getCandles(struct market *mkt)
{
  if(!(mkt->exch->state & EXS_INIT))
  {
    logger(C_EXCH, WARN, "exch_getCandles",
        "uninitialized exchange: %s", mkt->exch->name);
    return(FAIL);
  }

  if(!(mkt->exch->state & EXS_PUB))
  {
    logger(C_EXCH, WARN, "exch_getCandles",
        "public api not yet enabled: %s", mkt->exch->name);
    return(FAIL);
  }

  logger(C_EXCH, DEBUG3, "exch_getCandles", "[%s] request candle history", mkt->name);

  if(mkt->exch->getCandles(mkt) == SUCCESS)
  {
    mkt->state |= MKS_POLLED;
    return(SUCCESS);
  }
  return(FAIL);
}

int
exch_getFees(struct exchange *exch)
{
  exch->t_fees = global.now;

  if(!(exch->state & EXS_INIT))
  {
    logger(C_EXCH, WARN, "exch_getFees",
        "uninitialized exchange: %s", exch->name);
    exch->t_fees = global.now - EXT_FEES - 30;
    return(FAIL);
  }

  if(!(exch->state & EXS_PRIV))
  {
    logger(C_EXCH, WARN, "exch_getFees",
        "private api not yet enabled: %s", exch->name);
    return(FAIL);
  }

  exch->t_fees = global.now;

  logger(C_EXCH, DEBUG2, "exch_getFees", "requesting fees for: %s", exch->name);
  return(exch->getFees());
}

int
exch_getOrder(struct exchange *exch, const char *id)
{
  if(!(exch->state & EXS_INIT))
  {
    logger(C_EXCH, WARN, "exch_getOrder", "uninitialized exchange: %s", exch->name);
    return(FAIL);
  }

  if(!(exch->state & EXS_PRIV))
  {
    logger(C_EXCH, WARN, "exch_getOrder", "private api not yet enabled: %s", exch->name);
    return(FAIL);
  }

  logger(C_EXCH, DEBUG1, "exch_getOrder", "requesting update for %s: %s", exch->name, id);
  return(exch->getOrder(id));
}


int
exch_getOrders(struct exchange *exch)
{
  if(!(exch->state & EXS_INIT))
  {
    logger(C_EXCH, WARN, "exch_getOrders", "uninitialized exchange: %s", exch->name);
    return(FAIL);
  }

  if(!(exch->state & EXS_PRIV))
  {
    logger(C_EXCH, WARN, "exch_getOrders", "private api not yet enabled: %s", exch->name);
    exch->t_orders = global.now - EXT_ORDERS - 30;
    return(FAIL);
  }

  exch->t_orders = global.now;

  logger(C_EXCH, DEBUG3, "exch_getOrders", "requesting order status for: %s", exch->name);
  order_mkstale(exch);
  return(exch->getOrders());
}

// this is the only function that will ignore EXS_PUB because it's the poll
// we use to determine if the public api is available
int
exch_getProducts(struct exchange *exch)
{
  exch->t_products = global.now;
  if(exch->state & EXS_INIT)
  {
    int x = 0;
    for(; x < PRODUCTS_MAX; x++)
    {
      if(exch->products[x].currency[0] != '0')
        exch->products[x].flags |= PFL_STALE;
    }
    logger(C_EXCH, DEBUG2, "exch_getProducts", "requesting products from %s", exch->name);
    return(exch->getProducts());
  }
  logger(C_EXCH, WARN, "exch_getProducts", "uninitialized exchange: %s", exch->name);
  return(FAIL);
}

int
exch_getTicker(struct exchange *exch, struct product *prod)
{
  return(exch->getTicker(prod));
}

int
exch_getTickers(struct exchange *exch)
{
  int x = 0;

  exch->t_tickers = global.now;

  if(exch->getTickers != NULL)
    return(exch->getTickers());

  return(FAIL);
 
  // cbp does not have a "get all tickers" endpoint :-( I'll get around
  // to websockets eventually
  for(; x < PRODUCTS_MAX; x++)
  {
    struct product *p = &exch->products[x];

    if(p->currency[0] != '\0') // active product
    {
      if(exch->getTicker(&exch->products[x]) == FAIL)
        return(FAIL);
      exch->products[x].ticker.lastpoll = global.now;
    }
  }
  return(SUCCESS);
}

void
exch_endpoll_accounts(struct exchange *exch, const int status)
{
  // TODO: account comparisons, event hook, etc
  exch_status(exch, EXS_PRIV, status);
}

void
exch_endpoll_fees(struct exchange *exch, const int status)
{
  exch_status(exch, EXS_PRIV, status);
}

void
exch_endpoll_products(struct exchange *exch, const int status, const int x)
{
  if(status == SUCCESS)
    logger(C_EXCH, INFO, "exch_endpoll_products", "exchange %s provides %d products", exch->name, x);
  exch_status(exch, EXS_PUB, status);
}

void
exch_endpoll_orderCancel(struct order *o)
{
  if(o->state & ORS_DELETING) // cancel attempt failed
  {
    logger(C_EXCH, WARN, "exch_endpoll_orderCancel", "cancel order #%d (%s:%s) failed: %s",
        o->array, o->exch->name, o->product, o->msg);
    irc_broadcast("cancel order #%d (%s:%s) failed: %s",
        o->array, o->exch->name, o->product, o->msg);
    // TODO: auto-retry?
  }

  else
  {
    logger(C_EXCH, INFO, "exch_endpoll_orderCancel", "order #%d (%s:%s) canceled",
        o->array, o->exch->name, o->product);
    irc_broadcast("order #%d (%s:%s) canceled",
        o->array, o->exch->name, o->product);
  }
  memset((void *)o, 0, sizeof(struct order));
}

void
exch_endpoll_orderPost(struct order *o)
{
  if(o->state & ORS_FAILED)
  {
    logger(C_EXCH, WARN, "exch_endpoll_orderPost", "order #%d (%s:%s) failed: %s",
        o->array, o->exch->name, o->product, o->msg);
    irc_broadcast("order #%d (%s:%s) failed: %s",
        o->array, o->exch->name, o->product, o->msg);

    order_close(o);

    // TODO: market state update
    // TODO: event hook
    // TODO: auto-retries?
  }

  else if(o->state & ORS_OPEN)
  {
    logger(C_EXCH, INFO, "exch_endpoll_orderPost", "order #%d (%s:%s) confirmed: %s",
        o->array, o->exch->name, o->product, o->id);
    irc_broadcast("order #%d (%s:%s) confirmed: %s",
        o->array, o->exch->name, o->product, o->id);

    // TODO: market state
    // TODO: event hook
  }

  else if(o->state & ORS_CLOSED) // taker?
  {
    logger(C_EXCH, INFO, "exch_endpoll_orderPost", "order #%d (%s:%s) closed: %s",
        o->array, o->exch->name, o->product, o->id);
    irc_broadcast("order #%d (%s:%s) closed: %s",
        o->array, o->exch->name, o->product, o->id);

    order_close(o);

    // TODO: market state / profit/loss
    // TODO: event hook
  }
}

void
exch_endpoll_orders(struct exchange *exch, const int status)
{
  exch_status(exch, EXS_PRIV, status);
  if(status == SUCCESS)
    order_checkStale(exch);
}

void
exch_endpoll_getOrder(struct exchange *exch, const int status)
{
  exch_status(exch, EXS_PRIV, status);
}

void
exch_endpoll_ticker(struct exchange *exch, struct product *prod, const int status)
{
  exch_status(exch, EXS_PUB, status);
  // TODO: respond to IRC request
}

// called by an exchange API to signal that the previous poll is done
void
exch_endpoll_data(const int status, struct market *mkt, const int num_candles)
{
  exch_status(mkt->exch, EXS_PUB, status);
  if(status == FAIL)
    return;

  mkt->state &= ~MKS_POLLED;

  // if last poll returned 0 candles, assume exchange is out of data.. thus ending learning mode
  if(!num_candles)
  {
    mkt->state &= ~MKS_MOREDATA & ~MKS_LEARNING;
    mkt->state |= MKS_NEWEST | MKS_ENDLEARN;
  }

  // this tells the exchange API to immediately poll again
  if(mkt->state & MKS_MOREDATA)
    exch_getCandles(mkt);
  
  // learning mode has ended, process the cache
  else if(mkt->state & MKS_ENDLEARN)
  {
    char str[50];

    mkt->state &= ~MKS_ENDLEARN;

    // we feed candles from oldest to newest
    mkt->cc_time = mkt->start;

    if(mkt->c_array) // if candles downloaded successfully process them
      exch_pch(mkt);

    // In CandleMaker mode -- we exit after candle dump
    if(global.state & WMS_CM)
      global.state |= WMS_END;

    if(!mkt->c_array)
    {
      logger(C_EXCH, WARN, "exch_endpoll_data",
          "[%s] no candle data available", mkt->name);
      return;
    }

    // in non-learning mode we only need cc and pc for storage
    // so we use slots 0 & 1 of the array. TODO: we could realloc()
    mkt->cc = mkt->candles;     
    mkt->pc = &mkt->candles[1];

    // copy cc (last known candle) to it's new position as previous candle
    memcpy((void *)mkt->pc, (void *)mkt->cc, sizeof(struct candle));
    
    // clear memory for incoming candle
    memset((void *)mkt->candles, 0, sizeof(struct candle));

    mkt->cc->time = mkt->cc_time;
    time2str(str, mkt->cc_time, false);
    mkt->c_array = 0; // reset array index which we reuse later
    logger(C_EXCH, DEBUG3, "exch_endpoll_data", "exiting learning mode: new cc_time is %s", str);

    mkt->state |= MKS_READY; // open for business
  }
}

void
exch_cleanup(void)
{
  int x = 0;

  logger(C_EXCH, INFO, "exch_clean", "closing all exchange apis");

  for(; exchanges[x].name[0] != '\0'; x++)
    exchanges[x].close();
}

int
exch_initExch(struct exchange *exch)
{
  int x = 0, y = 0;

  if(exch != NULL) // init a specific exchange
  {
    if(exch->state & EXS_PUB || exch->state & EXS_INIT)
    {
      logger(C_EXCH, INFO, "exch_init", "exchange already initialized: %s", exch->name);
      return(FAIL);
    }
    logger(C_EXCH, INFO, "exch_init", "initializing exchange: %s", exch->name);
    if(exch->init() == SUCCESS)
    {
      exchanges[x].state |= EXS_INIT;
      exchanges[x].t_orders = global.now - (EXT_ORDERS - 8);
      exchanges[x].t_fees = global.now - (EXT_FEES - 5);
      exchanges[x].t_tickers = global.now - (EXT_TICKERS - 60);
      return(SUCCESS);
    }
    return(FAIL);
  }

  logger(C_EXCH, INFO, "exch_init", "initializing all supported exchanges");

  for(; exchanges[x].name[0] != '\0'; x++)
  {
    if(exchanges[x].init() == SUCCESS)
    {
      exchanges[x].state |= EXS_INIT;
      exchanges[x].t_orders = global.now - (EXT_ORDERS - 8);
      exchanges[x].t_fees = global.now - (EXT_FEES - 5);
      exchanges[x].t_tickers = global.now - (EXT_TICKERS - 60);
      y++;
    }
  }

  if(y)
    return(SUCCESS);
  return(FAIL);
}

void
exch_init(void)
{
  int x = 0, y;

  memset((void *)exchanges, 0, sizeof(struct exchange) * EXCH_MAX);
  logger(C_ANY, INFO, "exch_init", "exchange manager initialized (%d max)", EXCH_MAX);

  for(; exchs[x].exch_reg != NULL; x++)
  {
    // find next empty array slot
    for(y = 0; y < EXCH_MAX && exchanges[y].name[0] != '\0'; y++);
    if(y < EXCH_MAX)
    {
      exchs[x].exch_reg(&exchanges[y]);
      logger(C_ANY, INFO, "exch_init", "registered exchange %s [%s]", exchanges[y].longname, exchanges[y].name);
    }
    else
      logger(C_ANY, CRIT, "exch_init", "max exchanges reached");
  }
}

// process exchanges
void
exch_hook(void)
{
  int x = 0;

  for(; x < EXCH_MAX; x++)
  {
    if(exchanges[x].state & EXS_INIT)
    {
      if(global.now - exchanges[x].t_products >= EXT_PRODUCTS)
        exch_getProducts(&exchanges[x]);

      else if(global.state & WMS_CM) // in candle maker mode we dont need the other endpoints
        continue;

      else if(global.now - exchanges[x].t_accounts >= EXT_ACCOUNTS)
        exch_getAccounts(&exchanges[x]);

      else if(global.now - exchanges[x].t_fees >= EXT_FEES)
        exch_getFees(&exchanges[x]);

      else if(global.now - exchanges[x].t_orders >= EXT_ORDERS)
        exch_getOrders(&exchanges[x]);

      else if(global.now - exchanges[x].t_tickers >= EXT_TICKERS)
        exch_getTickers(&exchanges[x]);
    }
  }
}
