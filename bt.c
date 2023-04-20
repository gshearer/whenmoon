#define WM_BTC
#include "bt.h"

// sort by profit, wla, trades
int
bt_sort_profit(const void *c1, const void *c2)
{
  struct bt_case **bc1 = (struct bt_case **)c1, **bc2 = (struct bt_case **)c2;

  if((*bc1)->results.profit == (*bc2)->results.profit)
  {
    if((*bc1)->results.trades == (*bc2)->results.trades)
    {
      if((*bc1)->results.wla == (*bc2)->results.wla)
        return(0);
      if((*bc1)->results.wla < (*bc2)->results.wla)
        return(1);
      return(-1);
    }
    if((*bc1)->results.trades < (*bc2)->results.trades)
      return(1);
    return(-1);
  }
  if((*bc1)->results.profit < (*bc2)->results.profit)
    return(1);
  return(-1);
}

// sort by wla, trades, profit
int
bt_sort_wla(const void *c1, const void *c2)
{
  struct bt_case **bc1 = (struct bt_case **)c1, **bc2 = (struct bt_case **)c2;

  if((*bc1)->results.wla == (*bc2)->results.wla)
  {
    if((*bc1)->results.trades == (*bc2)->results.trades)
    {
      if((*bc1)->results.profit == (*bc2)->results.profit)
        return(0);
      if((*bc1)->results.profit < (*bc2)->results.profit)
        return(1);
      return(-1);
    }
    if((*bc1)->results.trades < (*bc2)->results.trades)
      return(1);
    return(-1);
  }
  if((*bc1)->results.wla < (*bc2)->results.wla)
    return(1);
  return(-1);
}

// sort by trades, wla, profit
int
bt_sort_trades(const void *c1, const void *c2)
{
  struct bt_case **bc1 = (struct bt_case **)c1, **bc2 = (struct bt_case **)c2;

  double x1 = (*bc1)->results.trades / (*bc1)->results.wla,
         x2 = (*bc2)->results.trades / (*bc2)->results.wla;

  //if((*bc1)->results.trades == (*bc2)->results.trades)
  if(x1 == x2)
  {
    if((*bc1)->results.wla == (*bc2)->results.wla)
    {
      if((*bc1)->results.profit == (*bc2)->results.profit)
        return(0);
      if((*bc1)->results.profit < (*bc2)->results.profit)
        return(1);
      return(-1);
    }
    if((*bc1)->results.wla < (*bc2)->results.wla)
      return(1);
    return(-1);
  }
  //if((*bc1)->results.trades < (*bc2)->results.trades)
  if(x1 < x2)
    return(1);
  return(-1);
}

void
bt_endthread(struct bt_thread *me)
{
  pthread_mutex_lock(&me->mutex);
  me->state = BTT_DIE;
  pthread_mutex_unlock(&me->mutex);
  pthread_exit(NULL);
}

// create html index of market test results
void
bt_mkindex(const struct bt_dataset *ds, const struct bt_case *bc, struct market *mkt)
{   
  char p[300], start_str[50];
  int x = 0;

  snprintf(p, 299, "%s/%s-%s-%s", global.conf.logpath, mkt->exch->name,
      mkt->asset, mkt->currency);

  mkdir(p, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);

  snprintf(p, 299, "%s/%s-%s-%s/plots", global.conf.logpath, mkt->exch->name,
      mkt->asset, mkt->currency);

  mkdir(p, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);

  snprintf(p, 299, "%s/%s-%s-%s/index.html", global.conf.logpath,
      mkt->exch->name, mkt->asset, mkt->currency);

  if((fp_index = fopen(p, "w")) ==  NULL)
  {
    logger(C_BT, WARN, "bt_mkindex",
        "error creating html index '%s': %s (disabling plots)",
        p, strerror(errno));
    mkt->opt &= ~MKO_PLOT;
    return;
  }

  time2strf(start_str, "%F %T",  mkt->start, false);

  fprintf(fp_index,
      "<html>\n"
      "  <head>\n"
      "    <title>%s:%s-%s start @ %s</title>\n"
      "    <style>\n"
      "      body { background-color: black; }\n"
      "      table { border: 5px solid black; }\n"
      "      th, td { border: 1px solid black; padding: 15px; }\n"
      "    </style>\n"
      "  </head>\n"
      "  <body>\n"
      "    <center>\n"
      "    <img src=\"https://shearer.tech/images/whenmoon.jpg\">\n"
      "    <h1>%s:%s-%s backtest start @ %s</h1>\n"
      "    <br>\n"
      "    <table>\n"
      "      <tr>\n"
      "        <td>strategy</td>\n"
      "        <td>description</td>\n"
      "        <td>uargs</td>\n"
      "      </tr>\n"
      "      <tr>\n"
      "        <td>%s</td>\n"
      "        <td>%s</td>\n"
      "        <td>",
      mkt->exch->name, mkt->asset, mkt->currency, start_str,
      mkt->exch->name, mkt->asset, mkt->currency, start_str,
      mkt->strat->name, mkt->strat->desc);

  for(; x < bc->num_uargs; x++)
    fprintf(fp_index, " %f", bc->uargs[x]);

  fprintf(fp_index,
      "</td>\n"
      "      </tr>\n"
      "    </table>\n"
      "    <br>\n"
      "    <table>\n"
      "      <tr>\n"
      "        <td style=\"text-align: center;\" colspan=\"8\">per-trade results</td>\n"
      "        <td style=\"text-align: center;\" colspan=\"3\">user vars</td>\n"
      "        <td style=\"text-align: center;\" colspan=\"3\">running totals</td>\n"
      "      </tr>\n"
      "      <tr>\n"
      "        <td>trade num</td>\n"
      "        <td>time</td>\n"
      "        <td>signal</td>\n"
      "        <td>profit</td>\n"
      "        <td>pct</td>\n"
      "        <td>high</td>\n"
      "        <td>low</td>\n"
      "        <td>exposed</td>\n"
      "        <td>u1</td>\n"
      "        <td>u2</td>\n"
      "        <td>u3</td>n"
      "        <td>profit</td>\n"
      "        <td>fees</td>\n"
      "        <td>balance</td>\n"
      "       </tr>\n");

  snprintf(p, 299, "%s/%s-%s-%s/wl_diff.html", global.conf.logpath,
      mkt->exch->name, mkt->asset, mkt->currency);

  if((fp_wldiff = fopen(p, "w")) ==  NULL)
  {
    logger(C_BT, WARN, "bt_mkindex",
        "error creating win/loss diff '%s': %s (disabling plots)",
        p, strerror(errno));
    mkt->opt &= ~MKO_PLOT;
    fclose(fp_index);
    fp_index = NULL;
    return;
  }

  fprintf(fp_wldiff,
      "<html>\n"
      "  <head>\n"
      "    <title>%s:%s-%s start @ %s</title>\n"
      "    <style>\n"
      "      body { background-color: black; }\n"
      "      table { border: 5px solid black; }\n"
      "      th, td { border: 1px solid black; padding: 15px; }\n"
      "    </style>\n"
      "  </head>\n"
      "  <body>\n"
      "    <center>\n"
      "    <img src=\"https://shearer.tech/images/whenmoon.jpg\">\n"
      "    <h1>%s:%s-%s backtest start @ %s</h1>\n"
      "    <br>\n"
      "    <table>\n"
      "      <tr>\n"
      "        <td>winning longs</td>\n"
      "        <td>losing longs</td>\n"
      "      </tr>\n",
      mkt->exch->name, mkt->asset, mkt->currency, start_str,
      mkt->exch->name, mkt->asset, mkt->currency, start_str);
}

