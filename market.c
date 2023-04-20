#define WM_MARKETC
#include "market.h"

// the general idea
//
// new candles come into market_candle() (sent by exchange APIs)
// the market_candle() function accepts 1-minute candles, processes them into
// various other candles (5, 15, 30, and so on) -- then feeds them to the
// market_update() function which performs indicator / moving average calcs
// and finally consults builtin policies and strategies for buy/long advice
// if advice is given, an order is created by market_buy / market_sell funcs
// and  sent to the exchange if not paper trading

// Higher the better
// HEY! if anything is modified in this function, it will invalidate candlemaker
// files because this is computed per candle during download
int
market_score(const struct market *m, const int i)
{
  struct candle *c[2];
  int s = 0;

  // if we haven't seen enough candles for a particular interval, consider it at least
  // 50% good since we don't really know.
  if(m->cint[i].tc < HISTSIZE)
    return(CINT_SCORE_MAX / 2);
  
  c[0] = m->cint[i].lc;
  c[1] = &m->cint[i].c[HISTSIZE - 2];

  if(c[0]->low_age > 0 && c[0]->low_diff > 0) // +1
    s += 1;  // not at the bottom

  if(c[0]->low_diff > 1)  // +1
    s += 1;  // at least 1% above the low

  if(c[0]->high_age > 0 && c[0]->high_diff > 0)  // +1
    s += 1;  // not at the top

  if(c[0]->high_diff < -1)
    s += 1;  // at least 1% under the high // +1

  if(c[0]->macd_h > 0 && c[0]->macd_h > c[1]->macd_h)
    s += 1;  // MACD is positive and improving

  if(c[0]->rsi < 70 && c[0]->rsi > 30) // +1
    s += 1;  // RSI in good range
  
  if(c[0]->rsi > c[0]->rsi_sma)
    s += 1;  // RSI is above normal
  
  if(c[0]->vsma1_5_diff > 0)
    s += 1;  // more volume than usual (sma5)

  if(c[0]->adx > 20)  // +1
    s += 1;  // reasonably strong trend?

  if(c[0]->vg5 > 50)
    s += 1;  // longest sma is 50% green volume or better

  if(c[0]->t_fish.dir == UP)
    s += 1;  // fisher transform is up

  if(c[0]->t_sma1_2.dir == UP) // +1
    s += 1;  // sma1 over sma2

  //if(c[0]->fish < 2)
  //  s += 1;

  if(c[0]->t_sma1_2.dir == UP && c[0]->t_sma1_2.count > 5 && c[0]->sma1_2_diff >= 0.1)
    s += 1;  // trend is holding for 5 count // +1

  if(c[0]->t_sma1_5.dir == UP)
    s += 2;  // sma1 over sma5

  if(c[0]->t_sma1_5.dir == UP && c[0]->t_sma1_5.count > 5 && c[0]->sma1_5_diff >= 0.1)
    s += 2;  // trend is holding for 5 count

  if(c[0]->size < 1.5)
    s += 1;  // no crazy stuff going on

  // 18 max
  return(s);
}

void
market_sleep(struct market *mkt, int sleepcount)
{
  mkt->sleepcount += sleepcount;

  irc_broadcast("%s automated trading sleeps for %d minutes",
      mkt->name, sleepcount);
  
  if(mkt->log != NULL)
    fprintf(mkt->log, "%s automated trading sleeps for %d minutes",
        mkt->name, sleepcount);
}

void
market_wake(struct market *mkt)
{
  irc_broadcast("%s automated trading wakes", mkt->name);
  
  if(mkt->log != NULL)
    fprintf(mkt->log, "%s automated trading wakes", mkt->name);
}

void
market_ai(struct market *mkt, int state)
{
  if(state == true && !(mkt->opt & MKO_AI))
  {
    mkt->opt |= MKO_AI;
    irc_broadcast("%s automated trading enabled", mkt->name);
    if(mkt->log != NULL)
      fprintf(mkt->log, "%s automated trading enabled", mkt->name);
  }

  if(state == false && (mkt->opt & MKO_AI))
  {
    mkt->opt &= ~MKO_AI;
    irc_broadcast("%s automated trading disabled", mkt->name);
    if(mkt->log != NULL)
      fprintf(mkt->log, "%s automated trading disabled", mkt->name);
  }
}

void
market_mklog(struct market *mkt)
{
  char p[300], temp[50];

  time2strf(temp, "%F %T", mkt->start, false);

  snprintf(p, 299, "%s/%s-%s-%s",
      global.conf.logpath, mkt->exch->name, mkt->asset, mkt->currency);

  if(mkdir(p, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH))
    logger(C_MKT, WARN, "market_mklog", "mkdir error '%s': %s (ignoring)",
        p, strerror(errno));

  snprintf(p, 299, "%s/%s-%s-%s/market.log", global.conf.logpath,
      mkt->exch->name, mkt->asset, mkt->currency);

  if((mkt->log = fopen(p, "w")) != NULL)
    fprintf(mkt->log,
        "WHENMOON - Market Log - %s market begin: %s\nstrategy: %s (%s)\n",
        mkt->name, temp, mkt->strat->name, mkt->strat->desc);
  else
    logger(C_MKT, CRIT, "market_mklog",
        "error opening market log '%s': %s (ignoring)", p, strerror(errno));
}

// TODO: events
// log the confirmed close of an order
void
market_log(const struct market *mkt)
{
  if(mkt->state & MKS_LONG)
  {
    if(mkt->log != NULL)
      fprintf(mkt->log,
          "\n#%d (( %sLONG%s CLOSE )) t: %d price: %.11f filled: %.11f fee: %.11f\n",
          mkt->cint[0].lc->num, YELLOW, ARESET,
          (mkt->opt & MKO_PAPER) ? mkt->stats.p_trades : mkt->stats.trades,
          mkt->o->price, mkt->o->filled, mkt->o->fee);

    if(!(global.state & WMS_BT))
      irc_broadcast("[%s:%s-%s] (( LONG CLOSE )) #%d price: %.11f filled: %.11f fee: %.11f",
          mkt->exch->name, mkt->asset, mkt->currency,
          (mkt->opt & MKO_PAPER) ? mkt->stats.p_trades : mkt->stats.trades,
          mkt->o->price, mkt->o->filled, mkt->o->fee);
  }

  else
  {
    if(mkt->log != NULL)
      fprintf(mkt->log,
          "\n#%d (( %sSHORT%s CLOSE )) t: %d price: %.11f filled: %.11f fee: %.11f\n",
          mkt->cint[0].lc->num, YELLOW, ARESET,
          (mkt->opt & MKO_PAPER) ? mkt->stats.p_trades : mkt->stats.trades,
          mkt->o->price, mkt->o->filled, mkt->o->fee);

    if(!(global.state & WMS_BT))
      irc_broadcast(
          "(( SHORT CLOSE )) [%s:%s-%s] #%d price: %.11f filled: %.11f fee: %.11f",
          mkt->exch->name, mkt->asset, mkt->currency,
          (mkt->opt & MKO_PAPER) ? mkt->stats.p_trades : mkt->stats.trades,
          mkt->o->price, mkt->o->filled, mkt->o->fee);

    if(mkt->profit > 0)
    {
      if(mkt->log != NULL)
        fprintf(mkt->log,
          ">> WIN << [%s:%s-%s] prof: %.11f (%.2f%%) (high: %.2f low: %.2f exp: %d fee: %.7f)\n",
          mkt->exch->name, mkt->asset, mkt->currency, mkt->profit, mkt->profit_pct,
          mkt->profit_pct_high, mkt->profit_pct_low, mkt->exposed, mkt->o->fee);

      irc_broadcast(
          ">> WIN << [%s:%s-%s] prof: %.11f (%.2f%%) (high: %.2f low: %.2f exp: %d fee: %.7f)",
          mkt->exch->name, mkt->asset, mkt->currency, mkt->profit, mkt->profit_pct,
          mkt->profit_pct_high, mkt->profit_pct_low, mkt->exposed, mkt->o->fee);
    }

    else
    {
      if(mkt->log != NULL)
        fprintf(mkt->log,
          ">> LOSS << [%s:%s-%s] prof: %.11f (%.2f%%) (high: %.2f low: %.2f exp: %d fee: %.7f)\n",
          mkt->exch->name, mkt->asset, mkt->currency, mkt->profit, mkt->profit_pct,
          mkt->profit_pct_high, mkt->profit_pct_low, mkt->exposed, mkt->o->fee);

      irc_broadcast(
          ">> LOSS << [%s:%s-%s] prof: %.11f (%.2f%%) (high: %.2f low: %.2f exp: %d fee: %.7f)",
          mkt->exch->name, mkt->asset, mkt->currency, mkt->profit, mkt->profit_pct,
          mkt->profit_pct_high, mkt->profit_pct_low, mkt->exposed, mkt->o->fee);
    }
  }
}

