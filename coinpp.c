#define WM_COINPPC
#include "coinpp.h"

void
coinpp_callback_tickers(const int x)
{
  char *result = cq[x].recv;
  json_object *ticker_json;
  enum json_tokener_error jerr = json_tokener_success;

  if(cq[x].recv == NULL || cq[x].state == CQS_FAIL)
  {
    logger(C_COINPP, WARN, "coinpp_callback_tickers", "poll failed - interval will have no data.");
    return;
  }

  ticker_json = json_tokener_parse_verbose(result, &jerr);

  if(jerr == json_tokener_success && json_object_is_type(ticker_json, json_type_array) == true)
  {
    int x = 0, num = json_object_array_length(ticker_json);

    while(x < num && x < MAX_CRYPTOS)
    {
      json_object *tmp;
      const char *str;

      if((tmp = json_object_array_get_idx(ticker_json, x)))
      {
        int rank, idx;

        if((str = json_object_get_string(json_object_object_get(tmp, "rank"))) != NULL)
          rank = atoi(str);
        else
          goto coinpp_callback_tickersFail;

        if(rank > 0)
        {
          idx = rank - 1;
          cryptos[idx].rank = rank;
        }
        else
        {
          x++;
          continue;
        }

        if((str = json_object_get_string(json_object_object_get(tmp, "name"))) != NULL)
          strncpy(cryptos[idx].name, str, 49);
        else
          goto coinpp_callback_tickersFail;

        if((str = json_object_get_string(json_object_object_get(tmp, "id"))) != NULL)
          strncpy(cryptos[idx].id, str, 49);
        else
          goto coinpp_callback_tickersFail;

        if((str = json_object_get_string(json_object_object_get(tmp, "symbol"))) != NULL)
          strncpy(cryptos[idx].sym, str, 19);
        else
          goto coinpp_callback_tickersFail;

        if((tmp = json_object_object_get(tmp, "quotes")) == NULL)
          goto coinpp_callback_tickersFail;

        if((tmp = json_object_object_get(tmp, "USD")) == NULL)
          goto coinpp_callback_tickersFail;

        if((str = json_object_get_string(json_object_object_get(tmp, "price"))) != NULL)
          cryptos[idx].price = atof(str);
        else
          goto coinpp_callback_tickersFail;

        if((str = json_object_get_string(json_object_object_get(tmp, "volume_24h"))) != NULL)
          cryptos[idx].vol = atof(str);
        else
          goto coinpp_callback_tickersFail;

        if((str = json_object_get_string(json_object_object_get(tmp, "volume_24h_change_24h"))) != NULL)
          cryptos[idx].vol_diff = atof(str);
        else
          goto coinpp_callback_tickersFail;

        if((str = json_object_get_string(json_object_object_get(tmp, "ath_price"))) != NULL)
          cryptos[idx].ath = atof(str);
        else
          cryptos[idx].ath = 0;

        if((str = json_object_get_string(json_object_object_get(tmp, "percent_from_price_ath"))) != NULL)
          cryptos[idx].ath_diff = atof(str);
        else
          cryptos[idx].ath_diff =0;

        if((str = json_object_get_string(json_object_object_get(tmp, "market_cap"))) != NULL)
          cryptos[idx].mcap = atof(str);
        else
          goto coinpp_callback_tickersFail;

        if((str = json_object_get_string(json_object_object_get(tmp, "market_cap_change_24h"))) != NULL)
          cryptos[idx].mcap_diff = atof(str);
        else
          goto coinpp_callback_tickersFail;

        if((str = json_object_get_string(json_object_object_get(tmp, "percent_change_15m"))) != NULL)
          cryptos[idx].change_15m = atof(str);
        else
          goto coinpp_callback_tickersFail;

        if((str = json_object_get_string(json_object_object_get(tmp, "percent_change_1h"))) != NULL)
          cryptos[idx].change_1h = atof(str);
        else
          goto coinpp_callback_tickersFail;

        if((str = json_object_get_string(json_object_object_get(tmp, "percent_change_24h"))) != NULL)
          cryptos[idx].change_24h = atof(str);
        else
          goto coinpp_callback_tickersFail;

        if((str = json_object_get_string(json_object_object_get(tmp, "percent_change_7d"))) != NULL)
          cryptos[idx].change_7d = atof(str);
        else
          goto coinpp_callback_tickersFail;

        if((str = json_object_get_string(json_object_object_get(tmp, "percent_change_30d"))) != NULL)
          cryptos[idx].change_30d = atof(str);
        else
          goto coinpp_callback_tickersFail;

        if((str = json_object_get_string(json_object_object_get(tmp, "percent_change_1y"))) != NULL)
          cryptos[idx].change_1y = atof(str);
        else
          goto coinpp_callback_tickersFail;

        x++;
      }
    }

    if(x > 0)
    {
      json_object_put(ticker_json);
      logger(C_COINPP, DEBUG1, "coinpp_callback_tickers", "successfully loaded %d cryptos", x);
      global.num_cryptos = x;
      return;
    }
  }

coinpp_callback_tickersFail:
  if(ticker_json != NULL)
    json_object_put(ticker_json);
  logger(C_COINPP, CRIT, "coinpp_callback_tickers", "api error");
}