// creates .gnuplot file which can be used to create candlestick chart .jpg
int
bt_mkplot(const struct bt_plot *p)
{
  const struct market *mkt = p->mkt;
  const struct candle_int *ci = &mkt->cint[p->cint];
  FILE *fp = fopen(p->path, "w");
  char temp[50];
  int x = 0, y = p->idx;

  if(fp == NULL)
    return(FAIL);

  fprintf(fp,
      "set datafile separator ','\n"
      "set terminal pngcairo size 1280,1024 background rgb '#000000' enhanced font 'Times-Roman,14' fontscale 1.\n"
      "set title '[%s] (%s) %s vs %s Trade #%d @ %f' textcolor rgb '#%s' font 'Times-Roman,20'\n"
      "set ylabel 'Price' textcolor rgb '#ffffff'\n"
      "set y2label 'Price' textcolor rgb '#ffffff'\n"
      "set xlabel 'Price' textcolor rgb '#ffffff'\n"
      "set x2label 'Price' textcolor rgb '#ffffff'\n"
      "set xdata time\n"
      "set x2data time\n"
      "set timefmt '%%Y-%%m-%%d %%H:%%M:%%S'\n"
      "set border linecolor rgb '#ffffff' linewidth 2\n"
      "set grid linecolor rgb '#ffffff' linewidth 2\n"
      "set key top left textcolor rgb '#ffffff' box linetype 1 linewidth 1\n"
      "set style fill solid noborder\n"
      "set palette defined (-1 '#990000', 1 '#009900')\n"
      "set cbrange [-1:1]\n"
      "set x2tics\n"
      "set y2tics\n"
      "set link x2\n"
      "set link y2\n"
      "unset colorbox\n"
      "set style line 1 linetype 1 linewidth 1 pointtype 0\n"
      "set style line 2 linetype 4 linewidth 30 pointtype 7\n"
      "$whenmoon << EOD\n",
    cint2str(cint_length(p->cint)), (p->islong == true) ? "LONG" : "SHORT",
    mkt->asset, mkt->currency, p->trade_num, p->price,
    (p->islong) ? "009900" : "990000");

  for(; y < ci->bt_seen && x < HISTSIZE; x++)
  {
    const struct candle *c = &ci->c[y++];

    time2strf(temp, "%F %T", c->time, false);

    fprintf(fp, "%s,%f,%f,%f,%f,%f,%f,%f,%f,%f\n", temp, c->open, c->close,
          c->high, c->low, c->sma1, c->sma2, c->sma3, c->sma4, c->sma5);
  }

  time2strf(temp, "%F %T", p->stamp, false);

  fprintf(fp,
      "EOD\n"
      "plot $whenmoon using 1:2:4:5:3:($3 < $2 ? -1 : 1) notitle with candlesticks palette, \\\n"
      "     ''        using 1:6 title 'SMA-%d' with linespoints ls 1 linecolor rgb '#11CCCC', \\\n"
      "     ''        using 1:7 title 'SMA-%d' with linespoints ls 1 linecolor rgb '#5555ff', \\\n"
      "     ''        using 1:8 title 'SMA-%d' with linespoints ls 1 linecolor rgb '#009900', \\\n"
      "     ''        using 1:9 title 'SMA-%d' with linespoints ls 1 linecolor rgb '#999900', \\\n"
      "     ''        using 1:10 title 'SMA-%d' with linespoints ls 1 linecolor rgb '#990000', \\\n"
      "     \"<echo '%s','%f'\" using 1:2 title '%s' with linespoints ls 2 linecolor rgb '#FFFF00'\n",
      mkt->u.sma1, mkt->u.sma2, mkt->u.sma3, mkt->u.sma4, mkt->u.sma5, temp, p->price,
      (p->islong) ? "LONG" : "SHORT");

  fclose(fp);
}

void
bt_mkplots(const struct market *mkt)
{
  int cint = 0;

  for(; cint < CINTS_MAX; cint++)
  {
    struct candle *lc = mkt->cint[cint].lc;
    struct bt_plot p;
    char path[300];

    if(mkt->cint[cint].tc < HISTSIZE)
      break;

    snprintf(path, 299, "%s/%s-%s-%s/plots/trade%05d-%s.gnuplot", global.conf.logpath,
        mkt->exch->name, mkt->asset, mkt->currency, mkt->stats.p_trades,
        cint2str(cint_length(cint)));

    p.mkt = mkt;
    p.path = path;
    p.cint = cint;
    p.trade_num = mkt->stats.p_trades;

    if(mkt->state & MKS_LONG)
    {
      p.islong = true;
      p.price = mkt->buy_price;
      p.stamp = mkt->buy_time;
    }

    else
    {
      p.islong = false;
      p.price = mkt->sell_price;
      p.stamp = mkt->sell_time; 
    }

    p.idx = lc->num - (HISTSIZE - (HISTSIZE / 4));

    if(p.idx < HISTSIZE)
      p.idx = HISTSIZE;

    bt_mkplot(&p);
  }
}