// in real mode, this is called when an exchange confirms an order was filled
// -or- when we canceled an order due to timeout etc but still got some filled
// in paper mode, this is called when candle price meets buy/sell price
void
market_trade(struct market *mkt)
{
  struct candle *lc = mkt->cint[0].lc;
  struct order *o = mkt->o;

  mkt->state &= ~MKS_ORDER;

  if(!o->filled)
  {
    if(mkt->log != NULL)
      fprintf(mkt->log, "#%d (( %s%s%s CANCELED )) no fills, no fees\n",
        lc->num, YELLOW, (mkt->state & MKS_LONG) ? "LONG" : "SHORT",  ARESET);
    if(mkt->state & MKS_LONG)
      mkt->state &= ~MKS_LONG & ~MKS_TRIMSTOP;
    return;
  }

  // buy
  if(mkt->state & MKS_LONG)
  {
    mkt->buy_size = o->filled;
    mkt->buy_price = o->price;
    mkt->buy_time = lc->time;

    // actual cost of acquisition of buy_size includes exchange fee
    mkt->buy_funds = (mkt->buy_size * mkt->buy_price) + o->fee;

    // paper trade
    if(o->state & ORS_PAPER)
    {
      mkt->stats.p_trades++;
      mkt->stats.p_fees += o->fee;
    }

    // real trade
    else
    {
      mkt->stats.trades++;
      mkt->stats.fees += o->fee;
    }
  }

  else // SHORT
  {
    double sell_funds = (o->filled * o->price) - o->fee;

    mkt->profit = sell_funds - mkt->buy_funds;
    mkt->profit_pct = mkt->profit / mkt->buy_funds * 100;

    mkt->sell_time = lc->time;

    // paper trade
    if(o->state & ORS_PAPER)
    {
      mkt->stats.p_trades++;
      mkt->stats.p_fees += o->fee;
      mkt->stats.p_profit += mkt->profit;

      if(mkt->profit > 0)
        mkt->stats.p_wins++;
      else
        mkt->stats.p_losses++;
    }

    // real trade
    else
    {
      mkt->stats.trades++;
      mkt->stats.fees += o->fee;
      mkt->stats.profit += mkt->profit;

      if(mkt->profit > 0)
        mkt->stats.wins++;
      else
        mkt->stats.losses++;
    }

    market_sleep(mkt, 60);
  }

  market_log(mkt);
  mkt->exposed = 0;
  mkt->o = NULL;
}

// paper trader
void
market_paper(struct market *mkt)
{
  struct order *o = mkt->o;

  if( (mkt->state & MKS_ORDER) &&
      (mkt->opt & MKO_PAPER) && (o->state & ORS_PAPER) )
  {
    struct candle *lc = mkt->cint[0].lc;

    if( ((o->state & ORS_LONG) && lc->low <= o->price) ||  // long fill
        (!(o->state & ORS_LONG) && lc->high >= o->price))   // short fill
    {
      // TODO: simultae slipage
      o->filled = o->size;

      // TODO: simulate maker and taker fees in paper mode
      o->fee = o->price * o->filled * mkt->exch->fee_maker;

      o->state |= ORS_CLOSED;
      o->state &= ~ORS_OPEN;
      order_close(o);
    }
  }
}

// create order in memory (and submit to exchange if in real mode)
void
market_order(struct market *mkt)
{
  struct order *o = order_new();

  if(o == NULL)
    return;

  mkt->state |= MKS_ORDER; // order pending flag

  mkt->o = o;
  o->mkt = mkt; // TODO: redundant

  o->exch = mkt->exch;
  o->type = ORT_LIMIT;  // TODO: support others

  if(mkt->opt & MKO_PAPER)
    o->state |= ORS_PAPER;

  if(mkt->state & MKS_LONG)
  {
    o->size = mkt->buy_size;
    o->price = mkt->buy_price;
    o->state |= ORS_LONG;
  }
  else
  {
    o->size = mkt->sell_size;
    o->price = mkt->sell_price;
  }

  snprintf(o->product, 99, "%s-%s", mkt->asset, mkt->currency);

  // if in paper mode, immediately execute order if price matches
  if(mkt->opt & MKO_PAPER)
    market_paper(mkt); 

  // if real trading, send it to the exchange
  else if(!(mkt->opt & MKO_PAPER) && !(global.state & WMS_CM))
    exch_order(o);
}

