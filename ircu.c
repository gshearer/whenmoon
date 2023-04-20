#define WM_IRCUC
#include "ircu.h"

void
ircu_identify(void)
{
  char *pass = irc[global.is].args;

  if(!strcmp(global.conf.adminpass, pass))
  {
    int x = irc_addAdmin(irc[global.is].from);

    if(x != -1)
      irc_broadcast("%s will be recognized as an admin for the next %d seconds", irc[global.is].admin[x].nick, IRC_ADMINTIME);
    else
      irc_reply("Maximum admins reached. Denied :~(?");
    return;
  }
  irc_reply("Incorrect. This attempt has been logged.");
  logger(C_IRCU, CRIT, "ircu_identify",
      "admin access attempt by %s on %s:%s with pw %s", irc[global.is].from,
      irc[global.is].server, irc[global.is].port, pass);
}

void
ircu_math(void)
{
  double res = te_interp(irc[global.is].args, 0);
  irc_reply("%f", res);
}

void
ircu_diff(void)
{
  char *str = irc[global.is].args;
  const char *argv[20];
  int argc = 0;
  double x, y;

  for(; (argv[argc] = strsep(&str, " ")) != NULL && argc < 20; argc++);

  if(argc == 2)
  {
    x = strtod(argv[0], NULL);
    y = strtod(argv[1], NULL);

    if(x == 0 || y == 0)
      irc_reply("no.");
    else
    {
      double z = (x - y) / y * 100;
      irc_reply("%.11f", z);
    }
  }

  else
    irc_reply("usage: diff num1 num2");
}

void
ircu_ping(void)
{
  char *args = irc[global.is].args;
  irc_reply("PONG %s", (args) ? args : " ");
}

void
ircu_version(void)
{
  irc_reply("%s", global.conf.verstring);
}

void
ircu_help(void)
{
  int x = 0;
  char buf[1000], *t = buf, *args = irc[global.is].args;

  if(args != NULL)
  {
    while(user_cmds[x].cmd != NULL && strcasecmp(user_cmds[x].cmd, args))
      x++;
    if(user_cmds[x].cmd != NULL)
      irc_reply("Usage: %s", user_cmds[x].usage);
    else
      irc_reply("Unknown command :~(?");
    return;
  }

  buf[0] = '\0';

  while(user_cmds[x].cmd != NULL)
  {
    strcat(t, user_cmds[x].cmd);
    if(user_cmds[x].alias != NULL)
    {
      strcat(t, " [");
      strcat(t, user_cmds[x].alias);
      strcat(t, "]");
    }
    x++;
    if(user_cmds[x].cmd != NULL)
      strcat(t, ", ");
  }
  irc_reply(buf);
}

void
ircu_join(void)
{
  int x = global.is;
  char *args = irc[x].args;

  if(irc_admin() == false)
    irc_reply("No. :~(?");

  else if(args == NULL || args[0] == '\0')
    irc_reply("Usage: join #channel [key]");

  else
  {
    char *temp = args, *chan;

    chan = strsep(&temp, " ");

    if(irc_find_chan(x, chan) != -1)
      irc_reply("I think I'm already on channel %s", chan);

    else
    {
      int y = 0;

      for(; y < IRC_MAXCHANS; y++)
      {
        if(irc[x].chan[y][0] == '\0')
        {
          strncpy(irc[x].chan[y], chan, 39);

          if(temp && *temp != '\0')
            sock_write(irc[x].sock, "JOIN %s :%s", chan, temp);
          else
            sock_write(irc[x].sock, "JOIN %s", chan);
          break;
        }
      }
    }
  }
}

void
ircu_part(void)
{
  int x = global.is, y;
  char *args = irc[x].args;

  if(irc_admin() == false)
    irc_reply("No. :~(?");

  else if(args == NULL || args[0] == '\0')
    irc_reply("Usage: part #channel");

  else if((y = irc_find_chan(x, args)) == -1)
    irc_reply("I don't think I'm on channel %s", args);

  irc[x].chan[y][0] = '\0';
  sock_write(irc[x].sock, "PART %s :Take care now, bye bye then. :~(?", args);
}

void
ircu_8ball(void)
{
  int x = 0;
  char *args = irc[global.is].args;
  char *answers_8ball[] = {
    "It is certain.",
    "It is decidedly so.",
    "Without a doubt.",
    "Yes – definitely.",
    "You may rely on it.",
    "As I see it, yes.",
    "Most likely.",
    "Outlook good.",
    "Yes.",
    "Signs point to yes.",
    "Reply hazy, try again.",
    "Ask again later.",
    "Better not tell you now.",
    "Cannot predict now.",
    "Concentrate and ask again.",
    "Don't count on it.",
    "My reply is no.",
    "My sources say no.",
    "Outlook not so good.",
    "Very doubtful."
  };

  size_t length = sizeof answers_8ball / sizeof *answers_8ball; // or sizeof answers_8ball[0]

  if(args == NULL || args[0] == '\0')
    irc_reply("Usage: 8ball question");
  else
  {
    salt_random();

    while(*args)  /* make user's question meaningful.. sortta */
      x += *args++;
    srand(x + global.now);

    // get random number with `length` of array as max
    x = rand() % length;

    logger(C_IRC, DEBUG2, "ircu_8ball", "random number generated (%d) maps to '%s'", x, answers_8ball[x]);

    irc_reply("🔮 %s", answers_8ball[x]);
  }
}

void
ircu_status(void)
{
  time_t uptime = global.now - global.start, remainder;
  char uptime_str[100], uptime_add[100];
  int x = 0, total = 0;

  for(;x < IRC_MAX; x++)
    if(irc[x].state != IRC_UNUSED)
      total++;

  irc_reply("Active IRC servers: %d", total);
  irc_reply("CURL servicer threads: %d", global.curl_threads);

  total = 0;
  for(x = 0; x < CURL_MAXQUEUE; x++)
  {
    pthread_mutex_lock(&cq[x].mutex);
    if(cq[x].state != CQS_UNUSED)
      total++;
    pthread_mutex_unlock(&cq[x].mutex);
  }

  irc_reply("CURL request queue: %d", total);
  irc_reply("logchans:%d", global.conf.logchans);

  if(global.dbconn != NULL)
    irc_reply("Postgresql connection is active (idle: %d secs)", global.now - global.dbtimer);

  uptime_str[0] = '\0';

  /* Day(s) */
  if(uptime >= 86400)
  {
    remainder = uptime - ((uptime / 86400) * 86400);
    sprintf(uptime_add,"%lu day",uptime / 86400);
    if(uptime/86400 != 1)
      strcat(uptime_add,"s");
    if(remainder)
      strcat(uptime_add,", ");
    uptime = remainder;
    strcat(uptime_str,uptime_add);
  }

  /* Hours */
  if(uptime >= 3600)
  {
    remainder = uptime - ((uptime / 3600) * 3600);
    sprintf(uptime_add,"%lu hour",uptime / 3600);
    if(uptime/3600 != 1)
      strcat(uptime_add,"s");
    if(remainder)
      strcat(uptime_add,", ");
    uptime = remainder;
    strcat(uptime_str,uptime_add);
  }

  /* Minutes */
  if(uptime)
  {
    sprintf(uptime_add,"%lu minute",uptime / 60);
    if(uptime/60 != 1)
      strcat(uptime_add,"s");
    strcat(uptime_str,uptime_add);
  }

  irc_reply("I have been alive for %s", uptime_str);
}