void
bt_logshort(const struct market *mkt)
{
  char path[300], temp[50], fn[50];
  int cint = 0;
  FILE *fp;

  struct candle *ct = &mkt->cint[0].c[mkt->cint[0].buy_num];

  bt_mkplots(mkt);

  time2strf(temp, "%F %T", mkt->cint[0].lc->time, false);

  sprintf(fn, "plots/trade%05d.html", mkt->stats.p_trades);

  snprintf(path, 299, "%s/%s-%s-%s/%s", global.conf.logpath, mkt->exch->name,
      mkt->asset, mkt->currency, fn);

  fprintf(fp_index,
      "      <tr>\n"
      "        <td><a href=\"%s\" target=\"_new\">paper trade #%d</a></td>\n"
      "        <td>%s</td>\n"
      "        <td>%s</td>\n"
      "        <td style=\"color:%s;\">%.11f</td>\n"
      "        <td style=\"color:%s;\">%.2f</td>\n"
      "        <td>%.2f</td>\n"
      "        <td>%.2f</td>\n"
      "        <td>%d</td>\n"
      "        <td>%f</td>\n"
      "        <td>%f</td>\n"
      "        <td>%f</td>\n"
      "        <td>%.11f</td>\n"
      "        <td>%.11f</td>\n"
      "        <td>%.11f</td>\n"
      "      </tr>\n",

      fn, mkt->stats.p_trades, temp, mkt->a.signal,
      (mkt->profit > 0) ? "#00FF00" : "#FF0000", mkt->profit,
      (mkt->profit > 0) ? "#00FF00" : "#FF0000", mkt->profit_pct,
      mkt->profit_pct_high, mkt->profit_pct_low, mkt->exposed,
      mkt->u1, mkt->u2, mkt->u3,
      mkt->stats.p_profit, mkt->stats.p_fees,
      mkt->stats.p_start_cur + mkt->stats.p_profit);

  // left column = winning longs
  if(bt_flip == false && mkt->profit > 0)
  {
    fprintf(fp_wldiff, "      <tr>\n");
    bt_flip = true;
  }

  fprintf(fp_wldiff,
      "        <td>\n"
      "          <a href=\"plots/trade%05d.html\" target=\"_new\"><img src=\"plots/trade%05d-%s.png\"></a><br>\n"
      "          <a href=\"plots/trade%05d.html\" target=\"_new\"><img src=\"plots/trade%05d-%s.png\"></a><br>\n"
      "        </td>\n",
      mkt->stats.p_trades, mkt->stats.p_trades - 1, cint2str(cint_length(1)),
      mkt->stats.p_trades, mkt->stats.p_trades - 1, cint2str(cint_length(4)));

  // right column = losing longs
  if(bt_flip == true && mkt->profit <= 0)
  {
    fprintf(fp_wldiff, "      </tr>\n");
    bt_flip = false;
  }

  sprintf(fn, "plots/trade%05d.html", mkt->stats.p_trades);

  snprintf(path, 299, "%s/%s-%s-%s/%s", global.conf.logpath, mkt->exch->name,
      mkt->asset, mkt->currency, fn);

  if((fp = fopen(path, "w")) == NULL)
  {
    logger(C_BT, CRIT, "bt_logshort", "error creating '%s': %s", path, strerror(errno));
    return;
  }

  fprintf(fp, "<html>\n"
    "  <head>\n"
    "    <title>WHENMOON [%s:%s-%s] trades #%d and #%d</title>\n"
    "    <style>\n"
    "      pre { font-family: Input Mono; font-size: 20px; }\n"
    "      body { background-color: black; }\n"
    "    </style>\n"
    "  </head>\n"
    "  <body>\n"
    "    <center>\n"
    "    <h1>WHENMOON [%s:%s-%s] trades #%d and #%d</h1><br>\n"
    "    <h2><font color=\"%s\">sig: %s profit: %f (%.2f)</font></h2><br>\n"
    "    <table>\n",

    mkt->exch->name, mkt->asset, mkt->currency,
    mkt->stats.p_trades - 1, mkt->stats.p_trades,
    mkt->exch->name, mkt->asset, mkt->currency,
    mkt->stats.p_trades - 1, mkt->stats.p_trades,
    (mkt->profit > 0) ? "#00FF00" : "#FF0000",
    mkt->a.signal, mkt->profit, mkt->profit_pct);

  // print the long & short candle for each interval
  for(; cint < CINTS_MAX; cint++)
  {
    const struct candle_int *ci = &mkt->cint[cint];
    char temp[6][1000];

    if(mkt->cint[cint].tc < HISTSIZE)
      break;

    strncpy(temp[0], candle_str(&ci->c[ci->buy_num - 3], ci->length, false), 999);
    strncpy(temp[1], candle_str(&ci->c[ci->sell_num - 3], ci->length, false), 999);
    strncpy(temp[2], candle_str(&ci->c[ci->buy_num - 2], ci->length, false), 999);
    strncpy(temp[3], candle_str(&ci->c[ci->sell_num - 2], ci->length, false), 999);
    strncpy(temp[4], candle_str(&ci->c[ci->buy_num - 1], ci->length, false), 999);
    strncpy(temp[5], candle_str(&ci->c[ci->sell_num - 1], ci->length, false), 999);

    fprintf(fp,
        "      <tr>\n"
        "        <td><pre>%s</pre></td>\n"
        "        <td><pre>%s</pre></td>\n"
        "      </tr>\n"
        "      <tr>\n"
        "        <td><pre>%s</pre></td>\n"
        "        <td><pre>%s</pre></td>\n"
        "      </tr>\n"
        "      <tr>\n"
        "        <td><pre>%s</pre></td>\n"
        "        <td><pre>%s</pre></td>\n"
        "      </tr>\n"
        "      <tr>\n"
        "        <td><img src=\"trade%05d-%s.png\"></td>\n"
        "        <td><img src=\"trade%05d-%s.png\"></td>\n"
        "      </tr>\n",
        temp[0], temp[1], temp[2], temp[3], temp[4], temp[5],
        mkt->stats.p_trades - 1, cint2str(mkt->cint[cint].length),
        mkt->stats.p_trades, cint2str(mkt->cint[cint].length));
  }

  fputs("    </table>\n"
        "    </center>\n"
        "  </body>\n"
        "</html>", fp);

  fclose(fp);
}