// this function changes the market's posture to LONG and calculates the
// size and price of the order based on user options and/or account balances
void
market_buy(struct market *mkt)
{
  struct candle *lc = mkt->cint[0].lc;
  double bal, cur, price, scale;
  char temp[3][50];
  int x = 0;

  if(mkt->state & MKS_ORDER)
  {
    logger(C_MKT, WARN, "market_buy",
        "[%s:%s-%s] attempt to buy while an order is pending",
        mkt->exch->name, mkt->asset, mkt->currency);
    return;
  }

  // use advice price otherwise last close
  price = (mkt->a.price) ? mkt->a.price : lc->close;

  // determine currency balance
  if(!(mkt->opt & MKO_PAPER)) // in real mode - check exchange accounts
  {
    struct account *a = account_find(mkt->exch, mkt->currency);

    if(a == NULL)
    {
      logger(C_MKT, WARN, "market_buy", "[%s:%s-%s] couldn't determine %s balance",
          mkt->exch->name, mkt->asset, mkt->currency, mkt->currency);
      return;
    }

    bal = a->available;
  }

  else // paper mode uses what we gave at market start plus paper profits
    bal = mkt->stats.p_start_cur + mkt->stats.p_profit;

  // shave 1% off available balance to pay for exchange fee
  bal = bal - (bal * .01);

  // determine how much currency to use for this order
  cur = (mkt->u.funds == 0 && mkt->u.pct == 0) ? // no user preferred amount
        (mkt->a.pct) ? bal * (mkt->a.pct / 100) :  // strategy gives pct advice
        bal : // otherwse go all-in
        (mkt->u.funds) ? mkt->u.funds :  // user defines specific currency qty
        bal * ((float)mkt->u.pct / 100); // user defines pct of balance to use

  if(cur > bal)
    cur = bal;

  scale = 1.0 / mkt->p->quote_increment;
  mkt->buy_price = trunc(scale * price) / scale;

  scale = 1.0 / mkt->p->base_increment;
  mkt->buy_size = trunc(cur * scale / mkt->buy_price) / scale;

  if(mkt->buy_price * mkt->buy_size < mkt->u.minfunds)
  {
    irc_broadcast("[%s:%s-%s] long aborted (minfunds: %s offered: %s)",
        mkt->exch->name, mkt->asset, mkt->currency,
        trim0(temp[0], 50, mkt->u.minfunds),
        trim0(temp[1], 50, mkt->buy_size * mkt->buy_price));
    market_sleep(mkt, 30);
    return;
  }

  if(mkt->buy_price * mkt->buy_size < mkt->p->min_market_funds)
  {
    logger(C_MKT, WARN, "market_buy",
        "%s:%s-%s order size error (min: %s offered: %s)",
        mkt->exch->name, mkt->asset, mkt->currency,
        trim0(temp[0], 50, mkt->p->min_market_funds),
        trim0(temp[1], 50, mkt->buy_size * mkt->buy_price));

    irc_broadcast("%s:%s-%s order size error (min: %s offered: %s)",
        mkt->exch->name, mkt->asset, mkt->currency,
        trim0(temp[0], 50, mkt->p->min_market_funds),
        trim0(temp[1], 50, mkt->buy_size * mkt->buy_price));

    market_ai(mkt, false);
    return;
  }

  if(mkt->log != NULL)
    fprintf(mkt->log,
        "\n#%d (( %sLONG%s OPEN )) sig: %s%s%s price: %s size: %s\n",
        mkt->cint[0].lc->num, PURPLE, ARESET, CYAN, mkt->a.signal, ARESET,
        trim0(temp[0], 50, mkt->buy_price),
        trim0(temp[1], 50, mkt->buy_size));

  irc_broadcast("(( LONG OPEN )) [%s:%s-%s] sig: %s price: %s size: %s",
      mkt->exch->name, mkt->asset, mkt->currency, mkt->a.signal,
      trim0(temp[0], 50, mkt->buy_price),
      trim0(temp[1], 50, mkt->buy_size));

  mkt->ht = lc->close; // trailing stop height
  mkt->state |= MKS_LONG;
  mkt->profit = mkt->profit_pct = mkt->profit_pct_high = mkt->profit_pct_low = 0;

  for(; x < CINTS_MAX; x++)
    mkt->cint[x].buy_num = mkt->cint[x].lc->num;

  market_order(mkt);
}

void
market_sell(struct market *mkt)
{
  struct candle *lc = mkt->cint[0].lc;
  double bal, cur, price, scale;
  char temp[2][50];
  int x = 0;

  if(mkt->state & MKS_ORDER)
  {
    logger(C_MKT, WARN, "market_sell",
        "[%s:%s-%s] attempt to sell while an order is pending",
        mkt->exch->name, mkt->asset, mkt->currency);
    return;
  }

  price = (mkt->a.price) ? mkt->a.price : lc->close;

  if(!(mkt->opt & MKO_PAPER))
  {
    struct account *a = account_find(mkt->exch, mkt->asset);

    if(a == NULL)
    {
      logger(C_MKT, WARN, "market_sell", "[%s:%s-%s] couldn't determine %s balance",
          mkt->exch->name, mkt->asset, mkt->currency, mkt->asset);
      return;
    }

    bal = a->available;
  }
  else
    bal = mkt->buy_size;

  // determine how much currency to use for this order
  // note: when shorting, currency == mkt->asset
  cur = (mkt->a.pct) ? bal * (mkt->a.pct / 100) : bal;

  scale = 1.0 / mkt->p->quote_increment;
  mkt->sell_price = trunc(scale * price) / scale;

  scale = 1.0 / mkt->p->base_increment;
  mkt->sell_size = trunc(cur * scale) / scale;

  if(mkt->sell_size * mkt->sell_price < mkt->p->min_market_funds)
  {

    logger(C_MKT, WARN, "market_sell",
        "%s:%s-%s order size error (min: %s offered: %s)",
        mkt->exch->name, mkt->asset, mkt->currency,
        trim0(temp[0], 50, mkt->p->min_market_funds),
        trim0(temp[1], 50, mkt->sell_size * mkt->sell_price));

    irc_broadcast("%s:%s-%s order size error (min: %s offered: %s)",
        mkt->exch->name, mkt->asset, mkt->currency,
        trim0(temp[0], 50, mkt->p->min_market_funds),
        trim0(temp[1], 50, mkt->sell_size * mkt->sell_price));

    market_ai(mkt, false);
    return;
  }

  mkt->state &= ~MKS_LONG & ~MKS_TRIMSTOP;

  if(mkt->log != NULL)
    fprintf(mkt->log,
        "\n#%d (( %sSHORT%s OPEN )) sig: %s%s%s price: %s size: %s\n",
        mkt->cint[0].lc->num, PURPLE, ARESET, CYAN, mkt->a.signal, ARESET,
        trim0(temp[0], 50, mkt->sell_price),
        trim0(temp[1], 50, mkt->sell_size));

  irc_broadcast("(( SHORT OPEN )) [%s:%s-%s] sig: %s price: %s size: %s",
      mkt->exch->name, mkt->asset, mkt->currency, mkt->a.signal,
      trim0(temp[0], 50, mkt->sell_price),
      trim0(temp[1], 50, mkt->sell_size));

  for(; x < CINTS_MAX; x++)
    mkt->cint[x].sell_num = mkt->cint[x].lc->num;

  market_order(mkt);

  if(mkt->opt & MKO_NOAIAFTER || mkt->opt & MKO_GETOUT)
    market_ai(mkt, false);
}