void
ircu_mcap(void)
{
  char mcap[200], mcap_ath[200], vol[200], vol_ath[200];

  d2h(mcap, crypto_global.mcap);
  d2h(mcap_ath, crypto_global.mcap_ath);
  d2h(vol, crypto_global.vol);
  d2h(vol_ath, crypto_global.vol_ath);

  irc_reply("mcap: %s (24h: %.2f%% ath: %s btc: %.2f%%) vol: %s (24h: %.2f%% ath: %s) total: %d assets vintage: %d seconds", mcap, crypto_global.mcap_24h, mcap_ath, crypto_global.btc_dom, vol, crypto_global.vol_24h, vol_ath, crypto_global.assets, global.now - global.cpptimer);
}

void
ircu_news(void)
{
  cpanic_news(irc[global.is].args);
}

void
print_crypto_single(int num)
{
  char mcap[50], vol[50];

  d2h(mcap, cryptos[num].mcap);
  d2h(vol, cryptos[num].vol);

  irc_reply("%s [id: %s sym: %s rank: %d]", cryptos[num].name, cryptos[num].id, cryptos[num].sym, cryptos[num].rank);
  irc_reply("last: %.4f ath: %.4f (%2.2f%%) vol: %s (%2.2f%%) mcap: %s", cryptos[num].price, cryptos[num].ath, cryptos[num].ath_diff, vol, cryptos[num].vol_diff, mcap);
  irc_reply("15m: %2.2f%% 1h: %2.2f%% 24h: %2.2f%% 7d: %2.2f%% 30d: %2.2f%% 1y: %2.2f%%", cryptos[num].change_15m, cryptos[num].change_1h, cryptos[num].change_24h, cryptos[num].change_7d, cryptos[num].change_30d, cryptos[num].change_1y);
}

void
print_crypto_header(void)
{
  irc_reply("Rank  Sym   Name           USD        15-min  1-hour  24-hour  7-Days");
  irc_reply("---------------------------------------------------------------------");
}

void
print_crypto(int num)
{
  irc_reply("%-5d %-5.5s %-14.14s %-10.4f %-7.2f %-7.2f %-8.2f %.2f", cryptos[num].rank, cryptos[num].sym, cryptos[num].name, cryptos[num].price, cryptos[num].change_15m, cryptos[num].change_1h, cryptos[num].change_24h, cryptos[num].change_7d);
}

void
ircu_export(void)
{
  char *temp = irc[global.is].args, *arg;

  if((arg = strsep(&temp, " ")))
  {
    if(!strcasecmp(arg, "products"))
    {
      if((arg = strsep(&temp, " ")))
      {
        struct exchange *exch;

        if((exch = exch_find(arg)) != NULL)
        {
          if((arg = strsep(&temp, " ")))
          {
            if(exch_exportProducts(exch, arg, temp) == SUCCESS)
              irc_reply("Successfully exported %s:%s products to %s", exch->name, arg, temp);
            else
              irc_reply("Export of %s:%s products to %s failed", exch->name, arg, temp);
          }
        }
      }
    }
  }
}

void
ircu_crypto(void)
{
  char *temp = irc[global.is].args, *arg;
  int single, header = false;
  int found = 0;

  if(global.num_cryptos == 0)
  {
    irc_reply("No data yet");
    return;
  }

  if(temp == NULL || *temp == '\0')
  {
    int i = 0;
    print_crypto_header();
    for(; i < 10; i++)
      print_crypto(i);
  }
  else
  {
    single = (strchr(temp, ' ') == NULL) ? true : false;

    while((arg = strsep(&temp, " ")))
    {
      int i = 0;

      for(; i < global.num_cryptos; i++)
      {
        int num = atoi(arg);
        
        if(!num && (!strcasecmp(arg, cryptos[i].sym) || !strcasecmp(arg, cryptos[i].id)))
          num = cryptos[i].rank;

        if(num && num <= global.num_cryptos && cryptos[i].rank == num)
        {
          found++;
          if(single == true)
            print_crypto_single(num - 1);
          else
          {
           if(header == false)
            {
              header = true;
              print_crypto_header();
            }
            print_crypto(num - 1);
          }
          break;
        }
      }
    }
    if(!found)
      irc_reply("Unknown crypto symbol :~(?");
  }
}

void
ircu_showAccounts(const int argc, const char *argv[])
{
  struct exchange *exch = NULL;
  struct account *a = NULL;
  int x = 0, row = 0;

  if(argc >= 2)
  {
    if(!strcasecmp(argv[1], "help"))
    {
      irc_reply("usage: show acc [exchange] [asset]");
      return;
    }

    if((exch = exch_find(argv[1])) == NULL)
    {
      irc_reply("unsupported exchange: %s", argv[1]);
      return;
    }
  
    if(argc >= 3)
    {
      if((a = account_find(exch, argv[2])) == NULL)
        irc_reply("no balance found for account %s:%s", exch->name, argv[2]);
      else
        irc_reply("bal: %.11f avail: %.11f hold: %.11f", a->balance, a->available, a->hold);
      return;
    }
  }

  for(; x < ACCOUNT_MAX; x++)
  {
    a = &accounts[x];

    if( (a->balance != 0 || a->available != 0 || a->hold != 0) &&
        ( (exch != NULL && exch == a->exch) || exch == NULL ) )
    {
      if(!row++)
      {
        irc_reply("exchange  asset  balance           available         hold");
        irc_reply("--------------------------------------------------------------------");
      }
      irc_reply("%-9.9s %-6.6s %-17.9f %-17.9f %-17.9f",
          a->exch->name, a->currency, a->balance, a->available, a->hold);
    }
  }

  if(!row)
    irc_reply("no accounts with balances found");
}

void
ircu_shlong(const struct market *mkt)
{
  double sell_price = mkt->cint[0].lc->close;
  double sell_fee = (sell_price * mkt->buy_size) * mkt->exch->fee_maker;

  irc_reply("buy_price: %f buy_size: %f buy_funds: %f exp: %d\n",
      mkt->buy_price, mkt->buy_size, mkt->buy_funds, mkt->exposed);

  irc_reply("sell_price: %f sell_size: %f fee_maker: %f sell_fee: %f sell_funds: %f\n",
      sell_price, mkt->buy_size, mkt->exch->fee_maker, sell_fee, sell_price - sell_fee);
}