// paper trader
// this code needs to be mathematically identical to market.c which is responsible
// for candlemaker and bot mode calculations.
void
bt_market(struct market *mkt)
{
  struct candle *lc = mkt->cint[0].lc;
  int x = 0;

  mkt->strat->update(mkt);

  if(mkt->cint[0].tc <= HISTSIZE)
    return;

  if(mkt->state & MKS_LONG)
  {
    double sell_funds = mkt->buy_size * lc->close;
    double sell_fee = sell_funds * bt_fee;

    sell_funds -= sell_fee;
    mkt->profit = sell_funds - mkt->buy_funds;
    mkt->profit_pct = mkt->profit / mkt->buy_funds * 100;

    mkt->profit_pct_high =
      (mkt->profit_pct > mkt->profit_pct_high || mkt->profit_pct_high == 0)
      ? mkt->profit_pct : mkt->profit_pct_high;

    mkt->profit_pct_low = lc->profit_pct_low =
      (mkt->profit_pct < mkt->profit_pct_low || mkt->profit_pct_low == 0)
      ? mkt->profit_pct : mkt->profit_pct_low;

    //mkt->ht = (cc->close > mkt->ht) ? cc->close : mkt->ht;

    mkt->exposed++;
  }

  else
    mkt->minprofit = lc->close * (1 + bt_fee) * (1 + bt_fee) * 1.001;

  mkt->a.advice = ADVICE_NONE;
  mkt->a.msg = mkt->a.signal = NULL;

  if(mkt->sleepcount)
  {
    mkt->sleepcount--;
    return;
  }

  mkt->strat->advice(mkt);

  if(!(mkt->state & MKS_LONG) && mkt->a.advice == ADVICE_LONG)
  {
    double buy_fee;

    mkt->buy_time = lc->time;
    mkt->buy_price = lc->close;
    mkt->buy_size = (mkt->stats.p_start_cur + mkt->stats.p_profit) / lc->close;
    mkt->buy_funds = mkt->buy_price * mkt->buy_size;
    buy_fee = mkt->buy_funds * bt_fee;
    mkt->buy_funds += buy_fee; // total cost to acquire buy_size
    mkt->state |= MKS_LONG;
    mkt->stats.p_fees += buy_fee;
    mkt->stats.p_trades++;
    mkt->profit = mkt->profit_pct = mkt->profit_pct_high = mkt->profit_pct_low = mkt->ht = 0;
    for(; x < CINTS_MAX && mkt->cint[x].tc >= HISTSIZE; x++)
      mkt->cint[x].buy_num = mkt->cint[x].lc->num;

    if(fp_index != NULL)
      bt_mkplots(mkt);
  }

  else if((mkt->state & MKS_LONG) && mkt->a.advice == ADVICE_SHORT)
  {
    double sell_funds = mkt->buy_size * lc->close;
    double sell_fee = sell_funds * bt_fee;

    sell_funds -= sell_fee;
    mkt->profit = sell_funds - mkt->buy_funds;
    mkt->profit_pct = (sell_funds - mkt->buy_funds) / mkt->buy_funds * 100;

    mkt->sell_size = mkt->buy_size;
    mkt->sell_price = lc->close;
    mkt->stats.p_fees += sell_fee;

    mkt->sell_time = lc->time;
    mkt->state &= ~MKS_LONG;
    mkt->stats.p_trades++;
    mkt->stats.p_profit += mkt->profit;

    if(mkt->profit > 0)
      mkt->stats.p_wins++;
    else
      mkt->stats.p_losses++;

    for(; x < CINTS_MAX && mkt->cint[x].tc >= HISTSIZE; x++)
      mkt->cint[x].sell_num = mkt->cint[x].lc->num;

    if(fp_index != NULL)
      bt_logshort(mkt);
    mkt->sleepcount = 30;
  }

  else
    return;

  if(mkt->log != NULL)
    market_log(mkt);

  mkt->exposed = 0;
}

void
bt_endlog(const struct bt_dataset *ds, const struct bt_case *bc, struct market *mkt)
{
  double first_close = ds->cstore[0][0].close;
  double last_close = ds->cstore[0][ds->hdr.init_candles - 1].close;
  double hodl = (mkt->stats.p_start_cur / first_close) * last_close;
  float profit_pct = mkt->stats.p_profit / mkt->stats.p_start_cur * 100;

  float wla = (mkt->stats.p_wins) ?
      (float) ((float) mkt->stats.p_wins / ( (float)mkt->stats.p_wins + (float)mkt->stats.p_losses ) * (float)100) : 0;

  fprintf(fp_index,
      "    </table>\n"
      "    <br>\n"
      "    <table>\n"
      "      <tr>\n"
      "        <td>trades</td>\n"
      "        <td>wins</td>\n"
      "        <td>losses</td>\n"
      "        <td>wla</td>\n"
      "        <td>profit</td>\n"
      "        <td>start currency</td>\n"
      "        <td>end currency</td>\n"
      "        <td>hodl</td>\n"
      "      </tr>\n"
      "      <tr>\n"
      "        <td>%d</td>\n"
      "        <td>%d</td>\n"
      "        <td>%d</td>\n"
      "        <td>%.2f%%</td>\n"
      "        <td style=\"color: %s;\">%.11f %.2f%%</td>\n"
      "        <td>%.11f</td>\n"
      "        <td style=\"color: %s;\">%.11f</td>\n"
      "        <td>%.11f</td>\n"
      "      </tr>\n"
      "    </table>\n"
      "  </center>\n"
      "  </body>\n"
      "</html>\n",
      mkt->stats.p_trades, mkt->stats.p_wins, mkt->stats.p_losses, wla,
      (mkt->stats.p_profit > 0) ? "#00FF00" : "#FF0000",
      mkt->stats.p_profit, profit_pct, mkt->stats.p_start_cur,
      (mkt->stats.p_profit > 0) ? "#00FF00" : "#FF0000",
      mkt->stats.p_start_cur + mkt->stats.p_profit, hodl);


  fclose(fp_index);

  fprintf(fp_wldiff,
      "    </table>\n"
      "    <br>\n"
      "    <table>\n"
      "      <tr>\n"
      "        <td>trades</td>\n"
      "        <td>wins</td>\n"
      "        <td>losses</td>\n"
      "        <td>wla</td>\n"
      "        <td>profit</td>\n"
      "        <td>start currency</td>\n"
      "        <td>end currency</td>\n"
      "        <td>hodl</td>\n"
      "      </tr>\n"
      "      <tr>\n"
      "        <td>%d</td>\n"
      "        <td>%d</td>\n"
      "        <td>%d</td>\n"
      "        <td>%.2f%%</td>\n"
      "        <td style=\"color: %s;\">%.11f %.2f%%</td>\n"
      "        <td>%.11f</td>\n"
      "        <td style=\"color: %s;\">%.11f</td>\n"
      "        <td>%.11f</td>\n"
      "      </tr>\n"
      "    </table>\n"
      "  </center>\n"
      "  </body>\n"
      "</html>\n",
      mkt->stats.p_trades, mkt->stats.p_wins, mkt->stats.p_losses, wla,
      (mkt->stats.p_profit > 0) ? "#00FF00" : "#FF0000",
      mkt->stats.p_profit, profit_pct, mkt->stats.p_start_cur,
      (mkt->stats.p_profit > 0) ? "#00FF00" : "#FF0000",
      mkt->stats.p_start_cur + mkt->stats.p_profit, hodl);

  fclose(fp_wldiff);
}