void
market_strategy(struct market *mkt)
{
  market_paper(mkt); // paper trader hook

  mkt->a.advice = ADVICE_NONE;
  mkt->a.price = mkt->a.pct = 0;
  mkt->a.signal = NULL;

  if(mkt->state & MKS_LONG && !(mkt->state & MKS_ORDER)
      && mkt->opt & MKO_GETOUT && mkt->profit_pct >= 0)
  {
    mkt->a.advice = ADVICE_SHORT;
    mkt->a.price = mkt->cint[0].lc->close;
    mkt->a.signal = "getout";
    market_sell(mkt);
  }

  // send candle to strategy
  if(mkt->strat != NULL)
    mkt->strat->update(mkt);

  if(mkt->sleepcount)
  {
    if(!--mkt->sleepcount)
      market_wake(mkt);
    return;
  }

  if(
      // if we are paper or live trading, do nothing until initial candles
      // have been downloaded
      !(mkt->state & MKS_READY)

      // user has disabled auto trading do nothing
      || !(mkt->opt & MKO_AI)
    )
    return; // do nothing

  if(mkt->cint[0].tc > HISTSIZE)
  {
    if(mkt->strat != NULL && mkt->a.advice == ADVICE_NONE && (mkt->opt & MKO_STRAT))
    {
      logger(C_MKT, DEBUG3, "market_strategy",
          "[%s] asking strategy %s for advice", mkt->name, mkt->strat->name);
      mkt->strat->advice(mkt);
    }
  }

  if(mkt->state & MKS_ORDER && mkt->o != NULL)
  {
    double filled = mkt->o->filled;
    char temp[100];

    if(mkt->u.orderwait && mkt->order_candles >= mkt->u.orderwait)
    {
      logger(C_MKT, WARN, "market_strategy",
          "%s:%s-%s timed out waiting for fill (filled: %s)",
          mkt->exch->name, mkt->asset, mkt->currency,
          trim0(temp, 100, filled));

      irc_broadcast("%s:%s-%s timed out waiting for fill (filled: %s)",
          mkt->exch->name, mkt->asset, mkt->currency,
          trim0(temp, 100, filled));

      if(mkt->log != NULL)
        fprintf(mkt->log, "[%s:%s-%s] timed out waiting for fill (filled: %s)\n",
          mkt->exch->name, mkt->asset, mkt->currency,
          trim0(temp, 100, filled));

      mkt->order_candles = 0;

      if(mkt->opt & MKO_PAPER)
        order_close(mkt->o);
      else
        exch_orderCancel(mkt->o);
      return;
    }

    if((mkt->o->state & ORS_PENDING) || (mkt->o->state & ORS_DELETING))
      return;

    if( ((mkt->state & MKS_LONG) && mkt->a.advice == ADVICE_SHORT) ||
        (!(mkt->state & MKS_LONG) && mkt->a.advice == ADVICE_LONG))
    {
      logger(C_MKT, WARN, "market_strategy",
            "[%s:%s-%s] canceling order (filled: %s)",
            mkt->exch->name, mkt->asset, mkt->currency,
            trim0(temp, 100, filled));

      irc_broadcast("[%s:%s-%s] canceling order (filled: %s)",
            mkt->exch->name, mkt->asset, mkt->currency, temp);

      if(mkt->opt & MKO_PAPER)
        order_close(mkt->o);
      else
        exch_orderCancel(mkt->o);
    }
  }

  else if(!(mkt->state & MKS_LONG) && mkt->a.advice == ADVICE_LONG)
    market_buy(mkt);
  else if((mkt->state & MKS_LONG) && mkt->a.advice == ADVICE_SHORT)
    market_sell(mkt);
}

void
market_close(struct market *mkt)
{
  int x = 0;

  logger(C_MKT, INFO, "market_close", "[%s:%s-%s] closing market",
      mkt->exch->name, mkt->asset, mkt->currency);

  for(;x < CINTS_MAX; x++)
  {
    if(mkt->cstore[x] != NULL)
      mem_free(mkt->cstore[x], mkt->cstore_cnt[x] * sizeof(struct candle));

    if(mkt->cint[x].c != NULL)
      mem_free(mkt->cint[x].c, sizeof(struct candle) * HISTSIZE);
  }

  if(mkt->log != NULL)
    fclose(mkt->log);

  if(mkt->strat != NULL)
    mkt->strat->close(mkt);

  if(mkt->exchdata != NULL)
    mem_free(mkt->exchdata, mkt->exchdata_size);

  if(mkt->candles != NULL)
    mem_free(mkt->candles, sizeof(struct candle) * mkt->init_candles);

  memset((void *)mkt, 0, sizeof(struct market));
}

// determine market trends
void
market_trend(struct candle_int *ci)
{
  struct candle *lc = ci->lc;

  // Fisher Transform
  if(lc->fish > lc->fish_sig)
  {
    if(ci->t_fish.dir != UP)
    {
      ci->t_fish.dir = UP;
      ci->t_fish.prev = ci->t_fish.count;
      ci->t_fish.count = 1;
    }
    else
      ci->t_fish.count++;
  }
  else
  {
    if(ci->t_fish.dir != DOWN)
    {
      ci->t_fish.dir = DOWN;
      ci->t_fish.prev = ci->t_fish.count;
      ci->t_fish.count = 1;
    }
    else
      ci->t_fish.count++;
  }
  memcpy((void *)&lc->t_fish, (void *)&ci->t_fish, sizeof(struct trend));

  // SMA 1 over 2
  if(lc->sma1_2_diff > 0)
  {
    if(ci->t_sma1_2.dir != UP)
    {
      ci->t_sma1_2.dir = UP;
      ci->t_sma1_2.prev = ci->t_sma1_2.count;
      ci->t_sma1_2.count = 1;
    }
    else
      ci->t_sma1_2.count++;
  }
  else
  {
    if(ci->t_sma1_2.dir != DOWN)
    {
      ci->t_sma1_2.dir = DOWN;
      ci->t_sma1_2.prev = ci->t_sma1_2.count;
      ci->t_sma1_2.count++;
    }
    else
      ci->t_sma1_2.count++;
  }
  memcpy((void *)&lc->t_sma1_2, (void *)&ci->t_sma1_2, sizeof(struct trend));

  // SMA 1 over 5
  if(lc->sma1_5_diff > 0)
  {
    if(ci->t_sma1_5.dir != UP)
    {
      ci->t_sma1_5.dir = UP;
      ci->t_sma1_5.prev = ci->t_sma1_5.count;
      ci->t_sma1_5.count = 1;
    }
    else
      ci->t_sma1_5.count++;
  }
  else
  {
    if(ci->t_sma1_5.dir != DOWN)
    {
      ci->t_sma1_5.dir = DOWN;
      ci->t_sma1_5.prev = ci->t_sma1_5.count;
      ci->t_sma1_5.count++;
    }
    else
      ci->t_sma1_5.count++;
  }
  memcpy((void *)&lc->t_sma1_5, (void *)&ci->t_sma1_5, sizeof(struct trend));
}