void
ircu_showLongs(const int argc, const char *args[])
{
  int x = 0, hdr = 0;

  for(; x < MARKET_MAX; x++)
  {
    struct market *mkt = &markets[x];
    char temp[2][50];

    if(!(mkt->state & MKS_ACTIVE) || !(mkt->state & MKS_LONG) || !mkt->exposed)
      continue;

    if(!hdr++)
    {

      irc_reply("id  entry            last             diff    profit  high    low     exp");
      irc_reply("--------------------------------------------------------------------------");
    }

    irc_reply("%02d  %-15s  %-15s  %-6.2f  %-6.2f  %-6.2f  %-6.2f  %d", x,
        trim0(temp[0], 50, mkt->buy_price),
        trim0(temp[1], 50, mkt->cint[0].lc->close),
        (mkt->cint[0].lc->close - mkt->buy_price) / mkt->buy_price * 100,
        mkt->profit_pct, mkt->profit_pct_high, mkt->profit_pct_low,
        mkt->exposed);
  }
  if(!hdr)
    irc_reply("no long markets");
}

void
ircu_showMinprofit(void)
{
  int x = 0, hdr = 0;

  for(; x < MARKET_MAX; x++)
  {
    struct market *mkt = &markets[x];

    if(!(mkt->state & MKS_ACTIVE) || mkt->state & MKS_LEARNING)
      continue;

    if(!hdr)
    {
      hdr = 1;
      irc_reply("## market                  %-6s  %-6s  %-6s  %-6s  %-6s  %-6s  %s",
        cint2str(cint_length(0)), cint2str(cint_length(1)), cint2str(cint_length(2)),
        cint2str(cint_length(3)), cint2str(cint_length(4)), cint2str(cint_length(5)),
        cint2str(cint_length(6)));
      irc_reply("--------------------------------------------------------------------------------");
    }

    irc_reply("%-25s  %-6.2f  %-6.2f  %-6.2f  %-6.2f  %-6.2f  %-6.2f  %.2f",
        mkt->name,
        (mkt->cint[0].tc > HISTSIZE) ? (mkt->minprofit - mkt->cint[0].lc->sma5) / mkt->cint[0].lc->sma5 * 100 : 0,
        (mkt->cint[1].tc > HISTSIZE) ? (mkt->minprofit - mkt->cint[1].lc->sma5) / mkt->cint[1].lc->sma5 * 100 : 0,
        (mkt->cint[2].tc > HISTSIZE) ? (mkt->minprofit - mkt->cint[2].lc->sma5) / mkt->cint[2].lc->sma5 * 100 : 0,
        (mkt->cint[3].tc > HISTSIZE) ? (mkt->minprofit - mkt->cint[3].lc->sma5) / mkt->cint[3].lc->sma5 * 100 : 0,
        (mkt->cint[4].tc > HISTSIZE) ? (mkt->minprofit - mkt->cint[4].lc->sma5) / mkt->cint[4].lc->sma5 * 100 : 0,
        (mkt->cint[5].tc > HISTSIZE) ? (mkt->minprofit - mkt->cint[5].lc->sma5) / mkt->cint[5].lc->sma5 * 100 : 0,
        (mkt->cint[6].tc > HISTSIZE) ? (mkt->minprofit - mkt->cint[6].lc->sma5) / mkt->cint[6].lc->sma5 * 100 : 0);
  }

  if(!hdr)
    irc_reply("no active markets");
}

void
ircu_showMarkets(const struct market *m)
{
  int x = 0, hdr = 0;

  for(; x < MARKET_MAX; x++)
  {
    struct market *mkt = &markets[x];
    char temp[50], temp2[50];

    if(!(markets[x].state & MKS_ACTIVE) || (m != NULL && m != &markets[x]))
      continue;

    if(!hdr++)
    {
      irc_reply("##  market            score  last              1-min   5-min   15-min  30-min  1-hour  4-hour  1-day");
      irc_reply("----------------------------------------------------------------------------------------------------");
    }

    snprintf(temp, 49, "%s:%s-%s", mkt->exch->name, mkt->asset, mkt->currency);

    if(mkt->state & MKS_LEARNING)
      irc_reply("%-02d  %-16s  learning mode: %d of %d candles",
          x, temp, mkt->c_array, mkt->init_candles);
    else
    {
      int y = 0, score = 0;

      for(; y < CINTS_MAX; y++)
        score += market_score(mkt, y);

      irc_reply("%-02d  %-16s  %-5.2f  %-16s  %-6.2f  %-6.2f  %-6.2f  %-6.2f  %-6.2f  %-6.2f  %-6.2f",
          x, temp, ((float)score / (CINT_SCORE_MAX * CINTS_MAX)) * 100,
          trim0(temp2, 49, mkt->cint[0].lc->close),
          (mkt->cint[0].tc) ? mkt->cint[0].lc->asize : 0,
          (mkt->cint[1].tc) ? mkt->cint[1].lc->asize : 0,
          (mkt->cint[2].tc) ? mkt->cint[2].lc->asize : 0,
          (mkt->cint[3].tc) ? mkt->cint[3].lc->asize : 0,
          (mkt->cint[4].tc) ? mkt->cint[4].lc->asize : 0,

          (mkt->cint[4].tc >= 4) ?
            (mkt->cint[4].lc->close - mkt->cint[4].c[HISTSIZE - 4].close)
            / mkt->cint[4].c[HISTSIZE - 4].close * 100 : 0,

          (mkt->cint[4].tc >= 24) ?
            (mkt->cint[4].lc->close - mkt->cint[4].c[HISTSIZE - 24].close)
            / mkt->cint[4].c[HISTSIZE - 24].close * 100 : 0);
    }
  }

  if(!hdr)
    irc_reply("no active markets");
}