// thread entry
void *
bt_testcase(void *arg)
{
  struct bt_thread *me = (struct bt_thread *)arg;
  struct market bt_mkt, *mkt = &bt_mkt;
  struct bt_dataset *ds = me->ds;
  struct bt_case *bc = me->bc;
  struct exchange exch;

  register int x = 0;

  memset((void *)mkt, 0, sizeof(struct market));

  strncpy(exch.name, ds->hdr.exchange, 15);
  mkt->exch = &exch;

  market_conf(mkt);
  mkt->opt |= MKO_PAPER;
 
  bt_start_cur = mkt->stats.p_start_cur;

  // minimum bet is 1% of starting currency
  bt_minfunds = bt_start_cur * .01;

  for(; x < CINTS_MAX; x++)
  {
    mkt->cint[x].c = ds->cstore[x];
    mkt->cint[x].bt_seen = ds->hdr.init_candles / cint_length(x);
  }

  memcpy((void *)mkt->ua, (void *)bc->uargs, sizeof(float) * bc->num_uargs);

  mkt->strat = bt_strat;
  mkt->strat->init(mkt);

  if(bt_numcases == 1)
  {
    strncpy(mkt->asset, ds->hdr.asset, 19);
    strncpy(mkt->currency, ds->hdr.currency, 19);
    mkt->start = ds->cstore[0][0].time;

    if(mkt->opt & MKO_LOG)
      market_mklog(mkt);
    if(mkt->opt & MKO_PLOT)
      bt_mkindex(ds, bc, mkt);
  }
 
  // this loop is computationally expensive 
  for(x = 0; x < ds->hdr.init_candles - 2; x++)
  {
    register int y = 0;

    for(; y < CINTS_MAX; y++)
    {
      struct candle_int *ci = &mkt->cint[y];

      if(++ci->ic == ci->length)
      {
        ci->ic = 0;   // reset interval counter

        // latest candle
        ci->lc_slot = ci->tc++;
        ci->lc = &ci->c[ci->lc_slot];

        if(ci->tc > HISTSIZE && mkt->log != NULL)
          candle_log(mkt, y);
      }
    }

    bt_market(mkt);     // process candle through paper market trading

    // End test early if not enough balance left for reasonable bet
    if(mkt->stats.p_start_cur + mkt->stats.p_profit < bt_minfunds)
      break;
  }

  // update the backtest case stats
  // note this does not need a mutex lock because 1:1 relationship of bc to thread
  if(mkt->stats.p_trades > 0)
  {
    bc->results.wins += mkt->stats.p_wins;
    bc->results.losses += mkt->stats.p_losses;
    bc->results.profit += mkt->stats.p_profit;
    bc->results.trades += mkt->stats.p_trades;
    bc->results.wla = (bc->results.wins) ? (float) ((float)bc->results.wins / ((float)bc->results.wins + (float)bc->results.losses) * (float)100) : 0;
  }

  mkt->strat->close(mkt);

  if(mkt->log != NULL)
    fclose(mkt->log);

  if(fp_index != NULL)
    bt_endlog(ds, bc, mkt);

  pthread_mutex_lock(&me->mutex);

  me->results.wins = mkt->stats.p_wins;
  me->results.losses = mkt->stats.p_losses;
  me->results.profit = mkt->stats.p_profit;
  me->results.trades = mkt->stats.p_trades;
  me->results.wla = (me->results.wins) ? (float) ((float)me->results.wins / ((float)me->results.wins + (float)me->results.losses) * (float)100) : 0;

  pthread_mutex_unlock(&me->mutex);

  bt_endthread(me);
}

