#define WM_CANDLEC
#include "candle.h"

// rounds time to previous beginning interval based on gran
static void
round_time(const int gran, struct tm *ct)
{
  ct->tm_sec = 0;

  switch(gran)
  {
    case 300:
      ct->tm_min = ct->tm_min - (ct->tm_min % 5);
      break;
    case 900:
      ct->tm_min = ct->tm_min - (ct->tm_min % 15);
      break;
    case 1800:
      ct->tm_min = ct->tm_min - (ct->tm_min % 30);
      break;
    case 3600:
      ct->tm_min = 0;
      break;
    case 14400:
      ct->tm_min = 0;
      ct->tm_hour = ct->tm_hour - (ct->tm_hour % 4);
      break;
    case 86400:
      ct->tm_min = 0;
      ct->tm_hour = 0;
      break;
  }
}

// create a new candle with 0 volume/trades based on prev
void
candle_mkempty(struct candle *ec, const struct candle *pc)
{
  ec->trades = 0L;
  ec->volume = 0;
  ec->open = pc->open;
  ec->high = pc->high;
  ec->low = pc->low;
  ec->close = pc->close;
}

// find candle start time based on gran
time_t
candle_time(const time_t when, const int gran, const int round)
{
  struct tm tm_temp;

  gmtime_r(&when, &tm_temp);  // UTC EPOCH
  round_time(gran, &tm_temp); // find interval start time

  if(round == true)
  {
    tm_temp.tm_min = 0;
    tm_temp.tm_hour = 0;
  }

  return(timegm(&tm_temp));
}

// generates candle data string
char *
candle_str(const struct candle *cc, const int length, const int color)
{
  char trim[8][100];

  time2str(trim[0], cc->time, false);

  snprintf(global.ctmp, 5000,
      "#%lu [ %s (%s) ] score: %d\n"
      "c: %s (%s%s%.2f%s) o: %s h: %s l: %s v: %s s: %.2f (%s%.2f%s)\n"
      "h: %s (%.2f / %.2f / %d) l: %s (%.2f / %.2f / %d) hlm: %.2f diff: %.2f\n"
      "L2: %.2f L3: %.2f L5: %.2f L10: %.2f L25: %.2f L50: %.2f L100: %.2f L150: %.2f all: %.2f\n"
      "sma1: %.2f (%.2f) sma2: %.2f sma3: %.2f sma4: %.2f sma5: %.2f (%.2f)\n"
      "sma1/2: %.2f sma1/4: %.2f sma1/5: %.2f sma2/5: %.2f sma3/5: %.2f sma4/5: %.2f\n"
      "vsma1: %.2f vsma2: %.2f vsma3: %.2f vsma4: %.2f vsma5: %.2f\n"
      "vsma1/2: %.2f vsma1/4: %.2f vsma1/5: %.2f vsma2/5: %.2f vsma3/5: %.2f vsma4/5: %.2f\n"
      "vwma1: %.2f vwma2: %.2f vwma3: %.2f vwma4: %.2f vwma5: %.2f\n"
      "vg1: %.2f vg2: %.2f vg3: %.2f vg4: %.2f vg5: %.2f\n"
      "adx: %.f rsi: %.2f (%.2f / %.2f) macd_h: %s fish: %.2f fish_sig: %.2f\n"
      "t_fish: %s/%d/%d t_sma1_2: %s/%d/%d t_sma1_5: %s/%d/%d\n"
      "t1: %f t2: %f t3: %f t4: %f t5: %f",

      cc->num, trim[0], cint2str(length), cc->score,

      trim0(trim[1], 100, cc->close),

      (cc->last2 > 0) ? "+" : "",

      (color == true) ? (cc->last2 > 0) ? GREEN : RED : "", cc->last2,
      (color == true) ? ARESET : "",

      trim0(trim[2], 100, cc->open), trim0(trim[3], 100, cc->high),

      trim0(trim[4], 100, cc->low), trim0(trim[5], 100, cc->volume), cc->size,

      (color == true) ? (cc->asize > 0) ? GREEN : RED : "", cc->asize,
      (color == true) ? ARESET : "",

      trim0(trim[6], 100, cc->ci_high), cc->high_diff, cc->pct_below_high, cc->high_age,

      trim0(trim[7], 100, cc->ci_low), cc->low_diff, cc->pct_above_low, cc->low_age,

      cc->hlm_diff, cc->high_low_diff,

      cc->last2, cc->last3, cc->last5, cc->last10, cc->last25, cc->last50, cc->last100, cc->last150, cc->all,

      cc->sma1_diff, cc->sma1_angle, cc->sma2_diff, cc->sma3_diff, cc->sma4_diff, cc->sma5_diff, cc->sma5_angle,

      cc->sma1_2_diff, cc->sma1_4_diff, cc->sma1_5_diff, cc->sma2_5_diff, cc->sma3_5_diff, cc->sma4_5_diff,

      cc->vsma1_diff, cc->vsma2_diff, cc->vsma3_diff, cc->vsma4_diff, cc->vsma5_diff,

      cc->vsma1_2_diff, cc->vsma1_4_diff, cc->vsma1_5_diff, cc->vsma2_5_diff, cc->vsma3_5_diff, cc->vsma4_5_diff,

      cc->vwma1_sma1_diff, cc->vwma2_sma2_diff, cc->vwma3_sma3_diff, cc->vwma4_sma4_diff, cc->vwma5_sma5_diff,

      cc->vg1, cc->vg2, cc->vg3, cc->vg4, cc->vg5,

      cc->adx, cc->rsi, cc->rsi_sma, (cc->rsi - cc->rsi_sma) / cc->rsi_sma * 100, 

      trim0(trim[6], 100, cc->macd_h), cc->fish, cc->fish_sig,

      (cc->t_fish.dir == UP) ? "up" : "down", cc->t_fish.count, cc->t_fish.prev,
      (cc->t_sma1_2.dir == UP) ? "up" : "down", cc->t_sma1_2.count, cc->t_sma1_2.prev,
      (cc->t_sma1_5.dir == UP) ? "up" : "down", cc->t_sma1_5.count, cc->t_sma1_5.prev,

      cc->test1, cc->test2, cc->test3, cc->test4, cc->test5);

  return(global.ctmp);
}