void
ircu_showMarket(int argc, const char *argv[])
{
  struct market *mkt = NULL;
  int mid = -1;

  if(argc >= 2)
  {
    if(!strcasecmp(argv[1], "help"))
    {
      irc_reply("usage: show [mar]ket [id] [item]");
      return;
    }

    mid = atoi(argv[1]);

    if((argv[1][0] != '0' && mid == 0) || mid < 0 || mid >= MARKET_MAX)
    {
      irc_reply("unknown market: %s", argv[1]);
      return;
    }

    mkt = &markets[mid];

    if(!(mkt->state & MKS_ACTIVE))
    {
      irc_reply("unknown market: %s", argv[1]);
      return;
    }

    if(argc > 2)
    {
      if(!strncasecmp(argv[2], "long", 4))
      {
        if(mkt->state & MKS_LONG && mkt->exposed)
          ircu_shlong(mkt);
        else
          irc_reply("market #%d is not long", mid);
      }

      // show market x strategy
      else if(!strncasecmp(argv[2], "stra", 4))
      {
        if(mkt->strat == NULL)
          irc_reply("strat: none");
        else
          irc_reply("strat: %s desc: %s msg: %s",
              mkt->strat->name, mkt->strat->desc,
              (mkt->a.msg == NULL ) ? "none" : mkt->a.msg);
      }

      // show market x cint
      else if(!strncasecmp(argv[2], "cint", 4))
      {
        char temp[1000];
        int cint = 0;

        memset((void *)temp, 0, 1000);

        for(; cint < CINTS_MAX; cint++)
        {
          char temp2[50];
          snprintf(temp2, 49, "%s:%d ",
              cint2str(cint_length(cint)), mkt->cint[cint].tc);
          strncat(temp, temp2, 999);
        }
        irc_reply(temp);
      }

      else if(!strncasecmp(argv[2], "can", 3))
      {
        int cint = 0, idx = 0, x = 0;
        char str[1000], *temp;
        struct candle *c;

        if(argc > 3)
        {
          cint = atoi(argv[3]);

          if(cint < 0 || cint >= CINTS_MAX)
          {
            irc_reply("candle interval (cint) must be between 0 and %d",
                CINTS_MAX - 1);
            return;
          }

          if(argc == 5)
            idx = atoi(argv[4]);
        }

        if(idx < 0 || idx >= HISTSIZE)
        {
          irc_reply("candle index must be between 0 and %d", HISTSIZE - 1);
          return;
        }

        if(idx > mkt->cint[cint].tc - 1)
        {
          irc_reply("candle int #%d has only seen %d candles thus far", cint,
          mkt->cint[cint].tc);
          return;
        }

        irc_reply_str(candle_str(&mkt->cint[cint].c[idx], cint_length(cint),
              false));
      }

      else if(!strncasecmp(argv[2], "avg", 3))
      {
        double high = 0, low = 0;
        struct candle_int *ci;
        int x = 0, cint = 0;

        if(argc > 3)
          cint = atoi(argv[3]);

        if(cint < 0 || cint >= CINTS_MAX)
        {
          irc_reply("candle interval (cint) must be between 0 and %d",
              CINTS_MAX - 1);
          return;
        }

        if(mkt->cint[cint].tc < HISTSIZE)
        {
          irc_reply("candle interval #%d (%s) is still learning (%d left)",
              cint, cint2str(mkt->cint[cint].length),
              HISTSIZE - mkt->cint[cint].tc);
          return;
        }

        for(; x < 10; x++)
        {
          struct candle *c;

          ci = &mkt->cint[cint];
          c = &ci->c[HISTSIZE - x - 1];

          if(low == 0 || c->low < low)
            low = c->low;

          if(high < c->high)
            high = c->high;
        }

        irc_reply("ci_high: %.11f L10_high: %.11f d: %.2f ci_low: %.11f L10_low: %.11f d: %.2f sma2: %.11f L10_high_sma2: %.2f L10_low_sma2: %.2f",
            ci->high, high, (high - ci->high) / ci->high * 100,
            ci->low, low, (low - ci->low) / ci->low * 100, ci->lc->sma2,
            (high - ci->lc->sma2) / ci->lc->sma2 * 100,
            (low - ci->lc->sma2) / ci->lc->sma2 * 100);

      }

      /*
      // show market trend
      if(!strncasecmp(argv[2], "trend", 5))
      {
        struct candle_int *ci;
        int cint = 0, x = 0, y;
        double x1 = 0, x2 = 0, x3 = 0;
        double lowest = 0, highest = 0;

        if(argc > 3)
          cint = atoi(argv[3]);

        ci = &mkt->cint[cint];
        y = ci->tc_sma5;

        for(;x < TREND_MAX; x++, y = (y) ? y - 1 : TREND_MAX - 1)
        {
          int z = (y) ? y - 1 : TREND_MAX - 1;
          double diff = 0;

          if(x > 0 && x < 9)
          {
            if(ci->sma5[y].dir == UP)
            {
              x1 += ci->sma5[y].price;
              if(highest == 0 || ci->sma5[y].price > highest)
                highest = ci->sma5[y].price;
            }
            else
            {
              x2 += ci->sma5[y].price;
              if(lowest == 0 || ci->sma5[y].price < lowest)
                lowest = ci->sma5[y].price;
            }
          }

          if(x < TREND_MAX -1)
            diff = (ci->sma5[y].price - ci->sma5[z].price) / ci->sma5[z].price * 100;

          irc_reply("[%d:%d] dir: %s price: %.11f count: %d diff: %.2f", x, y,
              (ci->sma5[y].dir == UP) ? "UP" : "DN", ci->sma5[y].price,
              ci->sma5[y].count, diff);
        }
        x1 /= 4;
        x2 /= 4;
        x3 = (x1 - x2) / x2 * 100;
        irc_reply("high avg: %.11f high: %.11f (int: %.11f) low avg: %.11f low: %.11f (int: %.11f) diff: %.2f",
            x1, highest, ci->high, x2, lowest, ci->low, x3);
      }

      else if(!strncasecmp(argv[2], "highlow", 7) || !strncasecmp(argv[2], "hl", 2))
      {
        struct candle_int *ci;
        int x = 0, y, cint = 0;
        char temp[4][100];
        double high_avg = 0, low_avg = 0, diff;

        if(argc > 3)
          cint = atoi(argv[3]);

        ci = &mkt->cint[cint];
        y = ci->tc_hl;

        for(; x < TREND_MAX; x++, y = (y) ? y - 1 : TREND_MAX - 1)
        {
          struct trend *t = &ci->hl[y];

          diff = (t->high - t->low) / t->low * 100;

          if(x) // exclude first frame that is still in progress
          {
            high_avg += t->high;
            low_avg += t->low;
          }

          if(!x)
            irc_reply("%d: %-15s (%-15s) %-15s (%-15s) diff: %.2f frame: %d", x,
                trim0(temp[0], 99, t->high),
                trim0(temp[1], 99, t->high_sma),
                trim0(temp[2], 99, t->low),
                trim0(temp[3], 99, t->low_sma),
                diff, ci->frame);
          else
            irc_reply("%d: %-15s (%-15s) %-15s (%-15s) diff: %.2f", x,
                trim0(temp[0], 99, t->high),
                trim0(temp[1], 99, t->high_sma),
                trim0(temp[2], 99, t->low),
                trim0(temp[3], 99, t->low_sma),
                diff);
        }

        high_avg /= (TREND_MAX - 1);
        low_avg /= (TREND_MAX - 1);
        diff = (high_avg - low_avg) / low_avg * 100;

        irc_reply("avg high: %s avg low: %s diff: %.2f",
            trim0(temp[0], 99, high_avg), trim0(temp[1], 99, low_avg), diff);

      } */

      // show market settings
      else if(!strncasecmp(argv[2], "set", 3))
      {
        irc_reply("market #%d (%s:%s-%s) settings", mid, mkt->exch->name, mkt->asset, mkt->currency);
        irc_reply("stoploss:%.2f stoploss_adjust:%.2f stoploss_new:%.2f", mkt->u.stoploss, mkt->u.stoploss_adjust, mkt->u.stoploss_new);
        irc_reply("greed:%.2f minprofit:%.2f p_start_cur:%.2f", mkt->u.greed, mkt->u.minprofit, mkt->stats.p_start_cur);

        if(mkt->opt & MKO_TRAILSTOP)
          irc_reply("trailstop:enabled");
        else
          irc_reply("trailstop:disabled");

        if(mkt->opt & MKO_AI)
          irc_reply("ai:enabled");
        else
          irc_reply("ai:disabled");

        if(mkt->opt & MKO_PAPER)
          irc_reply("paper:enabled");
        else
          irc_reply("paper:disabled");

        if(mkt->u.funds > 0)
          irc_reply("funds:%.11f (use this amount for LONG)", mkt->u.funds);
        else
        {
          if(mkt->u.pct > 0)
            irc_reply("pct:%.11f (use this percentage of available bal for LONG)", mkt->u.pct);
          else
            irc_reply("using all available balance for LONGs");
        }

        if(mkt->strat != NULL)
          irc_reply("strategy:%s", mkt->strat->name);
        else
          irc_reply("strategy:none");

        return;
      }
      else
        irc_reply("unknown show item: %s", argv[2]);
    }
    else
      ircu_showMarkets(mkt);
  }
  else
    ircu_showMarkets(NULL);
}