// receives candle for each interval and calculates various market
// indicators, trends, etc.
void
market_update(struct market *mkt, const int cinterval)
{
  struct candle_int *ci = &mkt->cint[cinterval];
  struct candle *lc = ci->lc, *pc;
  int x = 0;

  // if this interval doesn't have full history don't bother calculating
  // things because we won't have enough data to do so. So, the first real
  // candle the bot can make a decision on should be HISTSIZE, not #1
  if(ci->tc <= HISTSIZE)
    return;

  else if(ci->tc == HISTSIZE + 1)
  {
    logger(C_MKT, INFO, "market_update",
        "[%s] int%d (%d min) full history at candle #%d",
        mkt->name, cinterval, ci->length, mkt->num_candles);

    if(mkt->irc != -1)
      irc_reply_who(mkt->irc, mkt->reply,
          "[%s] int%d (%d min) full history at candle #%d",
          mkt->name, cinterval, ci->length, mkt->num_candles);
  }

  // previous candle
  pc = &ci->c[HISTSIZE - 2];

  lc->size = (lc->high - lc->low) / lc->low * 100;
  lc->asize = (lc->close - lc->open) / lc->open * 100;

  lc->last2 = (lc->close - pc->close) / pc->close * 100;
  lc->all = (lc->close - ci->c[0].close) / ci->c[0].close * 100;

  lc->last3 = (lc->close - ci->c[HISTSIZE - 3].close) / ci->c[HISTSIZE - 3].close * 100;
  lc->last5 = (lc->close - ci->c[HISTSIZE - 5].close) / ci->c[HISTSIZE - 5].close * 100;
  lc->last10 = (lc->close - ci->c[HISTSIZE - 10].close) / ci->c[HISTSIZE - 10].close * 100;
  lc->last25 = (lc->close - ci->c[HISTSIZE - 25].close) / ci->c[HISTSIZE - 25].close * 100;
  lc->last50 = (lc->close - ci->c[HISTSIZE - 50].close) / ci->c[HISTSIZE - 50].close * 100;
  lc->last100 = (lc->close - ci->c[HISTSIZE - 100].close) / ci->c[HISTSIZE - 100].close * 100;
  lc->last150 = (lc->close - ci->c[HISTSIZE - 150].close) / ci->c[HISTSIZE - 150].close * 100;
  
  // history totals recalc'd every candle
  ci->high = ci->low = ci->volume = 0;

  // reset moving avg's
  lc->sma1 = lc->sma2 = lc->sma3 = lc->sma4 = lc->sma5 = 0;
  lc->vwma1 = lc->vwma2 = lc->vwma3 = lc->vwma4 = lc->vwma5 = 0;
  lc->vsma1 = lc->vsma2 = lc->vsma3 = lc->vsma4 = lc->vsma5 = 0;
  lc->vg1 = lc->vg2 = lc->vg3 = lc->vg4 = lc->vg5 = 0;
  lc->rsi_sma = 0;

  // loop through candle history in order of oldest to latest
  for(; x < HISTSIZE; x++)
  {
    // Populate tulip arrays
    tulip_inputs[0][x] = ci->c[x].open;
    tulip_inputs[1][x] = ci->c[x].close;
    tulip_inputs[2][x] = ci->c[x].high;
    tulip_inputs[3][x] = ci->c[x].low;
    tulip_inputs[4][x] = ci->c[x].volume;

    ci->volume += ci->c[x].volume;

    if(ci->c[x].low < ci->low || ci->low == 0)
    {
      ci->low = ci->c[x].low;
      ci->t_low = ci->c[x].time;
      ci->low_num = ci->c[x].num;
    }

    if(ci->high < ci->c[x].high)
    {
      ci->high = ci->c[x].high;
      ci->t_high = ci->c[x].time;
      ci->high_num = ci->c[x].num;
    }

    if(x >= HISTSIZE - mkt->u.rsi_interval)
      lc->rsi_sma += ci->c[x].rsi;

    // calculate moving averages for price & vol
    if(x >= HISTSIZE - mkt->u.sma5)
    {
      lc->sma5 += ci->c[x].close;
      lc->vsma5 += ci->c[x].volume;
      lc->vwma5 += ci->c[x].close * ci->c[x].volume;

      if(ci->c[x].close > ci->c[x].open)
        lc->vg5 += ci->c[x].volume;

      if(x >= HISTSIZE - mkt->u.sma4)
      {
        lc->sma4 += ci->c[x].close;
        lc->vsma4 += ci->c[x].volume;
        lc->vwma4 += ci->c[x].close * ci->c[x].volume;

        if(ci->c[x].close > ci->c[x].open)
          lc->vg4 += ci->c[x].volume;

        if(x >= HISTSIZE - mkt->u.sma3)
        {
          lc->sma3 += ci->c[x].close;
          lc->vsma3 += ci->c[x].volume;
          lc->vwma3 += ci->c[x].close * ci->c[x].volume;

          if(ci->c[x].close > ci->c[x].open)
            lc->vg3 += ci->c[x].volume;

          if(x >= HISTSIZE - mkt->u.sma2)
          {
            lc->sma2 += ci->c[x].close;
            lc->vsma2 += ci->c[x].volume;
            lc->vwma2 += ci->c[x].close * ci->c[x].volume;

            if(ci->c[x].close > ci->c[x].open)
              lc->vg2 += ci->c[x].volume;

            if(x >= HISTSIZE - mkt->u.sma1)
            {
              lc->sma1 += ci->c[x].close;
              lc->vsma1 += ci->c[x].volume;
              lc->vwma1 += ci->c[x].close * ci->c[x].volume;

              if(ci->c[x].close > ci->c[x].open)
                lc->vg1 += ci->c[x].volume;
            }
          }
        }
      }
    }
  }

  // distance from high / low's
  lc->low_age = lc->num - ci->low_num;
  lc->high_age = lc->num - ci->high_num;

  lc->ci_low = ci->low;
  lc->ci_high = ci->high;

  // TULIP: ADX (average directional momentum)
  lc->adx = tulip_adx(HISTSIZE, mkt->u.adx_interval, tulip_inputs[2],
      tulip_inputs[3], tulip_inputs[1], tulip_outputs[0]);

  // TULIP: Fisher Transform
  tulip_fisher(HISTSIZE, mkt->u.fish_interval, tulip_inputs[2],
      tulip_inputs[3], tulip_outputs[0], tulip_outputs[1]);

  lc->fish = tulip_outputs[0][HISTSIZE - mkt->u.fish_interval];
  lc->fish_sig = tulip_outputs[1][HISTSIZE - mkt->u.fish_interval];

  // TULIP: MACD (Moving Average Conversion Diversion)
  tulip_macd(HISTSIZE, mkt->u.macd_fast, mkt->u.macd_slow,
      mkt->u.macd_signal, tulip_inputs[1], tulip_outputs[0],
      tulip_outputs[1], tulip_outputs[2]);
  lc->macd = tulip_outputs[0][HISTSIZE - mkt->u.macd_slow];
  lc->macd_s = tulip_outputs[1][HISTSIZE - mkt->u.macd_slow];
  lc->macd_h = tulip_outputs[2][HISTSIZE - mkt->u.macd_slow];

  // TULIP: RSI (Rising Strenght Index)
  lc->rsi = tulip_rsi(HISTSIZE, mkt->u.rsi_interval, tulip_inputs[1], tulip_outputs[0]);
  lc->rsi_sma /= mkt->u.rsi_interval;

  /*
  //use this snippit to validate tulip's returns
  if(cc->num == 400 && cinterval == 4)
  {
  int zz = 0;

  for(;zz < HISTSIZE; zz++)
  {
  printf("%d: %.11f %.11f\n", zz, tulip_outputs[0][zz], tulip_outputs[1][zz]);
  }
  }
  */

  // VWMA (volume weighted moving average)
  // 3-Day VWMA = (C1*V1 + C2*V2 + C3*V3) / (V1+ V2+ V3)
  lc->vwma1 /= lc->vsma1;
  lc->vwma2 /= lc->vsma2;
  lc->vwma3 /= lc->vsma3;
  lc->vwma4 /= lc->vsma4;
  lc->vwma5 /= lc->vsma5;

  // Simple Moving Average (SMA) for price
  lc->sma1 /= mkt->u.sma1;
  lc->sma1 = (lc->sma1) ? lc->sma1 : lc->close;
  lc->sma2 /= mkt->u.sma2;
  lc->sma2 = (lc->sma2) ? lc->sma2 : lc->close;
  lc->sma3 /= mkt->u.sma3;
  lc->sma3 = (lc->sma3) ? lc->sma3 : lc->close;
  lc->sma4 /= mkt->u.sma4;
  lc->sma4 = (lc->sma4) ? lc->sma4 : lc->close;
  lc->sma5 /= mkt->u.sma5;
  lc->sma5 = (lc->sma5) ? lc->sma5 : lc->close;

  // calculate the difference in price with each sma
  lc->sma1_diff = (lc->close - lc->sma1) / lc->sma1 * 100;
  lc->sma2_diff = (lc->close - lc->sma2) / lc->sma2 * 100;
  lc->sma3_diff = (lc->close - lc->sma3) / lc->sma3 * 100;
  lc->sma4_diff = (lc->close - lc->sma4) / lc->sma4 * 100;
  lc->sma5_diff = (lc->close - lc->sma5) / lc->sma5 * 100;

  // calculate sma's vs other sma's
  lc->sma1_2_diff = (lc->sma1 - lc->sma2) / lc->sma2 * 100;
  lc->sma1_3_diff = (lc->sma1 - lc->sma3) / lc->sma3 * 100;
  lc->sma1_4_diff = (lc->sma1 - lc->sma4) / lc->sma4 * 100;
  lc->sma1_5_diff = (lc->sma1 - lc->sma5) / lc->sma5 * 100;
  lc->sma2_3_diff = (lc->sma2 - lc->sma3) / lc->sma3 * 100;
  lc->sma2_4_diff = (lc->sma2 - lc->sma4) / lc->sma4 * 100;
  lc->sma2_5_diff = (lc->sma2 - lc->sma5) / lc->sma5 * 100;
  lc->sma3_4_diff = (lc->sma3 - lc->sma4) / lc->sma4 * 100;
  lc->sma3_5_diff = (lc->sma3 - lc->sma5) / lc->sma5 * 100;
  lc->sma4_5_diff = (lc->sma4 - lc->sma5) / lc->sma5 * 100;

  // compare sma with vwma
  lc->vwma1_sma1_diff = (lc->sma1 - lc->vwma1) / lc->vwma1 * 100;
  lc->vwma2_sma2_diff = (lc->sma2 - lc->vwma2) / lc->vwma2 * 100;
  lc->vwma3_sma3_diff = (lc->sma3 - lc->vwma3) / lc->vwma3 * 100;
  lc->vwma4_sma4_diff = (lc->sma4 - lc->vwma4) / lc->vwma4 * 100;
  lc->vwma5_sma5_diff = (lc->sma5 - lc->vwma5) / lc->vwma5 * 100;

  lc->sma1_angle = (lc->sma1 - ci->c[HISTSIZE - 3].sma1) / ci->c[HISTSIZE - 3].sma1 * 100;
  lc->sma2_angle = (lc->sma2 - ci->c[HISTSIZE - 3].sma2) / ci->c[HISTSIZE - 3].sma2 * 100;
  lc->sma3_angle = (lc->sma3 - ci->c[HISTSIZE - 3].sma3) / ci->c[HISTSIZE - 3].sma3 * 100;
  lc->sma4_angle = (lc->sma4 - ci->c[HISTSIZE - 3].sma4) / ci->c[HISTSIZE - 3].sma4 * 100;
  lc->sma5_angle = (lc->sma5 - ci->c[HISTSIZE - 3].sma5) / ci->c[HISTSIZE - 3].sma5 * 100;

  // green volume dominace for each moving average period
  lc->vg1 = (lc->vg1) ? lc->vg1 / lc->vsma1 * 100 : 0;
  lc->vg2 = (lc->vg2) ? lc->vg2 / lc->vsma2 * 100 : 0;
  lc->vg3 = (lc->vg3) ? lc->vg3 / lc->vsma3 * 100 : 0;
  lc->vg4 = (lc->vg4) ? lc->vg4 / lc->vsma4 * 100 : 0;
  lc->vg5 = (lc->vg5) ? lc->vg5 / lc->vsma5 * 100 : 0;

  // Simple Moving Average (SMA) for volume
  lc->vsma1 /= mkt->u.sma1;
  lc->vsma1 = (lc->vsma1) ? lc->vsma1 : lc->volume;
  lc->vsma2 /= mkt->u.sma2;
  lc->vsma2 = (lc->vsma2) ? lc->vsma2 : lc->volume;
  lc->vsma3 /= mkt->u.sma3;
  lc->vsma3 = (lc->vsma3) ? lc->vsma3 : lc->volume;
  lc->vsma4 /= mkt->u.sma4;
  lc->vsma4 = (lc->vsma4) ? lc->vsma4 : lc->volume;
  lc->vsma5 /= mkt->u.sma5;
  lc->vsma5 = (lc->vsma5) ? lc->vsma5 : lc->volume;

  // compare this candle's volume with moving average volumes
  lc->vsma1_diff = (lc->volume - lc->vsma1) / lc->vsma1 * 100;
  lc->vsma2_diff = (lc->volume - lc->vsma2) / lc->vsma2 * 100;
  lc->vsma3_diff = (lc->volume - lc->vsma3) / lc->vsma3 * 100;
  lc->vsma4_diff = (lc->volume - lc->vsma4) / lc->vsma4 * 100;
  lc->vsma5_diff = (lc->volume - lc->vsma5) / lc->vsma5 * 100;

  // calculate sma's vs other sma's
  lc->vsma1_2_diff = (lc->vsma1 - lc->vsma2) / lc->vsma2 * 100;
  lc->vsma1_3_diff = (lc->vsma1 - lc->vsma3) / lc->vsma3 * 100;
  lc->vsma1_4_diff = (lc->vsma1 - lc->vsma4) / lc->vsma4 * 100;
  lc->vsma1_5_diff = (lc->vsma1 - lc->vsma5) / lc->vsma5 * 100;
  lc->vsma2_3_diff = (lc->vsma2 - lc->vsma3) / lc->vsma3 * 100;
  lc->vsma2_4_diff = (lc->vsma2 - lc->vsma4) / lc->vsma4 * 100;
  lc->vsma2_5_diff = (lc->vsma2 - lc->vsma5) / lc->vsma5 * 100;
  lc->vsma3_4_diff = (lc->vsma3 - lc->vsma4) / lc->vsma4 * 100;
  lc->vsma3_5_diff = (lc->vsma3 - lc->vsma5) / lc->vsma5 * 100;
  lc->vsma4_5_diff = (lc->vsma4 - lc->vsma5) / lc->vsma5 * 100;

  // increment order_candles if waiting on fill
  if(mkt->state & MKS_ORDER)
    mkt->order_candles++;

  // 1-minute candles
  if(!cinterval) 
  {

    // exposed
    if( (mkt->state & MKS_LONG)
        && (((mkt->state & MKS_ORDER) && mkt->o != NULL && mkt->o->filled)
          || !(mkt->state & MKS_ORDER)))
    {
      double sell_funds = mkt->buy_size * lc->close;
      double sell_fee = sell_funds * mkt->exch->fee_maker;

      sell_funds -= sell_fee;
      mkt->profit = lc->profit = sell_funds - mkt->buy_funds;
      mkt->profit_pct = lc->profit_pct = mkt->profit / mkt->buy_funds * 100;

      mkt->profit_pct_high = lc->profit_pct_high =
        (mkt->profit_pct > mkt->profit_pct_high || mkt->profit_pct_high == 0)
        ? mkt->profit_pct : mkt->profit_pct_high;

      mkt->profit_pct_low = lc->profit_pct_low =
        (mkt->profit_pct < mkt->profit_pct_low || mkt->profit_pct_low == 0)
        ? mkt->profit_pct : mkt->profit_pct_low;

      mkt->ht = (lc->close > mkt->ht) ? lc->close : mkt->ht;
      mkt->exposed++;
    }

    // minprofit
    mkt->minprofit = lc->close * (1 + mkt->exch->fee_maker)
      * (1 + mkt->exch->fee_maker) * 1.001;
  }

  // price vs interval high / low
  lc->high_diff = (lc->close - ci->high) / ci->high * 100;
  lc->low_diff = (lc->close - ci->low) / ci->low * 100;

  // diff between interval high / low
  lc->high_low_diff = (ci->high - ci->low) / ci->low * 100;

  // price vs mean of interval high / low
  lc->hlm_diff = (lc->close - ((ci->high + ci->low) / 2)) / ((ci->high + ci->low) / 2) * 100;

  // percentages above high/low
  lc->pct_below_high = (ci->high - lc->close) / (ci->high - ci->low) * 100;
  lc->pct_above_low = (lc->close - ci->low) / (ci->high - ci->low) * 100;

  // track various trends per ci
  market_trend(ci);

  // tag current candle with interval score
  lc->score = market_score(mkt, cinterval);

}