void
bt_iterate(void)
{
  unsigned long int x = 0;
  char prog = false;

  for(; x < bt_numcases; x++)
  {
    unsigned short int y = 0;

    for(; y < bt_numdatasets; y++)
    {
      float c = (float)x / bt_numcases * 100;   // pct complete
      int r = (int)c % 5;

      if(prog == false && c > 4)
      {
        if(r == 0)
        {
          unsigned long int left = bt_numcases - x;
          time_t t_elapsed = time(NULL) - t_begin;
          float t_speed = (float)x / t_elapsed;

          prog = true;
          printf("\n%.2f%% complete (iters-per-sec: %.2f left: %lu eta: %s)\n",
              c, t_speed, left, stopwatch_time(left / t_speed));
        }
      }
      else if(r != 0)
        prog = false;

      while(true)
      {
        struct bt_thread *t;
        int z = 0;

        for(; z < bt_numthreads; z++)
        {
          int state;

          t = &bt_threads[z];
          pthread_mutex_lock(&t->mutex);
          state = t->state;
          pthread_mutex_unlock(&t->mutex);

          if(state == BTT_ACTIVE)
            continue;

          if(state == BTT_UNUSED)
            break;

          if(state == BTT_DIE)  // test case completed
          {
            pthread_join(t->thread, NULL);
            t->state = BTT_UNUSED;

            if( (t->ds->results.trades == 0 && t->results.trades > 0) ||
                (t->ds->results.trades > 0 && t->results.trades > 0
                 && t->results.profit > t->ds->results.profit)
              )
            {
              memcpy((void *)&t->ds->results, (void *)&t->results, sizeof(struct mkt_stats));
              t->ds->best_case = t->bc;
            }

            if(t->bc->results.profit > 0
                && (best_case == NULL || best_case->results.profit < t->bc->results.profit))
            {
              int zz = 0;

              printf("[%.2f%%] prof: %f wins: %d losses: %d trades: %d wla: %.2f uargs:",
                  (float)x / bt_numcases * 100,
                t->bc->results.profit, t->bc->results.wins, t->bc->results.losses,
                t->bc->results.trades, t->bc->results.wla);

              for(; zz < t->bc->num_uargs; zz++)
                printf(" %f", t->bc->uargs[zz]);
              puts("");
              best_case = t->bc;
            }
            break; // z is now a usable slot
          }
        }

        if(z < bt_numthreads) // available slot found
        {
          t->state = BTT_ACTIVE;
          t->bc = &bt_cases[x];
          t->ds = &bt_datasets[y];

          if(pthread_create(&t->thread, NULL, bt_testcase, (void *)t) != 0)
          {
            logger(C_BT, CRIT, "bt_begin", "unable to create thread: %s\n", strerror(errno));
            exit_program("bt_begin", FAIL);
          }
          break;
        }
        else
          usleep(200);
      }
    }
  }

  for(x = 0; x < bt_numthreads; x++)
  {
    while(true)
    {
      struct bt_thread *t = &bt_threads[x];
      int state;

      pthread_mutex_lock(&t->mutex);
      state = t->state;
      pthread_mutex_unlock(&t->mutex);

      if(state == BTT_ACTIVE)
      {
        usleep(200);
        continue;
      }

      if(state == BTT_DIE)
      {
        pthread_join(t->thread, NULL);
        t->state = BTT_UNUSED;

        if( (t->ds->results.trades == 0 && t->results.trades > 0) ||
            (t->ds->results.trades > 0 && t->results.trades > 0 && t->results.profit > t->ds->results.profit)
          )
        {
          memcpy((void *)&t->ds->results, (void *)&t->results, sizeof(struct mkt_stats));
          t->ds->best_case = t->bc;
        }
      }
      break;
    }
  }
}

int
bt_read_data(const char *fn, int slot)
{
  int y = 0;
  FILE *fp;

  if(bt_numdatasets >= BTDS_MAX)
    return(FAIL);

  if((fp = fopen(fn, "rb")) == NULL)
  {
    logger(C_BT, CRIT, "bt_read_data", "unable to read file '%s': %s", fn, strerror(errno));
    return(FAIL);
  }

  if(fread((void *)&bt_datasets[slot].hdr, sizeof(struct bt_hdr), 1, fp) != 1)
  {
    fclose(fp);
    logger(C_BT, CRIT, "bt_read_data", "error reading candle data from '%s': %s",
        fn, strerror(errno));
    return(FAIL);
  }

  if(bt_datasets[slot].hdr.soa != 6809
      || bt_datasets[slot].hdr.cst_size != sizeof(struct candle))
  {
    fclose(fp);
    logger(C_BT, CRIT, "bt_read_data", "non-whenmoon candle file: %s", fn);
    return(FAIL);
  }

  for(; y < CINTS_MAX; y++)
  {
    int rc = bt_datasets[slot].hdr.init_candles / cint_length(y);

    bt_datasets[slot].cstore[y] = (struct candle *)mem_alloc(sizeof(struct candle) * rc);

    if(fread((void *)bt_datasets[slot].cstore[y], sizeof(struct candle), rc, fp) != rc)
    {
      fclose(fp);
      logger(C_BT, CRIT, "bt_read_data", "error reading candle data from '%s': %s",
          fn, strerror(errno));
      return(FAIL);
    }
//    else
//      printf("read %d (%s) candles from '%s'\n", rc, cint2str(cint_length(y)), fn);

    if(!y)
      bt_numcandles += rc;
  }
  fclose(fp);

  return(SUCCESS);
}

void
bt_add_case(const int argc, const float *argf)
{
  if(bt_numcases < BTT_MAXCASES)
  {
    int x = 0;

    bt_cases[bt_numcases].num_uargs = argc;

    for(; x < argc; x++)
      bt_cases[bt_numcases].uargs[x] = argf[x];

    bt_numcases++;
  }
}

int
bt_read_cases(const char *fn)
{
  FILE *fp = fopen(fn, "r");
  char buf[1000];

  if(fp == NULL)
  {
    logger(C_BT, CRIT, "bt_read_cases", "unable to open test cases '%s': %s",
        fn, strerror(errno));
    return(0);
  }

  bt_cases = (struct bt_case *)mem_alloc(sizeof(struct bt_case) * BTT_MAXCASES);
  memset((void *)bt_cases, 0, sizeof(struct bt_case) * BTT_MAXCASES);

  while(!feof(fp) && fgets(buf, 999, fp) && bt_numcases < BTT_MAXCASES)
  {
    if(buf[0] == '#' || buf[0] == ' ' || buf[0] == '\r' || buf[0] == '\n')
      continue;

    buf[strlen(buf) - 1] = '\0';

    printf("Expanding cases: %s\n", buf);

    if(expstr(buf, bt_add_case) == FAIL)
    {
      logger(C_BT, CRIT, "bt_read_cases", "expstr() returned failure");
      fclose(fp);
      return(0);
    }
  }

  printf("\nExpanded %lu test cases from '%s'\n", bt_numcases, fn);

  fclose(fp);
  return(bt_numcases);
}

void
bt_init_threads(void)
{
  int x = 0;

  printf("\nSetting up %d threads for %s\n", bt_numthreads, find_cpuname());

  bt_threads = (struct bt_thread *)mem_alloc(sizeof(struct bt_thread) * bt_numthreads);
  memset((void *)bt_threads, 0, sizeof(struct bt_thread) * bt_numthreads);

  for(; x < bt_numthreads; x++)
  {
    struct bt_thread *t = &bt_threads[x];

    t->num = x;
    t->state = BTT_UNUSED;
    pthread_mutex_init(&t->mutex, NULL);
  }
}