void
ircu_showOrder(int argc, const char *argv[])
{
  struct exchange *exch = NULL;
  int row = 0, x = 0;

  if(argc == 2)
  {
    if((exch = exch_find(argv[1])) == NULL)
    {
      irc_reply("unsupported exchange: %s", argv[1]);
      return;
    }
  }

  for(; x < ORDERS_MAX; x++)
  {
    if((exch != NULL && orders[x].exch != exch) || !orders[x].type)
      continue;

    if(!row++)
    {
      irc_reply("##  exchange  product      state  type   side   price            size        filled");
      irc_reply("---------------------------------------------------------------------------------------");
    }

    irc_reply("%-02d  %-8.8s  %-11.11s  %-5.5s  %-5.5s  %-5.5s  %-15.9f  %-10.5f  %.5f", x,
        orders[x].exch->name,
        orders[x].product,
        order_state(orders[x].state),
        (orders[x].state & ORS_STOP) ? "stop" : order_type(orders[x].type),
        (orders[x].state & ORS_LONG) ? "long" : "short",
        orders[x].price,
        orders[x].size,
        orders[x].filled);
  }

  if(!row)
    irc_reply("no active orders found.");
}

void
ircu_showPerf(int argc, const char *argv[])
{
  int x = 0, hdr = 0, paper = (argc > 1) ? true : false;

  for(; x < MARKET_MAX; x++)
  {
    if(markets[x].state & MKS_ACTIVE)
    {
      struct market *mkt = &markets[x];
      char temp[3][50];

      if(!hdr++)
      {
                // 12  12345678901234567890  12345678901234567890  12345678901234567890  123456  123456  1234  1234  123
        irc_reply("id  market                fees                  profit                pct     trades  wins  loss  wla");
        irc_reply("--------------------------------------------------------------------------------------------------------");
      }

      sprintf(temp[0], "%s:%s-%s", mkt->exch->name, mkt->asset, mkt->currency);

      irc_reply("%-2d  %-20s  %-20s  %-20s  %-6.2f  %-6d  %-4d  %-4d  %.2f",
          x, temp[0],
          trim0(temp[1], 50, (paper == true) ?  mkt->stats.p_fees : mkt->stats.fees),
          trim0(temp[2], 50, (paper == true) ?  mkt->stats.p_profit : mkt->stats.profit),

          (paper == true) ?
            (mkt->stats.p_profit) ? mkt->stats.p_profit / mkt->stats.p_start_cur * 100 : 0 :
            (mkt->stats.profit) ?  mkt->stats.profit / mkt->stats.start_cur * 100 : 0,

          (paper == true) ? mkt->stats.p_trades : mkt->stats.trades,
          (paper == true) ? mkt->stats.p_wins : mkt->stats.wins,
          (paper == true) ? mkt->stats.p_losses : mkt->stats.losses,
          (paper == true) ? (mkt->stats.p_wins) ?
          (mkt->stats.p_wins / (double)(mkt->stats.p_wins + mkt->stats.p_losses)) * 100 : 0 :
          (mkt->stats.wins) ? (mkt->stats.wins / (double)(mkt->stats.wins + mkt->stats.losses)) * 100 : 0);
    } } if(!hdr) irc_reply("no active markets"); }

void
ircu_showProduct(int argc, const char *argv[])
{
  struct exchange *exch;
  struct product *p;

  if(argc >= 2)
  {
    if(!strcasecmp(argv[1], "help"))
      goto ircu_showProductUsage;

    else if((exch = exch_find(argv[1])) == NULL)
    {
      irc_reply("unsupported exchange: %s", argv[1]);
      return;
    }

    if(argc == 4)
    {
      char temp[6][50];

      if((p = exch_findProduct(exch, argv[2], argv[3])) == NULL)
      {
        irc_reply("exchange %s has no such product: %s-%s", exch->name, argv[2], argv[3]);
        return;
      }

      irc_reply("%s:%s-%s status: %s (msg: %s) quote_increment: %s base_increment: %s min_market_funds: %s max_market_funds: %s",
        exch->name, p->asset, p->currency,
        (p->status[0] != '\0') ? p->status : "none",
        (p->status_msg[0] != '\0') ? p->status_msg : "none",
        trim0(temp[2], 50, p->quote_increment),
        trim0(temp[3], 50, p->base_increment),
        trim0(temp[4], 50, p->min_market_funds),
        trim0(temp[5], 50, p->max_market_funds));
      return;
    }

    else
    {
      int x = 0, count = 0;

      irc_reply("asset  currency  ticker            flags");
      irc_reply("------------------------------------------------------------");

      for(; x < PRODUCTS_MAX; x++)
      {
        p = &exch->products[x];

        if(p->asset[0] && p->currency[0])
        {
          char flags[200];
          int y = 0;

          flags[0] = '\0';

          if(p->flags & PFL_CANCELONLY)
          {
            sprintf(flags, "canelonly ");
            y = 12;
          }
          
          if(p->flags & PFL_LIMITONLY)
          {
            sprintf(flags + y, "limitonly ");
            y += 10;
          }

          if(p->flags & PFL_POSTONLY)
          {
            sprintf(flags + y, "postonly ");
            y += 9;
          }

          if(p->flags & PFL_DISABLED)
          {
            sprintf(flags + y, "disabled ");
            y += 9;
          }

          if(p->flags & PFL_STABLE)
            sprintf(flags + y, "stable");

          count++;
          irc_reply("%-5s  %-8s  %-16.11f  %s", p->asset, p->currency, p->ticker.price, flags);
        }
      }
      irc_reply("exchange %s provides %d products", exch->name, count);
      return;
    }
  }

ircu_showProductUsage:
  irc_reply("usage: show [prod]uct exchange [asset currency]");
}