// exchange APIs and backtest deliver candles to this function in order of
// oldest to newest - and always 1-minute candles. This function will 
// calculate other candle intervals (5-min, 15-min, etc)
void
market_candle(struct market *mkt, const struct candle *c)
{
  int x = 0;

  if(!c->close || !c->open)
  {
    logger(C_MKT, CRIT, "market_candle", "candle open and/or close is 0 - bug");
    market_close(mkt);
    return;
  }

  mkt->num_candles++;

  // advance the market's current candle time to next (newer) interval
  mkt->cc_time += mkt->gran;

  for(; x < CINTS_MAX; x++)
  {
    struct candle_int *ci = &mkt->cint[x];
    struct candle *nc = &ci->nc;

    if(ci->ic++ == 0) // this is the first candle in this interval
    {
      // reset next candle store
      memset((void *)nc, 0, sizeof(struct candle));

      nc->open   = c->open;
      nc->high   = c->high;
      nc->low    = c->low;
      nc->volume = c->volume;
      nc->vwp    = c->vwp;
      nc->num    = mkt->num_candles;
      nc->time   = c->time;
    }

    else // not first candle, merge
    {
      if(c->high > nc->high)
        nc->high = c->high;
      if(c->low < nc->low)
        nc->low = c->low;
      nc->num    = mkt->num_candles;
      nc->volume += c->volume;
      nc->vwp    += c->vwp;
    }

    if(ci->ic == ci->length) // candle is full, send it
    {
      nc->close = c->close;
      nc->num = ++ci->tc;

      if(nc->vwp)
        nc->vwp /= nc->volume;

      ci->ic = 0;      // reset interval counter

      // shift entire array by one candle. this does two things:
      // 1) makes room for the new candle to be inserted (at the end)
      // 2) chucks the oldest candle
      // Previously I had simply used an integer to keep track of where
      // the newest candle was inserted but this made strategies
      // cumbersome and backtesting slower. Ideally, rewrite all of this
      // with a linked list strategy. TODO
      memcpy((void *)ci->c, (void *)ci->c + sizeof(struct candle),
          sizeof(struct candle) * (HISTSIZE - 1));

      // set oldest / newest candle pointers
      // TODO: move these to market_add they only need set once
      ci->lc = &ci->c[HISTSIZE - 1];
      ci->lc_slot = HISTSIZE - 1;

      // incoming candle (nc) becomes latest candle (lc)
      memcpy((void *)ci->lc, nc, sizeof(struct candle));

      market_update(mkt, x);  // indicator math and strategy hooks

      // in candle maker mode -- store all candles in a contiguous memory
      // space so that they can be written in reverse order (oldest to newest)
      // to data files for backtesting
      if(global.state & WMS_CM)
        memcpy((void *)&mkt->cstore[x][mkt->cstore_cnt[x]++], ci->lc, sizeof(struct candle));

      else if(mkt->log != NULL && ci->tc >= HISTSIZE)
        candle_log(mkt, x);
    }
  }

  if(mkt->strat != NULL)
    market_strategy(mkt); // send candle to strategy

  // after all processing has been done on current candle
  // it becomes previous candle
  if(mkt->pc != NULL)
    memcpy((void *)mkt->pc, (void *)c, sizeof(struct candle));
}