void
bt_dumpcases(FILE *fp, struct bt_case **sorted)
{
  int x = 0;

  for(; x < bt_numcases; x++)
  {
    struct bt_case *bc = sorted[x];
    int y = 0;

    if(!bc->results.trades)
      continue;

    fprintf(fp, "%f %d %d %d %.2f uargs:",
        bc->results.profit, bc->results.wins, bc->results.losses,
        bc->results.trades, bc->results.wla);
    for(; y < bc->num_uargs; y++)
       fprintf(fp, " %f", bc->uargs[y]);
    fprintf(fp, "\n");
  }
}

void
bt_report(FILE *out)
{
  struct bt_case **sortme, *bc;
  int x = 0;

  fprintf(out, "\n"
               "dataset           profit      pct     trades  wla     end_currency       hodl           uargs:\n"
               "----------------------------------------------------------------------------------------------------------------\n");

  for(; x < bt_numdatasets; x++)
  {
    struct bt_dataset *ds = &bt_datasets[x];
    double first_close = ds->cstore[0][0].close;
    double last_close = ds->cstore[0][ds->hdr.init_candles - 1].close;
    float profit_pct = ds->results.profit / bt_start_cur * 100;
    float wla = (ds->results.wins) ?
      (float) ((float) ds->results.wins / ( (float)ds->results.wins + (float)ds->results.losses ) * (float)100) : 0;
    char mname[100];
    int y = 0;

    snprintf(mname, 99, "%s:%s-%s", ds->hdr.exchange, ds->hdr.asset, ds->hdr.currency);

    fprintf(out, "%-16s  %s%-10f  %-6.2f%s  %-6d  %-6.2f  %-17.9f  %-17.9f", mname,
        (ds->results.profit > 0) ? GREEN : RED, ds->results.profit, profit_pct, ARESET,
        ds->results.trades, wla, bt_start_cur + ds->results.profit,
        (bt_start_cur / first_close) * last_close);

    if(ds->best_case != NULL)
    {
      for(; y < ds->best_case->num_uargs; y++)
       fprintf(out, " %f", ds->best_case->uargs[y]);
    }
    fprintf(out, "\n");
  }

  sortme = (struct bt_case **)mem_alloc(sizeof(struct bt_case **) * bt_numcases);

  for(x = 0; x < bt_numcases; x++)
    sortme[x] = &bt_cases[x];

  qsort((void *)sortme, bt_numcases, sizeof(struct bt_case **), bt_sort_profit);

  bc = sortme[0];
  fprintf(out, "\nBest case by profit:\n"
               "%f %d %d %d %.2f uargs:",
               bc->results.profit, bc->results.wins, bc->results.losses,
               bc->results.trades, bc->results.wla);

  for(x = 0; x < bc->num_uargs; x++)
     fprintf(out, " %f", bc->uargs[x]);
  fprintf(out, "\n");

  if(best_cases[0].results.trades)
  {
    fprintf(out, "%s%f %d %d %d %.2f%s uargs:",
        PURPLE, best_cases[0].results.profit,
        best_cases[0].results.wins, best_cases[0].results.losses,
        best_cases[0].results.trades, best_cases[0].results.wla, ARESET);
    for(x = 0; x < best_cases[0].num_uargs; x++)
       fprintf(out, " %f", best_cases[0].uargs[x]);
    fprintf(out, "\n");
  }

  memcpy((void *)&best_cases[0], (void *)bc, sizeof(struct bt_case));

  if(out != stdout)
  {
    fprintf(out, "\nSORTED by profit\n"
                "================\n\n");
    bt_dumpcases(out, sortme);
    fprintf(out, "\n");
  }

  qsort((void *)sortme, bt_numcases, sizeof(struct bt_case **), bt_sort_wla);

  bc = sortme[0];
  fprintf(out, "\nBest case by win/loss average: \n"
               "%f %d %d %d %.2f uargs:",
               bc->results.profit, bc->results.wins, bc->results.losses,
               bc->results.trades, bc->results.wla);

  for(x = 0; x < bc->num_uargs; x++)
     fprintf(out, " %f", bc->uargs[x]);
  fprintf(out, "\n");

  if(best_cases[1].results.trades)
  {
    fprintf(out, "%s%f %d %d %d %.2f%s uargs:",
        PURPLE, best_cases[1].results.profit,
        best_cases[1].results.wins, best_cases[1].results.losses,
        best_cases[1].results.trades, best_cases[1].results.wla, ARESET);
    for(x = 0; x < best_cases[1].num_uargs; x++)
       fprintf(out, " %f", best_cases[1].uargs[x]);
    fprintf(out, "\n");
  }

  memcpy((void *)&best_cases[1], (void *)bc, sizeof(struct bt_case));

  if(out != stdout)
  {
    fprintf(out, "\nSORTED by WLA\n"
                 "=============\n\n");
    bt_dumpcases(out, sortme);
    fprintf(out, "\n\n");
  }

  qsort((void *)sortme, bt_numcases, sizeof(struct bt_case **), bt_sort_trades);

  bc = sortme[0];
  fprintf(out, "\nBest case by num trades\n"
               "%f %d %d %d %.2f uargs:",
               bc->results.profit, bc->results.wins, bc->results.losses,
               bc->results.trades, bc->results.wla);

  for(x = 0; x < bc->num_uargs; x++)
     fprintf(out, " %f", bc->uargs[x]);
  fprintf(out, "\n");

  if(best_cases[2].results.trades)
  {
    fprintf(out, "%s%f %d %d %d %.2f%s uargs:",
        PURPLE, best_cases[2].results.profit,
        best_cases[2].results.wins, best_cases[2].results.losses,
        best_cases[2].results.trades, best_cases[2].results.wla, ARESET);
    for(x = 0; x < best_cases[2].num_uargs; x++)
       fprintf(out, " %f", best_cases[2].uargs[x]);
    fprintf(out, "\n");
  }

  memcpy((void *)&best_cases[2], (void *)bc, sizeof(struct bt_case));

  if(out != stdout)
  {
    fprintf(out, "\nSORTED by TRADES\n"
                "==================\n\n");
    bt_dumpcases(out, sortme);
  }

  fputs("\n", out);
  mem_free((void *)sortme, sizeof(struct bt_case *) * bt_numcases);
}