void
ircu_show(void)
{
  char *str = irc[global.is].args;
  const char *argv[20];
  int argc = 0;

  for(; (argv[argc] = strsep(&str, " ")) != NULL && argc < 20; argc++);

  // account balances
  if(!strncasecmp(argv[0], "bal", 3) || !strncasecmp(argv[0], "acc", 3))
    ircu_showAccounts(argc, argv);

  // show longs
  else if(!strncasecmp(argv[0], "long", 4))
    ircu_showLongs(argc, argv);

  // show market
  else if(!strncasecmp(argv[0], "mar", 3))
    ircu_showMarket(argc, argv);

  else if(!strncasecmp(argv[0], "minp", 4))
    ircu_showMinprofit();

  // show order
  else if(!strncasecmp(argv[0], "ord", 3))
    ircu_showOrder(argc, argv);

  // show perf
  else if(!strncasecmp(argv[0], "perf", 4))
    ircu_showPerf(argc, argv);

  // show product
  else if(!strncasecmp(argv[0], "prod", 4))
    ircu_showProduct(argc, argv);

  // show usage
  else
  {
    irc_reply("usage: show item [help]");
    irc_reply("items: [acc]ount, [mar]ket, [ord]er, [perf]ormance, [prod]uct");
  }
}

void
ircu_set(void)
{
  char *str = irc[global.is].args, *argv[20];
  int argc = 0;

  if(irc_admin() == false)
  {
    irc_reply("No. :~(?");
    return;
  }

  for(; (argv[argc] = strsep(&str, " ")) != NULL && argc < 20; argc++);

  if(!strcasecmp(argv[0], "help") || argc == 20)
    goto ircu_setUsage;

  if(!strncasecmp(argv[0], "mar", 3))
  {
    struct market *mkt;
    int mid;

    if(argc < 4)
      goto ircu_setUsage;

    mid = atoi(argv[1]);

    if((argv[1][0] != '0' && mid == 0) || mid < 0 || mid >= MARKET_MAX)
    {
      irc_reply("unknown market");
      return;
    }

    mkt = &markets[mid];

    if(!(mkt->state & MKS_ACTIVE))
    {
      irc_reply("market #%d is not active", mid);
      return;
    }

    // long (resume)
    if(!strncasecmp(argv[2], "long", 4))
    {
      if(mkt->state & MKS_LEARNING)
      {
        irc_reply("market %s is still learning", mkt->name);
        return;
      }

      if(argc == 6)
      {
        double buy_fee, buy_price, buy_size;
        int x =  0;

        buy_price = strtod(argv[3], NULL);
        buy_size = strtod(argv[4], NULL);
        buy_fee = strtod(argv[5], NULL);

        if(buy_price > 0 && buy_size > 0 && buy_fee > 0)
        {
          mkt->buy_time = mkt->cint[0].lc->time;
          mkt->buy_price = buy_price;
          mkt->buy_size = buy_size;
          mkt->buy_funds = (buy_price * buy_size) + buy_fee;

          if(mkt->opt & MKO_PAPER)
          {
            mkt->stats.p_trades++;
            mkt->stats.p_fees += buy_fee;
          }
          else
          {
            mkt->stats.trades++;
            mkt->stats.fees += buy_fee;
          }

          mkt->state |= MKS_LONG;
          mkt->profit = mkt->profit_pct = mkt->profit_pct_high = mkt->profit_pct_low = 0;
          for(; x < CINTS_MAX; x++)
            mkt->cint[x].buy_num = mkt->cint[x].lc->num;

          irc_reply("%s switched posture to LONG (exposed)", mkt->name);
          return;
        }
      }
      irc_reply("usage: set market ## long buy_price buy_size buy_fee");
      return;
    }

    // stoploss
    if(!strncasecmp(argv[2], "stoploss", 4))
    {
      if(argc != 4)
        irc_reply("usage: set market <mid> stoploss value");
      else
      {
        mkt->u.stoploss = strtod(argv[3], NULL);

        if(!mkt->u.stoploss)
          irc_reply("%s stoploss disabled. HODL!", mkt->name);
        else
        {
          if(mkt->u.stoploss > 0)
            mkt->u.stoploss = -3;
          irc_reply("%s stoploss set to %.2f", mkt->name, mkt->u.stoploss);
        }
      }

      return;
    }

    // greed
    if(!strncasecmp(argv[2], "greed", 4))
    {
      if(argc != 4)
        irc_reply("usage: set market <mid> greed value");
      else
      {
        mkt->u.greed = strtod(argv[3], NULL);

        if(!mkt->u.greed)
          irc_reply("market #%d (%s:%s-%s) greed disabled. HODL!", mid, mkt->exch->name, mkt->asset, mkt->currency);
        else
        {
          if(mkt->u.greed < 0)
            mkt->u.greed = 5;
          irc_reply("market #%d (%s:%s-%s) greed set to %.2f", mid, mkt->exch->name, mkt->asset, mkt->currency, mkt->u.greed);
        }
      }

      return;
    }

    // minprofit
    if(!strncasecmp(argv[2], "minprofit", 4))
    {
      if(argc != 4)
        irc_reply("usage: set market <mid> minprofit value");
      else
      {
        mkt->u.minprofit = strtod(argv[3], NULL);

        if(!mkt->u.minprofit)
          irc_reply("market #%d (%s:%s-%s) minprofit disabled. HODL!", mid, mkt->exch->name, mkt->asset, mkt->currency);
        else
        {
          if(mkt->u.minprofit < 0)
            mkt->u.minprofit = 1.5;
          irc_reply("market #%d (%s:%s-%s) minprofit set to %.2f", mid, mkt->exch->name, mkt->asset, mkt->currency, mkt->u.minprofit);
        }
      }

      return;
    }

    // stoploss_adjust
    if(!strcasecmp(argv[2], "stoploss_adjust"))
    {
      if(argc != 4)
        irc_reply("usage: set market <mid> stoploss_adjust value");
      else
      {
        mkt->u.stoploss_adjust = strtod(argv[3], NULL);

        if(!mkt->u.stoploss_adjust)
          irc_reply("market #%d (%s:%s-%s) stoploss_adjust disabled", mid, mkt->exch->name, mkt->asset, mkt->currency);
        else
        {
          if(mkt->u.stoploss_adjust < 0)
            mkt->u.stoploss_adjust = 5;
          irc_reply("market #%d (%s:%s-%s) stoploss_adjust set to %.2f", mid, mkt->exch->name, mkt->asset, mkt->currency, mkt->u.stoploss_adjust);
        }
      }

      return;
    }

    // stoploss_new
    if(!strncasecmp(argv[2], "stoploss_new", 4))
    {
      if(argc != 4)
        irc_reply("usage: set market <mid> stoploss_new value");
      else
      {
        mkt->u.stoploss_new = strtod(argv[3], NULL);

        if(!mkt->u.stoploss_new)
          irc_reply("%s stoploss_new disabled", mkt->name);
        else
        {
          if(mkt->u.stoploss_new > 0 )
            mkt->u.stoploss_new = -2;
          irc_reply("market #%d (%s:%s-%s) stoploss_new set to %.2f", mid, mkt->exch->name, mkt->asset, mkt->currency, mkt->u.stoploss_new);
        }
      }

      return;
    }

    // trade funds
    if(!strncasecmp(argv[2], "fund", 4))
    {
      if(argc != 4)
        irc_reply("usage: set market <mid> funds value");
      else
      {
        char *t = strchr(argv[3], '%');

        if(t != NULL)
        {
          *t = '\0';
          mkt->u.pct = strtod(argv[3], NULL);

          if(mkt->u.pct < 1 || mkt->u.pct > 100)
          {
            irc_reply("percentage must be an integer between 1 and 100");
            return;
          }

          mkt->u.funds = 0;

          if(mkt->u.pct)
            irc_reply("%s will use %d percent of avail balance",
                mkt->name, mkt->u.pct);
          else
            irc_reply("%s will use 100 percent of avail balance", mkt->name);
        }
        else
        {
          mkt->u.funds = strtod(argv[3], NULL);

          mkt->u.pct = 0;

          if(mkt->u.funds)
            irc_reply("%s will use %f %s", mkt->name,
                mkt->u.funds, mkt->currency);
          else
            irc_reply("%s will use 100 percent of avail balance", mkt->name);
        }
      }
      return;
    }

    // trailstop
    if(!strcasecmp(argv[2], "trail"))
    {
      if(argc != 4)
        irc_reply("usage: set market <mid> trail <enable | disable>");
      else
      {
        if(toupper(argv[3][0]) == 'E')
        {
          mkt->opt |= MKO_TRAILSTOP;
          irc_reply("market #%d (%s:%s-%s) trailing stoploss enabled", mid, mkt->exch->name, mkt->asset, mkt->currency);
        }
        else
        {
          mkt->opt &= ~MKO_TRAILSTOP;
          irc_reply("market #%d (%s:%s-%s) trailing stoploss disabled", mid, mkt->exch->name, mkt->asset, mkt->currency);
        }
      }
      return;
    }

    // auto trading (AI)
    if(!strcasecmp(argv[2], "ai"))
    {
      if(argc != 4)
        irc_reply("usage: set market <mid> ai <enable | disable>");
      else
      {
        if(toupper(argv[3][0]) == 'E')
          market_ai(mkt, true);
        else
          market_ai(mkt, false);
      }
      return;
    }

    // disable auto trading after next short
    if(!strcasecmp(argv[2], "disable_after_short"))
    {
      if(argc != 4)
        irc_reply("usage: set market <mid> disable_after_short <enable | disable>");
      else
      {
        if(toupper(argv[3][0]) == 'E')
        {
          mkt->opt |= MKO_NOAIAFTER;
          irc_reply("%s automated trading will be disabled after next short",
              mkt->name);
        }
        else
        {
          mkt->opt &= ~MKO_NOAIAFTER;
          irc_reply("%s automated trading will not be disabled after next short",
              mkt->name);
        }
      }
      return;
    }

    // getout
    if(!strcasecmp(argv[2], "getout"))
    {
      if(argc != 4)
        irc_reply("usage: set market <mid> getout <enable | disable>");
      else
      {
        if(toupper(argv[3][0]) == 'E')
        {
          mkt->opt |= MKO_GETOUT;
          irc_reply("%s get out mode enabled: will short and disable auto trading as soon as profit > 0",
              mkt->name);
        }
        else
        {
          mkt->opt &= ~MKO_GETOUT;
          irc_reply( "%s get out mode disabled", mkt->name);
        }
      }
      return;
    }

    // paper trading
    if(!strcasecmp(argv[2], "paper"))
    {
      if(argc != 4)
        irc_reply("usage: set market <mid> paper <enable | disable>");
      else
      {
        if(toupper(argv[3][0]) == 'E')
        {
          if(!(mkt->opt & MKO_PAPER) && (mkt->state & MKS_LONG))
            irc_reply("%s must exit long position first", mkt->name);
          else
          {
            mkt->opt |= MKO_PAPER;
            irc_reply("%s paper trading only", mkt->name);
          }
        }
        else
        {
          if((mkt->opt & MKO_PAPER) && (mkt->state & MKS_LONG))
          {
            mkt->a.advice = ADVICE_SHORT;
            mkt->a.price = mkt->cint[0].lc->close;
            mkt->a.signal = "exiting paper trading";
            market_sell(mkt);
          }

          mkt->opt &= ~MKO_PAPER;
          irc_reply("%s trades are real!", mkt->name);
        }
      }
      return;
    }

    // strategy
    if(!strncasecmp(argv[2], "strat", 5))
    {
      if(argc != 4)
        irc_reply("usage: set [m]arket id [strat]egy name");

      else
      {
        struct strategy *s = strat_find(argv[3]);

        if(s == NULL)
        {
          irc_reply("unknown strategy: %s", argv[3]);
          return;
        }

        if(mkt->strat != NULL)
          mkt->strat->close(mkt);

        mkt->strat = s;
        s->init(mkt);
        irc_reply("%s strategy %s (%s)", mkt->name, s->name, s->desc);
      }
      return;
    }

    return;
  }

ircu_setUsage:
  irc_reply("usage: set [mar]ket id item value");
}