// TODO: get this stuff from user config or db
void
market_conf(struct market *mkt)
{
  char *temp;
  int x = 0;

  if((temp = getenv("STARTCUR")) != NULL)
    mkt->stats.p_start_cur = strtod(temp, NULL);
  else
    mkt->stats.p_start_cur = 1000;

  for(; x < CINTS_MAX; x++)
    mkt->cint[x].length = cint_length(x);

  mkt->u.orderwait = 720;
  mkt->u.minfunds = 5;

  // SMA lengths
  // Note that these are used for various other things too
  mkt->u.sma1 = ((temp = getenv("SMA1")) == NULL) ? 7 : atoi(temp);
  mkt->u.sma2 = ((temp = getenv("SMA2")) == NULL) ? 25 : atoi(temp);
  mkt->u.sma3 = ((temp = getenv("SMA3")) == NULL) ? 50 : atoi(temp);
  mkt->u.sma4 = ((temp = getenv("SMA4")) == NULL) ? 100 : atoi(temp);
  mkt->u.sma5 = ((temp = getenv("SMA5")) == NULL) ? 200 : atoi(temp);

  // Fisher Transform
  mkt->u.fish_interval = ((temp = getenv("FISHINTERVAL")) == NULL) ? 9 : atoi(temp);

  // Relative strength index
  mkt->u.rsi_interval = ((temp = getenv("RSIINTERVAL")) == NULL) ? 14 : atoi(temp);
  mkt->u.rsi_oversold = ((temp = getenv("RISOVERSOLD")) == NULL) ? 70 : atoi(temp);
  mkt->u.rsi_underbought = ((temp = getenv("RSIUNDERBOUGHT")) == NULL) ? 30 : atoi(temp);

  // Moving average conversion diversion
  mkt->u.macd_fast = ((temp = getenv("MACDFAST")) == NULL) ? 12 : atoi(temp);
  mkt->u.macd_slow = ((temp = getenv("MACDSLOW")) == NULL) ? 26 : atoi(temp);
  mkt->u.macd_signal = ((temp = getenv("MACDSIGNAL")) == NULL) ? 9 : atoi(temp);

  // Average directioanl index (trend strength)
  mkt->u.adx_interval = ((temp = getenv("ADXINTERVAL")) == NULL) ? 14 : atoi(temp);

  // Volume weighted moving average
  mkt->u.vwma_interval = ((temp = getenv("VWMAINTERVAL")) == NULL) ? 14 : atoi(temp);

  if(getenv("MARKETLOG") != NULL)
    mkt->opt |= MKO_LOG;

  if(getenv("MARKETPLOT") != NULL)
    mkt->opt |= MKO_PLOT;
}