void
bt_results(const char *path)
{
  FILE *fp;
  char fn_bc[500], fn_report[500];

  snprintf(fn_bc, 499, "%s/best.wm", path);

  memset((void *)best_cases, 0, sizeof(struct bt_case) * 3);

  if((fp = fopen(fn_bc, "r")) != NULL)
  {
    if(fread((void *)best_cases, sizeof(struct bt_case), 3, fp) != 3)
      logger(C_BT, WARN, "bt_report", "error reading best case data: %s (ignored)",
          strerror(errno));
    fclose(fp);
  }

  bt_report(stdout);

  snprintf(fn_report, 499, "%s/wmbt_report.txt", path);

  if((fp = fopen(fn_report, "w")) == NULL)
  {
    logger(C_BT, CRIT, "bt_report", "unable to create report '%s': %s", fn_report, strerror(errno));
    return;
  }

  bt_report(fp);
  fclose(fp);

  if((fp = fopen(fn_bc, "w")) == NULL)
  {
    logger(C_BT, CRIT, "bt_report", "unable to create best data file '%s': %s", fn_bc, strerror(errno));
    return;
  }

  if(fwrite((void *)best_cases, sizeof(struct bt_case), 3, fp) != 3)
  {
    logger(C_BT, CRIT, "bt_report", "error writing best case data: %s", strerror(errno));
    return;
  }
  fclose(fp);
}

void
backtest(const int argc, const char *argv[])
{
  FILE *fp;
  int x = 0;
  char *temp;

  init_globals();
  set_signals();
  strat_init();

  global.conf.logchans = C_MAIN | C_MISC | C_BT;
  global.conf.logsev = INFO;
  global.state = WMS_BT;

  banner();
  puts("Backtest mode enabled\n");

  if((bt_numthreads = atoi(argv[3])) < 1)
  {
    logger(C_BT, CRIT, "backtest", "error: need a least 1 thread");
    exit_program("backtest", FAIL);
  }

  bt_numthreads = (bt_numthreads < BTT_MAX) ? bt_numthreads : BTT_MAX;

  if((bt_strat = strat_find(argv[2])) == NULL)
  {
    logger(C_BT, CRIT, "backtest", "unknown strategy: '%s'", argv[2]);
    exit_program("backtest", FAIL);
  }

  x = 6;
  bt_numdatasets = argc - x;

  bt_datasets = (struct bt_dataset *)mem_alloc(sizeof(struct bt_dataset) * bt_numdatasets);
  memset((void *)bt_datasets, 0, sizeof(struct bt_dataset) * bt_numdatasets);

  if((temp = getenv("FEE")) != NULL)
    bt_fee = strtof(temp, NULL);
  if(bt_fee == 0)
    bt_fee = 0.004;

  printf("\nUsing fee %f\n", bt_fee);

  for(; x < argc; x++)
  {
    struct bt_dataset *ds = &bt_datasets[x - 6];

    printf("Loading '%s': ", argv[x]);
    fflush(stdout);

    if(bt_read_data(argv[x], x - 6) == FAIL)
      exit_program("backtest", FAIL);

    printf("market %s:%s-%s (%d 1-min candles)\n", ds->hdr.exchange, ds->hdr.asset, ds->hdr.currency, ds->hdr.init_candles);
  }

  if(bt_read_cases(argv[4]) == 0)
    exit_program("backtest", FAIL);

  strncpy(global.conf.logpath, argv[5], 199);

  if(bt_numcases && bt_numdatasets)
  {
    bt_init_threads();

    puts("\nLaunching threads...");

    t_begin = time(NULL);
    bt_iterate();

    printf("\nCompleted %d iterations against %lu 1-minute candles in %s\n\n",
        bt_numcases, bt_numcandles, stopwatch_time(time(NULL) - t_begin));

    bt_results(argv[5]);
  }
  else
    logger(C_BT, CRIT, "backtest", "missing test cases and/or data sets");

  exit_program("backtest", SUCCESS);
}

void
bt_cleanup(void)
{
  // wait for threads to die before we cleaning up shared mem
  if(bt_threads != NULL)
  {
    int x = 0;

    for(; x < bt_numthreads; x++)
    {
      struct bt_thread *t = &bt_threads[x];
      int state;

      pthread_mutex_lock(&t->mutex);
      state = t->state;
      pthread_mutex_unlock(&t->mutex);

      if(state == BTT_ACTIVE)
      {
        while(true)
        {
          logger(C_BT, INFO, "bt_cleanup", "waiting on thread #%d to finish", x);
          sleep(1);
          pthread_mutex_lock(&t->mutex);
          state = t->state;
          pthread_mutex_unlock(&t->mutex);
          if(state == BTT_DIE)
            break;
        }
      }

      if(state == BTT_DIE)
      {
        pthread_mutex_lock(&t->mutex);
        pthread_join(t->thread, NULL);
        t->state = BTT_UNUSED;
        pthread_mutex_unlock(&t->mutex);
      }

      if(state == BTT_UNUSED)
      {
        pthread_mutex_destroy(&t->mutex);
        t->state = BTT_CLEAN;
      }
    }
    mem_free((void *)bt_threads, sizeof(struct bt_thread) * bt_numthreads);
    bt_threads = NULL;
  }

  if(bt_cases != NULL)
  {
    mem_free((void *)bt_cases, sizeof(struct bt_case) * BTT_MAXCASES);
    best_case = bt_cases = NULL;
  }

  if(bt_datasets != NULL)
  {
    int x = 0;

    for(; x < bt_numdatasets; x++)
    {
      int y = 0;
  
      for(; y < CINTS_MAX; y++)
      {
        int rc = bt_datasets[x].hdr.init_candles / cint_length(y);

        if(bt_datasets[x].cstore[y] != NULL)
        {
          mem_free(bt_datasets[x].cstore[y], sizeof(struct candle) * rc);
          bt_datasets[x].cstore[y] = NULL;
        }
      }
    }
    mem_free((void *)bt_datasets, sizeof(struct bt_dataset) * bt_numdatasets);
    bt_datasets = NULL;
  }
}