void
ircu_market(void)
{
  char *str = irc[global.is].args, *argv[20];
  struct market *mkt;
  int argc = 0, mid;

  if(irc_admin() == false)
  {
    irc_reply("No :~(?");
    return;
  }

  for(; (argv[argc] = strsep(&str, " ")) != NULL && argc < 20; argc++);

  if(!strcasecmp(argv[0], "help") || argc == 20)
    goto ircu_marketUsage;

  if(!strcasecmp(argv[0], "add"))
  {
    double start_currency = 0;
    struct exchange *e;
    struct strategy *s;
    struct product *p;
    int mid;

    if(argc != 6)
    {
      irc_reply("usage: market add exchange asset currency strategy start_balance");
      return;
    }

    if((e = exch_find(argv[1])) == NULL)
    {
      irc_reply("unsupported exchange: %s", argv[1]);
      return;
    }

    if(!(e->state & EXS_PUB) || !(e->state & EXS_INIT))
    {
      irc_reply("exchange %s has not yet initialized", e->name);
      return;
    }

    if((p = exch_findProduct(e, argv[2], argv[3])) == NULL)
    {
      irc_reply("exchange %s has no such trading pair %s:%s",
          e->name, argv[2], argv[3]);
      return;
    }

    if(market_find(e, argv[2], argv[3]) != NULL)
    {
      irc_reply("market is already active");
      return;
    }

    if(!strcasecmp(argv[4], "none"))
      s = NULL;
    else if((s = strat_find(argv[4])) == NULL)
    {
      irc_reply("unknown strategy: %s", argv[4]);
      return;
    }

    if((start_currency = strtod(argv[5], NULL)) == 0)
    {
      irc_reply("provide reasonable starting balance for paper trader");
      return;
    }

    if((mid = market_open(e, INITCANDLES, argv[2], argv[3], NULL, s, 0, start_currency, 0)) != -1)
      irc_reply("[%s:%s-%s] market slot #%d initialized with strategy %s (%s)",
          e->name, p->asset, p->currency, mid, (s == NULL) ? "none" : s->name,
          (s == NULL) ? "none" : s->desc);
    else
      irc_reply("[%s:%s-%s] failed to add market (see log)",
          e->name, argv[2], argv[3]);

    return;
  }

  mid = atoi(argv[0]);

  if((argv[0][0] != '0' && mid == 0) || mid < 0 || mid >= MARKET_MAX)
    goto ircu_marketUsage;

  mkt = &markets[mid];

  if(!(mkt->state & MKS_ACTIVE))
  {
    irc_reply("market #%d is not active", mid);
    return;
  }

  if(mkt->state & MKS_LEARNING)
  {
    irc_reply("market #%d is still learning", mid);
    return;
  }

  if(!strcasecmp(argv[1], "buy"))
  {
    float pct = 0;

    if(mkt->state & MKS_LONG)
    {
      irc_reply("market %s:%s:%s is already long", mkt->exch->name, mkt->asset, mkt->currency);
      return;
    }

    if(argv[2] != NULL)
    {
      pct = strtod(argv[2], NULL);

      if(!pct || pct < 0 || pct >100)
      {
        irc_reply("percentage must be a number greater than 0 and equal to or less than 100");
        return;
      }

      if(pct == 100) // the default is all-in anyway
        pct = 0;
    }

    mkt->a.advice = ADVICE_LONG;
    mkt->a.signal = "user forced";
    mkt->a.pct = pct;

    irc_reply("[%s:%s-%s] automatic trading disabled: forced long",
        mkt->exch->name, mkt->asset, mkt->currency);

    mkt->opt &= ~MKO_AI;

    market_buy(mkt);
    return;
  }

  if(!strcasecmp(argv[1], "sell"))
  {
    float pct = 0;

    if(!(mkt->state & MKS_LONG))
    {
      irc_reply("[%s:%s-%s] already short",
          mkt->exch->name, mkt->asset, mkt->currency);
      return;
    }

    if(argv[2] != NULL)
    {
      pct = strtod(argv[2], NULL);

      if(!pct || pct < 0 || pct > 100)
      {
        irc_reply("percentage must be a number greater than 0 and equal to or less than 100");
        return;
      }

      if(pct == 100) // the default is all-in anyway
        pct = 0;
    }

    irc_reply("[%s:%s-%s] automatic trading disabled: forced short",
        mkt->exch->name, mkt->asset, mkt->currency);

    mkt->opt &= ~MKO_AI;

    mkt->a.advice = ADVICE_SHORT;
    mkt->a.signal = "user forced";
    mkt->a.pct = pct;

    market_sell(mkt);
    return;
  }

  if(!strcasecmp(argv[1], "close"))
  {
    if(mkt->opt & MKO_PAPER && mkt->state & MKS_LONG)
    {
      mkt->a.advice = ADVICE_SHORT;
      mkt->a.signal = "user forced";
      mkt->a.pct = 0;
      market_sell(mkt);
    }

    irc_reply("[%s:%s-%s] market closed",
        mkt->exch->name, mkt->asset, mkt->currency);
    market_close(mkt);
    return;
  }

ircu_marketUsage:
  irc_reply("market <id> <command> [argv]");
  irc_reply("market add exchange asset currency strategy");
}