void
candle_log(const struct market *mkt, const int cint)
{
  if(cint == 0 && mkt->state & MKS_LONG)
  {
    char trim[100];
      
    fprintf(mkt->log,
        "\n[[ EXPOSED ]] %s @ %s%.2f%s (high: %.2f low: %.2f) exp: %lu\n",
        trim0(trim, 99, mkt->profit),
        (mkt->profit > 0) ? GREEN : RED, mkt->profit_pct, ARESET,
        mkt->profit_pct_high, mkt->profit_pct_low, mkt->exposed);
  }
  fprintf(mkt->log, "\n%s\n", candle_str(mkt->cint[cint].lc, cint_length(cint), true));
}

// generate html report for a trade
// this is called by market_close for SHORT orders
void
candle_html(const struct market *mkt, const char *signal, const double profit, const double profit_pct)
{
  int trade_num = (mkt->opt & MKO_PAPER) ? mkt->stats.p_trades : mkt->stats.trades;
  char title[100], ttype[10], path[300];
  int x = 0;
  FILE *fp;

  if(mkt->opt & MKO_PAPER)
  {
    snprintf(path, 299, "%s/%s-%s-%s/ptrade%05d.html", global.conf.logpath, 
        mkt->exch->name, mkt->asset, mkt->currency, trade_num);
    snprintf(title, 99,  "Whenmoon Paper Trades #%d and #%d", trade_num - 1, trade_num);
    strcpy(ttype, "ptrade");
  }
  else
  {
    snprintf(path, 299, "%s/%s-%s-%s/trade%05d.html", global.conf.logpath,
        mkt->exch->name, mkt->asset, mkt->currency, trade_num);
    snprintf(title, 99, "Whenmoon Trades #%d and #%d", trade_num - 1, trade_num);
    strcpy(ttype, "trade");
  }

  if((fp = fopen(path, "w")) == NULL)
  {
    logger(C_MISC, CRIT, "candle_html", "file create error '%s': %s", path, strerror(errno));
    return;
  }

  fprintf(fp, "<html>\n"
      "  <head>\n"
      "    <title>%s</title>\n"
      "    <style>\n"
      "      pre { font-family: Input Mono; font-size: 20px; }\n"
      "    </style>\n"
      "  </head>\n"
      "  <body>\n"
      "    <center>\n"
      "    <h1>%s</h1><br>\n"
      "    <h2><font color=\"%s\">sig: %s profit: %f (%.2f)</font></h2><br>\n"
      "    <table>\n",
      title, title,
      !(profit > 0) ? "#FF0000" : "#00FF00",
      signal, profit, profit_pct);

  // print the long & short candle for each interval
  for(; x < CINTS_MAX; x++)
  {
    char trim[6][2000];
    int y = 0, z = 0;

    if(mkt->cint[x].tc < HISTSIZE)
      break;

    //for(; z < 3; z++)
      //candle_str(temp[y++], mkt->cint[x].length, &mkt->long_candle[(x*3)+z]);

    //for(z = 0; z < 3; z++)
      //candle_str(temp[y++], mkt->cint[x].length, &mkt->short_candle[(x*3)+z]);

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
        "        <td><img src=\"plots/%s%05d-%s-hl.png\"></td>\n"
        "        <td><img src=\"plots/%s%05d-%s-hl.png\"></td>\n"
        "      </tr>\n"
        "      <tr>\n"
        "        <td><img src=\"plots/%s%05d-%s.png\"></td>\n"
        "        <td><img src=\"plots/%s%05d-%s.png\"></td>\n"
        "      </tr>\n",
        trim[2], trim[5], trim[1], trim[4], trim[0], trim[3],
        ttype, trade_num - 1, cint2str(mkt->cint[x].length), ttype, trade_num,
        cint2str(mkt->cint[x].length),
        ttype, trade_num - 1, cint2str(mkt->cint[x].length), ttype, trade_num,
        cint2str(mkt->cint[x].length));
  }

  fprintf(fp,
      "    </table>\n"
      "  </body>\n"
      "</html>\n");

  fclose(fp);
}