struct market *
market_find(const struct exchange *e, const char *a, const char *c)
{
  int x = 0;
  for(; markets[x].state & MKS_ACTIVE; x++)
  {
    struct market *m = &markets[x];

    if(m->exch == e && !strcasecmp(m->asset, a) && !strcasecmp(m->currency, c))
      return(m);
  }
  return(NULL);
}

// TOOD: break this into multiple functions sheesh
int
market_open(struct exchange *exch, const int req_candles, const char *asset, const char *currency, FILE *fp, struct strategy *strat, const double fee, const double start_currency, time_t start)
{
  char start_time[100], end_time[100];
  struct market *mkt;
  char path[300];
  int mslot = 0, x = 0;

  for(; mslot < MARKET_MAX && markets[mslot].asset[0] != '\0'; mslot++);
  if(mslot == MARKET_MAX)
  {
    logger(C_MKT, WARN, "market_open",
        "request to add market %s:%s-%s failed (max markets reached %d)",
        exch->name, asset, currency, MARKET_MAX);
    return(-1);
  }

  // zero out our memory for newly added market
  mkt = &markets[mslot];
  memset((void *)mkt, 0, sizeof(struct market));

  if((mkt->p = exch_findProduct(exch, asset, currency)) == NULL)
  {
    logger(C_MKT, WARN, "market_open",
        "request to add unknown market %s:%s0%s", exch->name, asset, currency);
    return(-1);
  }

  // setup market defaults
  market_conf(mkt); // TODO: pull these from db
  mkt->irc = global.is;

  if(global.is != -1)
    strncpy(mkt->reply, irc[global.is].reply, 19);

  mkt->gran = 60;
  mkt->exch = exch;

  // initial market state
  mkt->state |= MKS_ACTIVE | MKS_LEARNING | MKS_NEWEST | MKS_MOREDATA;

  // initial market options, paper mode and strategy enabled
  mkt->opt |= MKO_PAPER | MKO_STRAT;

  strncpy(mkt->asset, asset, 19);
  mkupper(mkt->asset);
  strncpy(mkt->currency, currency, 19);
  mkupper(mkt->currency);

  snprintf(mkt->name, 79, "%02d %s:%s-%s", mslot, mkt->exch->name,
      mkt->asset, mkt->currency);

  if(!(global.state & WMS_CM))
  {
    struct account *a = account_find(mkt->exch, mkt->currency);

    if(a == NULL)
    {
      logger(C_MKT, CRIT, "market_open",
          "couldn't find account balance for %s:%s",
          mkt->exch->name, mkt->currency);
      market_close(mkt);
      return(-1);
    }

    mkt->stats.start_cur = a->available;

    // TODO: make user setting
    mkt->stats.p_start_cur = start_currency;

    logger(C_MKT, INFO, "market_open", "%s paper trader start balance: %f",
        mkt->name, mkt->stats.p_start_cur);
  }

  // markets begin in learning mode: inital candles come in newest to oldest
  // start one interval behind as the current interval has not yet closed
  mkt->start = candle_time(global.now - ((req_candles + 1) * 60), 60, true);
  mkt->cc_time = candle_time(global.now - 60, 60, false);
  mkt->init_candles = (mkt->cc_time - mkt->start) / 60;

  time2str(start_time, mkt->start, false);
  time2str(end_time, mkt->cc_time, false);
  logger(C_MKT, INFO, "market_open", "first: %s last: %s total: %d candles",
      start_time, end_time, mkt->init_candles);

  // allocate memory for learning mode candles
  mkt->candles = (struct candle *)mem_alloc(sizeof(struct candle) * mkt->init_candles);
  memset((void *)mkt->candles, 0, sizeof(struct candle) * mkt->init_candles);

  // allocate HISTSIZE candle arrays for each candle interval
  for(x = 0; x < CINTS_MAX; x++)
  {
    mkt->cint[x].c = (struct candle *)mem_alloc(sizeof(struct candle) * HISTSIZE);
    memset((void *)mkt->cint[x].c, 0, sizeof(struct candle) * HISTSIZE);
  }

  mkt->cc = mkt->candles;
  mkt->cc->time = mkt->cc_time;

  logger(C_MKT, INFO, "market_open", "%s market initialized", mkt->name);

  if(!(global.state & WMS_CM))
  {
    mkt->strat = strat;

    if(mkt->opt & MKO_LOG)
      market_mklog(mkt);

    if(strat != NULL && strat->init(mkt) == FAIL)
    {
      market_close(mkt);
      return(-1);
    }
  }

  // request data
  if(exch_getCandles(mkt) == FAIL)
  {
    logger(C_MKT, WARN, "market_open", "unable to add market %s vs %s @ %s", asset, currency, exch->name);
    market_close(mkt);
    return(-1);
  }

  return(mslot);
}

void
market_cleanup(void)
{
  int x = 0;

  for(; x < MARKET_MAX; x++)
  {
    struct market *mkt = &markets[x];
    if(mkt->state & MKS_ACTIVE)
      market_close(mkt);
  }
}

void
market_init(void)
{
  memset((void *)markets, 0, sizeof(struct market) * MARKET_MAX);
  logger(C_MKT, INFO, "market_init", "market manager initialized (%d max)", MARKET_MAX);
}

void
market_hook(void)
{
  int x = 0;

  for(; x < MARKET_MAX; x++)
  {
    struct market *mkt = &markets[x];

    if(mkt->state & MKS_LEARNING || !(mkt->state & MKS_ACTIVE))
      continue;

    if(!(mkt->state & MKS_LEARNING) && !(mkt->state & MKS_POLLED))
    {
      // if cc_time is in the past, poll
      if(global.now > mkt->cc_time + mkt->gran)
      {
        char str[50], str2[50];

        time2str(str, mkt->cc_time, false);
        time2str(str2, global.now, false);

        logger(C_MKT, DEBUG3, "market_hook", "[%s:%s-%s] candle time: %s now: %s", mkt->exch->name, mkt->asset, mkt->currency, str, str2);

        if(exch_getCandles(&markets[x]) == FAIL)
          market_close(mkt);
      }
    }
  }
}