void
ircu_order(void)
{
  char *str = irc[global.is].args, *argv[20];
  int x = 0, oid;

  for(; (argv[x] = strsep(&str, " ")) != NULL && x < 20; x++);

  if(!strcasecmp(argv[0], "help"))
    goto ircu_orderUsage;

  if(!strcasecmp(argv[0], "cancel"))
  {
    if(x != 2)
      goto ircu_orderUsage;

    oid = atoi(argv[1]);
    
    if(oid < 0 || oid > ORDERS_MAX)
      goto ircu_orderUsage;

    if(!orders[oid].state)
    {
      irc_reply("no such order");
      return;
    }

    exch_orderCancel(&orders[oid]);
    return;
  }

  else if(!strcasecmp(argv[0], "create"))
  {
    struct exchange *exch;
    struct order o;

    if(x < 8)
      goto ircu_orderUsage;

    exch = exch_find(argv[1]);

    if(exch == NULL)
    {
      irc_reply("unsupported exchange: %s:", argv[1]);
      return;
    }

    if((oid = order_findfree()) == -1)
    {
      irc_reply("maximum orders reached");
      return;
    }

    memset((void *)&o, 0, sizeof(struct order));

    o.exch = exch;
    o.array = oid;
    o.change = o.create = global.now;
    o.state = ORS_PENDING;

    if(!strcasecmp(argv[2], "buy") || !strcasecmp(argv[2], "long"))
      o.state |= ORS_LONG;
    else if(strcasecmp(argv[2], "sell") && strcasecmp(argv[2], "short"))
      goto ircu_orderUsage;

    if(argv[3] != NULL && argv[4] != NULL)
    {
      struct product *p = exch_findProduct(o.exch, argv[3], argv[4]);

      if(p == NULL)
      {
        irc_reply("exchange %s has no such product: %s-%s", exch->name, argv[3], argv[4]);
        return;
      }

      // TODO: maybe make product a "struct product *" ?
      snprintf(o.product, 99, "%s-%s", argv[3], argv[4]);
      mkupper(o.product);
    }
    else
      goto ircu_orderUsage;

    if(!strcasecmp(argv[5], "stop"))
    {
      if(x != 9)
        goto ircu_orderUsage;

      o.type = ORT_LIMIT;
      o.state |= ORS_STOP;

      if(!(o.stop_price = strtod(argv[6], NULL)))
        goto ircu_orderUsage;

      if(!(o.price = strtod(argv[7], NULL)))
        goto ircu_orderUsage;

      if(!(o.size = strtod(argv[8], NULL)))
        goto ircu_orderUsage;
    }

    else if(!strcasecmp(argv[5], "limit"))
    {
      o.type = ORT_LIMIT;
      if(!(o.price = strtod(argv[6], NULL)))
        goto ircu_orderUsage;
      if(!(o.size = strtod(argv[7], NULL)))
        goto ircu_orderUsage;
    }

    else if(!strcasecmp(argv[5], "market"))
    {
      o.type = ORT_MARKET;
      if(!strcasecmp(argv[6], "funds"))
      {
        if(!(o.funds = strtod(argv[7], NULL)))
          goto ircu_orderUsage;
      }
      else if(!strcasecmp(argv[6], "size"))
      {
        if(!(o.size = strtod(argv[7], NULL)))
          goto ircu_orderUsage;
      }
      else
        goto ircu_orderUsage;
    }
    else
      goto ircu_orderUsage;

    memcpy((void *)&orders[oid], (void *)&o, sizeof(struct order));
    exch_order(&orders[oid]);
  }

  return;

ircu_orderUsage:
  irc_reply("usage: create exchange <buy | sell> asset currency limit price size");
  irc_reply("usage: create exchange <buy | sell> asset currency stop stopPrice price size");
  irc_reply("usage: create exchange <buy | sell> asset currency market <funds quantity | size quantity>");
  irc_reply("usage: cancel id");
}