void
coinpp_callback_global(const int x)
{
  char *result = cq[x].recv;
  json_object *mcap_json;
  enum json_tokener_error jerr = json_tokener_success;

  if(cq[x].recv == NULL || cq[x].state == CQS_FAIL)
  {
    logger(C_COINPP, WARN, "coinpp_callback_global", "poll failed - interval will have no data.");
    return;
  }

  mcap_json = json_tokener_parse_verbose(result, &jerr);

  if(jerr == json_tokener_success)
  {
    const char *str;

    if((str = json_object_get_string(json_object_object_get(mcap_json, "market_cap_usd"))) != NULL)
      crypto_global.mcap = atof(str);
    else
      goto coinpp_callback_globalFail;

    if((str = json_object_get_string(json_object_object_get(mcap_json, "volume_24h_usd"))) != NULL)
      crypto_global.vol = atof(str);
    else
      goto coinpp_callback_globalFail;

    if((str = json_object_get_string(json_object_object_get(mcap_json, "bitcoin_dominance_percentage"))) != NULL)
      crypto_global.btc_dom = atof(str);
    else
      goto coinpp_callback_globalFail;

    if((str = json_object_get_string(json_object_object_get(mcap_json, "cryptocurrencies_number"))) != NULL)
      crypto_global.assets = atoi(str);
    else
      goto coinpp_callback_globalFail;

    if((str = json_object_get_string(json_object_object_get(mcap_json, "market_cap_ath_value"))) != NULL)
      crypto_global.mcap_ath = atof(str);
    else
      goto coinpp_callback_globalFail;

    if((str = json_object_get_string(json_object_object_get(mcap_json, "volume_24h_ath_value"))) != NULL)
      crypto_global.vol_ath = atof(str);
    else
      goto coinpp_callback_globalFail;

    if((str = json_object_get_string(json_object_object_get(mcap_json, "market_cap_change_24h"))) != NULL)
      crypto_global.mcap_24h = atof(str);
    else
      goto coinpp_callback_globalFail;

    if((str = json_object_get_string(json_object_object_get(mcap_json, "volume_24h_change_24h"))) != NULL)
      crypto_global.vol_24h = atof(str);
    else
      goto coinpp_callback_globalFail;

    json_object_put(mcap_json);
    logger(C_COINPP, DEBUG1, "coinpp_callback_global", "parsed json for '%s'", COINPP_GLOBAL_API);
    curl_request(CRT_GET, 0, COINPP_TICKERS_API, NULL, NULL, coinpp_callback_tickers, NULL, NULL);
    return;
  }

coinpp_callback_globalFail:
  if(mcap_json != NULL)
    json_object_put(mcap_json);
  logger(C_COINPP, CRIT, "coinpp_callback_global", "api error");
}

void
coinpp_init(void)
{
  memset((void *)cryptos, 0, sizeof(struct crypto) * MAX_CRYPTOS);
  memset((void *)cryptos_prev, 0, sizeof(struct crypto) * MAX_CRYPTOS);
  memset((void *)&crypto_global, 0, sizeof(struct crypto_global_market));
  logger(C_ANY, INFO, "coinpp_init", "coinpaprika.com api initialized");
  return;
}

void
coinpp_hook(void)
{
  if(global.now - global.cpptimer >= COINPP_INTERVAL)
  {
    global.cpptimer = global.now;
    logger(C_COINPP, DEBUG2, "coinpp_hook", "adding curl requests");
    curl_request(CRT_GET, 0, COINPP_GLOBAL_API, NULL, NULL, coinpp_callback_global, NULL, NULL);
  }
}
