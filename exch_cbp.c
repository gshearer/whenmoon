// CoinbasePro API support for whenmoon
// george@shearer.tech 
// -------------------------------------------------------------------------------------------

#define WM_EXCH_CBP
#include "exch_cbp.h"

// This function updates the headers prior to a request to CBP's authenticated
// endpoints which include the hash of the headers, url, and body.
void
cbp_updatehash(const char *type, const char *endpoint, const char *body)
{
  char b64_encoded[1000], data[1000];
  unsigned char md[1000];
  unsigned int md_len;

  if(body == NULL)
    snprintf(data, 999, "%lu%s/%s", global.now, type, endpoint);
  else
    snprintf(data, 999, "%lu%s/%s%s", global.now, type, endpoint, body);

  HMAC(EVP_sha256(), cbp.decsec, cbp.decsec_len, (unsigned char *)data, strlen(data), md, &md_len);

  memset((void *)b64_encoded, 0, 1000);
  base64_encode(b64_encoded, md, md_len);

  snprintf(cbp.head[1], 2999, "CB-ACCESS-SIGN: %s", b64_encoded);
  snprintf(cbp.head[2], 2999, "CB-ACCESS-TIMESTAMP: %lu", global.now);
}

struct json_object *
cbp_json_parse(const int creq)
{
  enum json_tokener_error jerr = json_tokener_success;
  struct json_object *j;

  if(cq[creq].state == CQS_FAIL)
  {
    logger(C_EXCH, CRIT, "cbp_json_parse", "curl request failed with msg: %s",
        cq[creq].errmsg);
    return(NULL);
  }

  j = json_tokener_parse_verbose(cq[creq].recv, &jerr);

  if(jerr != json_tokener_success)
  {
    logger(C_EXCH, CRIT, "cbp_json_parse", "error parsing json");
    logger(C_EXCH, DEBUG2, "cbp_json_parse", "payload: >%s<", cq[creq].recv);
    return(NULL);
  }

  if(!json_object_is_type(j, json_type_array))
  {
    const char *err_msg = json_object_get_string(json_object_object_get(j, "message"));

    if(err_msg != NULL)
    {
      logger(C_EXCH, CRIT, "cbp_json_parse", "api error: %s", err_msg);
      json_object_put(j);
      return(NULL);
    }
  }
  return(j);
}

// called by curl servicer on completion of GET to /candles endpoint
void
cbp_callback_candles(const int creq)
{
  struct market *mkt = (struct market *)cq[creq].userdata;
  int array_size, x = 0;
  json_object *j;

  if(mkt->state == 0)
  {
    logger(C_EXCH, WARN, "cbp_callback_candles",
        "ignoring req for closed market");
    return;
  }

  if((j = cbp_json_parse(creq)) == NULL)
  {
    logger(C_EXCH, CRIT, "cbp_callback_candles", "api error");
    exch_endpoll_data(FAIL, mkt, 0);
    return;
  }

  if(!json_object_is_type(j, json_type_array))
  {
    const char *err_msg = json_object_get_string(json_object_object_get(j, "message"));

    if(err_msg != NULL)
      logger(C_EXCH, CRIT, "cbp_callback_candles", "api error: %s", err_msg);
    else
      logger(C_EXCH, CRIT, "cbp_callback_candles", "api error (returned object is not an array)");
    json_object_put(j);
    exch_endpoll_data(FAIL, mkt, 0);
    return;
  }

  array_size = json_object_array_length(j);

  for(; x < array_size; x++)
  {
    json_object *j2;

    if((j2 = json_object_array_get_idx(j, x)))
    {
      int array2_size, y = 0;
      struct candle c;
      const char *ele[6];

      if(!json_object_is_type(j2, json_type_array))
      {
        const char *err_msg = json_object_get_string(json_object_object_get(j, "message"));

        if(err_msg != NULL)
          logger(C_EXCH, CRIT, "cbp_callback_candles", "api error: %s", err_msg);
        else
          logger(C_EXCH, CRIT, "cbp_callback_candles", "api error (inner object is not an array)");
        json_object_put(j);
        exch_endpoll_data(FAIL, mkt, 0);
        return;
      }

      array2_size = json_object_array_length(j2);

      while(y < array2_size && y < 6) 
      {
        json_object *tmp = json_object_array_get_idx(j2, y);
        ele[y++] = json_object_get_string(tmp);
      }

      memset((void *)&c, 0, sizeof(struct candle));

      // API candle schema
      // timestamp, price_low, price_high, price_open, price_close]

      c.time   = atol(ele[0]);
      c.low    = strtod(ele[1], NULL);
      c.high   = strtod(ele[2], NULL);
      c.open   = strtod(ele[3], NULL);
      c.close  = strtod(ele[4], NULL);
      c.volume = strtod(ele[5], NULL);

      logger(C_EXCH, DEBUG4, "cbp_callback_candles", "parsed candle: %lu %0.9f", c.time, c.close);

      if(exch_candle(mkt, &c) == FAIL)
        break;
    }
  }
  json_object_put(j);
  logger(C_EXCH, DEBUG2, "cbp_callback_candles", "downloaded %d candles", array_size);
  exch_endpoll_data(SUCCESS, mkt, array_size);
}

// called by curl servicer on completion of GET to /trades endpoint
void
cbp_callback_trades(const int creq)
{
  struct market *mkt = (struct market *)cq[creq].userdata;
  struct cbp_pmv *pmv = (struct cbp_pmv *)mkt->exchdata;
  enum json_tokener_error jerr = json_tokener_success;
  int array_size, x = 0;
  json_object *j;

  if(cq[creq].state == CQS_FAIL)
  {
    logger(C_EXCH, CRIT, "cbp_callback_trades", "curl failed with msg=%s\n", cq[creq].errmsg);
    return;
  }

  if(mkt->state == 0)
  {
    logger(C_EXCH, WARN, "cbp_callback_trades",
        "ignoring req for closed market");
    return;
  }

  if(cq[creq].result[0][0] != '\0') /* cbp pagnation cb-after */
    pmv->restpage = atol(cq[creq].result[0]);

  j = json_tokener_parse_verbose(cq[creq].recv, &jerr);

  if(jerr != json_tokener_success)
  {
    logger(C_EXCH, CRIT, "cbp_callback_trades", "api error (json parsing)\n");
    exch_endpoll_data(FAIL, mkt, 0);
    return;
  }

  if(!json_object_is_type(j, json_type_array))
  {
    const char *err_msg = json_object_get_string(json_object_object_get(j, "message"));

    if(err_msg != NULL)
      logger(C_EXCH, CRIT, "cbp_callback_trades", "api error: %s", err_msg);
    else
      logger(C_EXCH, CRIT, "cbp_callback_trades", "api error (returned object is not an array)");
    json_object_put(j);
    exch_endpoll_data(FAIL, mkt, 0);
    return;
  }

  array_size = json_object_array_length(j);

  for(; x < array_size; x++)
  {
    json_object *tmp;

    if((tmp = json_object_array_get_idx(j, x)))
    {
      const char *str;
      struct tm tm_ttime;
      struct trade t;

      memset((void *)&tm_ttime, 0, sizeof(struct tm));
      memset((void *)&t, 0, sizeof(struct trade));

      // TODO: error checking :-)
      str = json_object_get_string(json_object_object_get(tmp, "time"));
      strptime(str, "%Y-%m-%dT%H:%M:%SZ", &tm_ttime);
      t.time = timegm(&tm_ttime);

      str = json_object_get_string(json_object_object_get(tmp, "trade_id"));
      t.id = atol(str);

      str = json_object_get_string(json_object_object_get(tmp, "price"));
      t.price = strtod(str, NULL);

      str = json_object_get_string(json_object_object_get(tmp, "size"));
      t.size = strtod(str, NULL);

      str = json_object_get_string(json_object_object_get(tmp, "side"));
      t.side = (strncasecmp(str, "buy", 3)) ? 1 : 0;

      if(exch_trade(mkt, &t) == FAIL)
      {
        x++;
        break;
      }
    }
    else
    {
      logger(C_EXCH, CRIT, "cbp_callback_trades", "api error (getting array idx #%d)", x);
      exch_endpoll_data(FAIL, mkt, 0);
      break;
    }
  }
  json_object_put(j);
  logger(C_EXCH, DEBUG4, "cbp_callback_trades", "received %d trades (%s vs %s)", x, mkt->asset, mkt->currency);

  exch_endpoll_data(SUCCESS, mkt, array_size); // notify exchange framework that we're done processing last API get
}

// called when exchange responses after request to cancel order
void
cbp_callback_orderCancel(const int creq)
{
  struct curl_queue *cr = &cq[creq];
  struct order *o = (struct order *)cr->userdata;

  if(cr->state & CQS_SUCCESS)
  {
    if(cr->http_rc == 200)
    {
      o->state &= ~ORS_DELETING;
      exch_endpoll_orderCancel(o);
      return;
    }

    else // JSON
    {
      enum json_tokener_error jerr = json_tokener_success;
      json_object *payload_json = json_tokener_parse_verbose(cr->recv, &jerr);

      if(jerr == json_tokener_success)
      {
        const char *str;

        if((str = json_object_get_string(json_object_object_get(payload_json, "message"))) != NULL)
        {
          logger(C_EXCH, WARN, "cbp_callback_orderCancel", "failed to cancel order: %s", str);
          strncpy(o->msg, str, 99);
          // note we dont remove deleting flag, this means failed
          exch_endpoll_orderCancel(o);
          return;
        }
      }
      json_object_put(payload_json);
    }
  }
  logger(C_EXCH, CRIT, "cbp_callback_orderCancel", "api error");
  strcpy(o->msg, "api error");
  exch_endpoll_orderCancel(o);
}

// called when the exchange responds with results from our order submission
void
cbp_callback_orderPost(const int creq)
{
  struct curl_queue *cr = &cq[creq];
  struct order *o = (struct order *)cr->userdata;

  if(cr->state & CQS_SUCCESS) // libcurl succeeded, parse json response
  {
    enum json_tokener_error jerr = json_tokener_success;
    json_object *payload_json;

    payload_json = json_tokener_parse_verbose(cr->recv, &jerr);

    if(jerr == json_tokener_success)
    {
      const char *str;

      if(cr->http_rc == 200) // server has confirmed our order
      {
        if((str = json_object_get_string(json_object_object_get(payload_json, "id"))) != NULL)
          strncpy(o->id, str, 99);
        else
          goto cbp_callback_orderFail;

        if((str = json_object_get_string(json_object_object_get(payload_json, "status"))) != NULL)
        {
          o->state &= ~ORS_PENDING; // order is confirmed at this point

          if(!strcasecmp(str, "open") || !strcasecmp(str, "pending"))
            o->state |= ORS_OPEN;

          else if(!strcasecmp(str, "closed") || !strcasecmp(str, "done"))
          {
            o->state |= ORS_CLOSED;
            if((str = json_object_get_string(json_object_object_get(payload_json, "fill_fees"))) != NULL)
              o->fee = strtod(str, NULL);
            else
              goto cbp_callback_orderFail;

            if((str = json_object_get_string(json_object_object_get(payload_json, "filled_size"))) != NULL)
              o->filled = strtod(str, NULL);
            else
              goto cbp_callback_orderFail;

          }
          else
            goto cbp_callback_orderFail;

          exch_endpoll_orderPost(o);
          return;
        }
      }

      else // there was a problem :(
      {
        if((str = json_object_get_string(json_object_object_get(payload_json, "message"))) != NULL)
        {
          strncpy(o->msg, str, 99);
          o->state &= ~ORS_PENDING;
          o->state |= ORS_FAILED;
          json_object_put(payload_json);
          exch_endpoll_orderPost(o);
          return;
        }
      }
    }
    json_object_put(payload_json);
  }

cbp_callback_orderFail:
  o->state &= ORS_PENDING;
  o->state |= ORS_FAILED;
  strcpy(o->msg, "api error");
  logger(C_EXCH, CRIT, "cbp_callback_order", "api error");
  exch_endpoll_orderPost(o);
}

// string order:
// ---- never null -----
// 0 == exchange's order id
// 1 == ASSET-CURRENCY
// 2 == side (buy or sell)
// 3 == type
// 4 == create time
// 5 == status
// -------- may be null ----
// 6 == price
// 7 == size
// 8 == fee
// 9 == filled
// 10 == stop
// 11 == stop_price

int
cbp_parseOrder(struct order *o, const char *vals[])
{
  struct tm tm_temp;

  memset((void *)&tm_temp, 0, sizeof(struct tm));
  memset((void *)o, 0, sizeof(struct order));

  o->array = -1;
  o->exch = cbp.exch;
  o->state = ORS_UPDATE; // this is an incoming update

  strncpy(o->id, vals[0], 99);
  strncpy(o->product, vals[1], 99);

  if(strncasecmp(vals[2], "sell", 4))
    o->state |= ORS_LONG;

  if(!strncasecmp(vals[3], "limit", 5))
    o->type = ORT_LIMIT;

  else if(!strncasecmp(vals[3], "market", 6))
    o->type = ORT_MARKET;

  else
  {
    logger(C_EXCH, CRIT, "cbp_parseOrder", "unknown order type: %s", vals[3]);
    return(FAIL);
  }

  strptime(vals[4], "%Y-%m-%dT%H:%M:%SZ", &tm_temp);
  o->create = timegm(&tm_temp);

  if(!strncasecmp(vals[5], "open", 4) || !strncasecmp(vals[5], "pending", 7) || !strcasecmp(vals[5], "active"))
  {
    o->state |= ORS_OPEN;
    if(!strcasecmp(vals[5], "active"))
      o->state |= ORS_STOP;
  }
  else if(!strncasecmp(vals[5], "closed", 6) || !strncasecmp(vals[5], "done", 4))
    o->state |= ORS_CLOSED;
  else
  {
    logger(C_EXCH, CRIT, "cbp_parseOrder", "unknown order status: %s", vals[5]);
    return(FAIL);
  }

  if(vals[6] != NULL)
    o->price = strtod(vals[6], NULL);
  if(vals[7] != NULL)
    o->size = strtod(vals[7], NULL);
  if(vals[8] != NULL)
    o->fee = strtod(vals[8], NULL);
  if(vals[9] != NULL)
    o->filled = strtod(vals[9], NULL);

  // coinbase doesnt seem to bother to send these even though api docs say so
  if(vals[10] != NULL)
  {
    o->state |= ORS_STOP;
    if(vals[11] != NULL)
      o->stop_price = strtod(vals[11], NULL);
  }
  return(SUCCESS);
}

void
cbp_callback_getOrder(const int creq)
{
  struct curl_queue *cr = &cq[creq];

  if(cr->state & CQS_SUCCESS)
  {
    enum json_tokener_error jerr = json_tokener_success;
    json_object *payload_json = json_tokener_parse_verbose(cr->recv, &jerr);

    if(cr->http_rc == 200)
    {
      // {
      // "id":"dd9686ef-d5c8-437d-af1e-9df6b2ca17e0",
      // "price":"4130.00000000",
      // "size":"0.00500000",
      // "product_id":"ETH-USD",
      // "profile_id":"3511edd9-25c2-4134-b72a-116b997708e6",
      // "side":"buy",
      // "type":"limit",
      // "time_in_force":"GTC",
      // "post_only":false,
      // "created_at":"2021-12-13T00:17:25.57541Z",
      // "done_at":"2021-12-13T00:17:50.869933Z",
      // "done_reason":"filled",
      // "fill_fees":"0.0165200000000000",
      // "filled_size":"0.00500000",
      // "executed_value":"20.6500000000000000",
      // "status":"done",
      // "settled":true
      // }

      if(jerr == json_tokener_success)
      {
        const char *str, *keys[12] = { "id", "product_id", "side", "type", "created_at", "status", "price", "size", "fill_fees", "filled_size", "stop", "stop_price" };
        const char *values[12] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
        int y = 0, mandatory = 6;
        struct order o;

        for(; y < 12; y++)
        {
          if((str = json_object_get_string(json_object_object_get(payload_json, keys[y]))) != NULL)
            values[y] = str;
          else if(y < mandatory)
          {
            logger(C_EXCH, WARN, "cbp_callback_getOrder", "missing mandatory element");
            goto cbp_callback_getOrderFail;
          }
        }

        logger(C_EXCH, DEBUG3, "cbp_callback_getOrder", "single order update on %s: %s", cbp.exch->name, values[0]);
        if(cbp_parseOrder(&o, values) == SUCCESS)
          exch_orderUpdate(&o);
        json_object_put(payload_json);
        exch_endpoll_getOrder(cbp.exch, SUCCESS);
        return;
      }
      else
        logger(C_EXCH, WARN, "cbp_callback_getOrder", "json parsing error");
    }

    else if(cr->http_rc == 404)
    {
      logger(C_EXCH, WARN, "cbp_callback_getOrder", "order canceled with no fills");
      json_object_put(payload_json);
      exch_endpoll_getOrder(cbp.exch, SUCCESS);
      return;
    }
    else
      logger(C_EXCH, WARN, "cbp_callack_getOrder", "http response code %d: %s", cr->http_rc, (cr->recv != NULL) ? cr->recv : "no message");
  }

cbp_callback_getOrderFail:
  exch_endpoll_getOrder(cbp.exch, FAIL);
}

void
cbp_callback_getOrders(const int creq)
{
  json_object *j = NULL;

  if((j = cbp_json_parse(creq)) == NULL)
    goto cbp_callback_getOrdersFail;

  if(json_object_is_type(j, json_type_array) == true)
  {
    int x = 0, num = json_object_array_length(j);

    // * = required
    // *"id":"d0b07c51-f217-423f-9131-0f0e0416da93",
    // *"price":"0.65000000",
    // "size":"10.00000000",
    // *"product_id":"GALA-USD",
    // "profile_id":"3511edd9-25c2-4134-b72a-116b997708e6",
    // *"side":"buy",
    // *"type":"limit",
    // "time_in_force":"GTC",
    // "post_only":false,
    // *"created_at":"2021-12-12T18:45:46.078721Z",
    // "fill_fees":"0.0000000000000000",
    // "filled_size":"0.00000000",
    // "executed_value":"0.0000000000000000",
    // *"status":"active",
    // "settled":false

    while(x < num)
    {
      json_object *tmp;

      if((tmp = json_object_array_get_idx(j, x)))
      {
        const char *str, *keys[12] = {
          "id", "product_id", "side", "type", "created_at", "status", "price", "size",
          "fill_fees", "filled_size", "stop", "stop_price" };
        const char *values[12] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
          NULL, NULL, NULL };

        int y = 0, mandatory = 6;
        struct order o;

        for(; y < 12; y++)
        {
          if((str = json_object_get_string(json_object_object_get(tmp, keys[y]))) != NULL)
            values[y] = str;
          else if(y < mandatory)
            goto cbp_callback_getOrdersFail;
        }
        if(cbp_parseOrder(&o, values) == SUCCESS)
          exch_orderUpdate(&o);
      }
      else
        goto cbp_callback_getOrdersFail;
      x++;
    }
    logger(C_EXCH, DEBUG3, "cbp_callback_getOrders", "processed %d orders", x);
    json_object_put(j);
    exch_endpoll_orders(cbp.exch, SUCCESS);
    return;
  }

cbp_callback_getOrdersFail:
  if(j != NULL)
    json_object_put(j);
  logger(C_EXCH, CRIT, "cbp_callback_getOrders", "api error");
  exch_endpoll_orders(cbp.exch, FAIL);
}

void
cbp_callback_accounts(const int creq)
{
  enum json_tokener_error jerr = json_tokener_success;
  json_object *payload_json;

  char currency[200];

  double balance, available, hold;

  if(cq[creq].recv == NULL || cq[creq].state == CQS_FAIL)
  {
    logger(C_EXCH, CRIT, "cbp_callback_accounts", "curl request failed");
    return;
  }

  payload_json = json_tokener_parse_verbose(cq[creq].recv, &jerr);

  if(jerr == json_tokener_success && json_object_is_type(payload_json, json_type_array) == true)
  {
    int x = 0, num = json_object_array_length(payload_json);

    /*
     * [
     *   {
     *     "id":"a0a891a4-4f5c-42b5-a1a5-ee88345bbd49",
     *     "currency":"1INCH",
     *     "balance":"0.0000000000000000",
     *     "hold":"0.0000000000000000",
     *     "available":"0",
     *     "profile_id":"7de031f4-d317-41c5-a640-7585978a850f",
     *     "trading_enabled":true
     *   },
     * ]
     */

    while(x < num)
    {
      json_object *tmp;
      const char *str;

      if((tmp = json_object_array_get_idx(payload_json, x)))
      {
        if((str = json_object_get_string(json_object_object_get(tmp, "currency"))) != NULL)
          strncpy(currency, str, 199);
        else
          goto cbp_callback_accountsFail;

        if((str = json_object_get_string(json_object_object_get(tmp, "balance"))) != NULL)
          balance = strtod(str, NULL);
        else
          goto cbp_callback_accountsFail;

        if((str = json_object_get_string(json_object_object_get(tmp, "available"))) != NULL)
          available = strtod(str, NULL);
        else
          goto cbp_callback_accountsFail;

        if((str = json_object_get_string(json_object_object_get(tmp, "hold"))) != NULL)
          hold = strtod(str, NULL);
        else
          goto cbp_callback_accountsFail;

        exch_accountUpdate(cbp.exch, currency, balance, available, hold);
      }
      else
        goto cbp_callback_accountsFail;
      x++;
    }
    json_object_put(payload_json);
    logger(C_EXCH, DEBUG3, "cbp_callback_balances", "processed %d accounts", x);
    exch_endpoll_accounts(cbp.exch, SUCCESS);
    return;
  }
cbp_callback_accountsFail:
  if(payload_json != NULL)
    json_object_put(payload_json);
  logger(C_EXCH, CRIT, "cbp_callback_accounts", "api error (json parsing)");
  exch_endpoll_accounts(cbp.exch, FAIL);
}

void
cbp_callback_fees(const int creq)
{
  enum json_tokener_error jerr = json_tokener_success;
  json_object *payload_json;

  if(cq[creq].recv == NULL || cq[creq].state == CQS_FAIL)
  {
    logger(C_EXCH, CRIT, "cbp_callback_fees", "curl request failed");
    return;
  }

  payload_json = json_tokener_parse_verbose(cq[creq].recv, &jerr);

  if(jerr == json_tokener_success)
  {
    json_object *tmp;
    const char *str;

    if((tmp = payload_json))
    {
      int count = 0;

      if((str = json_object_get_string(json_object_object_get(tmp, "maker_fee_rate"))) != NULL)
      {
        cbp.exch->fee_maker = strtod(str, NULL);
        count++;
      }
      else
        goto cbp_callback_fees_fail;

      if((str = json_object_get_string(json_object_object_get(tmp, "taker_fee_rate"))) != NULL)
      {
        cbp.exch->fee_taker = strtod(str, NULL);
        count++;
      }
      else
        goto cbp_callback_fees_fail;

      if((str = json_object_get_string(json_object_object_get(tmp, "usd_volume"))) != NULL)
        cbp.exch->vol = strtod(str, NULL);
      else
        cbp.exch->vol = 0;

      if(count == 2)
      {
        if(payload_json != NULL)
          json_object_put(payload_json);

        exch_endpoll_fees(cbp.exch, SUCCESS);
        logger(C_EXCH, DEBUG1, "cbp_callback_fees", "cbp fees: maker:%.11f taker:%.11f vol:%.11f", cbp.exch->fee_maker, cbp.exch->fee_taker, cbp.exch->vol);
        return;
      }
    }
  }

cbp_callback_fees_fail:
  if(payload_json != NULL)
    json_object_put(payload_json);
  logger(C_EXCH, WARN, "cbp_callback_fees", "error getting exchange fees");
  exch_endpoll_fees(cbp.exch, FAIL);
}

void
cbp_callback_ticker(const int creq)
{
  struct product *prod = (struct product *)cq[creq].userdata;
  enum json_tokener_error jerr = json_tokener_success;
  json_object *payload_json;

  if(cq[creq].recv == NULL || cq[creq].state == CQS_FAIL)
  {
    logger(C_EXCH, CRIT, "cbp_callback_ticker", "curl request failed");
    return;
  }

  payload_json = json_tokener_parse_verbose(cq[creq].recv, &jerr);

  if(jerr == json_tokener_success)
  {
    const char *str;

    //  "trade_id": 86326522,
    //  "price": "6268.48",
    //  "size": "0.00698254",
    //  "time": "2020-03-20T00:22:57.833897Z",
    //  "bid": "6265.15",
    //  "ask": "6267.71",
    //  "volume": "53602.03940154"

    if((str = json_object_get_string(json_object_object_get(payload_json, "price"))) != NULL)
    {
      prod->ticker.lastpoll = global.now;
      prod->ticker.price = strtod(str, NULL);
    }
    else
      goto cbp_callback_tickerFail;

    if((str = json_object_get_string(json_object_object_get(payload_json, "bid"))) != NULL)
      prod->ticker.bid = strtod(str, NULL);
    else
      goto cbp_callback_tickerFail;

    if((str = json_object_get_string(json_object_object_get(payload_json, "ask"))) != NULL)
      prod->ticker.ask = strtod(str, NULL);
    else
      goto cbp_callback_tickerFail;

    if((str = json_object_get_string(json_object_object_get(payload_json, "size"))) != NULL)
      prod->ticker.size = strtod(str, NULL);
    else
      goto cbp_callback_tickerFail;

    if((str = json_object_get_string(json_object_object_get(payload_json, "volume"))) != NULL)
    {
      prod->ticker.volume = strtod(str, NULL);
      logger(C_EXCH, DEBUG3, "cbp_callback_ticker", "processed ticker for %s:%s:%s", cbp.exch->name, prod->asset, prod->currency);
      exch_endpoll_ticker(cbp.exch, prod, FAIL);
      return;
    }
  }

cbp_callback_tickerFail:
  if(payload_json != NULL)
    json_object_put(payload_json);
  logger(C_EXCH, CRIT, "cbp_callback_ticker", "api error");
  exch_endpoll_ticker(cbp.exch, prod, FAIL);
}

void
cbp_callback_products(const int creq)
{
  enum json_tokener_error jerr = json_tokener_success;
  json_object *payload_json;

  if(cq[creq].recv == NULL || cq[creq].state == CQS_FAIL)
  {
    logger(C_EXCH, CRIT, "cbp_callback_products", "curl request failed");
    return;
  }

  payload_json = json_tokener_parse_verbose(cq[creq].recv, &jerr);

  if(jerr == json_tokener_success && json_object_is_type(payload_json, json_type_array) == true)
  {
    int x = 0, num = json_object_array_length(payload_json);

    while(x < num && x < PRODUCTS_MAX)
    {
      json_object *tmp;
      const char *str;

      if((tmp = json_object_array_get_idx(payload_json, x)))
      {
        struct product prod;
        memset((void *)&prod, 0, sizeof(struct product));

    /*
     * {
        "id": "BTC-USD",
        "display_name": "BTC/USD",
        "base_currency": "BTC",
        "quote_currency": "USD",
        "base_increment": "0.00000001",
        "quote_increment": "0.01000000",
        "base_min_size": "0.00100000",
        "base_max_size": "280.00000000",
        "min_market_funds": "5",
        "max_market_funds": "1000000",
        "status": "online",
        "status_message": "",
        "cancel_only": false,
        "limit_only": false,
        "post_only": false,
        "trading_disabled": false,
        "fx_stablecoin": false
    },*/

        if((str = json_object_get_string(json_object_object_get(tmp, "base_currency"))) != NULL)
          strncpy(prod.asset, str, 19);
        else
          goto cbp_callback_productsFail;

        if((str = json_object_get_string(json_object_object_get(tmp, "quote_currency"))) != NULL)
          strncpy(prod.currency, str, 19);
        else
          goto cbp_callback_productsFail;

        if((str = json_object_get_string(json_object_object_get(tmp, "status"))) != NULL)
          strncpy(prod.status, str, 29);
        else
          goto cbp_callback_productsFail;

        if((str = json_object_get_string(json_object_object_get(tmp, "status_msg"))) != NULL)
          strncpy(prod.status_msg, str, 49);

        if((str = json_object_get_string(json_object_object_get(tmp, "min_market_funds"))) != NULL)
          prod.min_market_funds = strtod(str, NULL);
        else
          goto cbp_callback_productsFail;

        /*  --- base_min_size, base_max_size, and max_market_funds were removed on June 30 2022
         *  min_market_funds has been repurposed as the new "base_min_size"

        if((str = json_object_get_string(json_object_object_get(tmp, "base_min_size"))) != NULL)
          prod.base_min_size = strtod(str, NULL);

        else
          goto cbp_callback_productsFail;

        if((str = json_object_get_string(json_object_object_get(tmp, "base_max_size"))) != NULL)
          prod.base_max_size = strtod(str, NULL);
        else
          goto cbp_callback_productsFail;

        if((str = json_object_get_string(json_object_object_get(tmp, "max_market_funds"))) != NULL)
          prod.max_market_funds = strtod(str, NULL);

        else
          goto cbp_callback_productsFail;

        */

        if((str = json_object_get_string(json_object_object_get(tmp, "base_increment"))) != NULL)
          prod.base_increment = strtod(str, NULL);
        else
          goto cbp_callback_productsFail;

        if((str = json_object_get_string(json_object_object_get(tmp, "quote_increment"))) != NULL)
          prod.quote_increment = strtod(str, NULL);
        else
          goto cbp_callback_productsFail;

        if(json_object_get_boolean(json_object_object_get(tmp, "cancel_only")) == true)
          prod.flags |= PFL_CANCELONLY;

        if(json_object_get_boolean(json_object_object_get(tmp, "limit_only")) == true)
          prod.flags |= PFL_LIMITONLY;

        if(json_object_get_boolean(json_object_object_get(tmp, "post_only")) == true)
          prod.flags |= PFL_POSTONLY;

        if(json_object_get_boolean(json_object_object_get(tmp, "trading_disabled")) == true)
          prod.flags |= PFL_DISABLED;

        exch_productUpdate(cbp.exch, &prod);
      }
      else
        goto cbp_callback_productsFail;
      x++;
    }
    json_object_put(payload_json);
    exch_endpoll_products(cbp.exch, SUCCESS, x);
    return;
  }
cbp_callback_productsFail:
  if(payload_json != NULL)
    json_object_put(payload_json);
  logger(C_EXCH, CRIT, "cbp_callback_products", "api error");
  exch_endpoll_products(cbp.exch, FAIL, 0);
}

int
cbp_getTicker(struct product *prod)
{
  char url[500];

  snprintf(url, 499, "%s/%s/%s-%s/%s", CBP_API, CBP_API_PRODUCTS, prod->asset, prod->currency, CBP_API_TICKER);
  if(curl_request(CRT_GET, 0, url, NULL, NULL, cbp_callback_ticker, NULL, (void *)prod) > -1)
    return(SUCCESS);
  return(FAIL);
}

int
cbp_getProducts(void)
{
  if(curl_request(CRT_GET, 0, cbp.api_products, NULL, NULL, cbp_callback_products, NULL, NULL) > -1)
    return(SUCCESS);
  return(FAIL);
}

int
cbp_getFees(void)
{
  cbp_updatehash("GET", CBP_API_FEES, NULL);
  if(curl_request(CRT_GET, 0, cbp.api_fees, cbp.headers, NULL, cbp_callback_fees, NULL, NULL) > -1)
    return(SUCCESS);
  return(FAIL);
}

int
cbp_getAccounts(void)
{
  cbp_updatehash("GET", CBP_API_ACCOUNTS, NULL);
  if(curl_request(CRT_GET, 0, cbp.api_accounts, cbp.headers, NULL, cbp_callback_accounts, NULL, NULL) > -1)
    return(SUCCESS);
  return(FAIL);
}

int
cbp_getOrder(const char *id)
{
  char url[500];

  snprintf(url, 499, "%s/%s", CBP_API_ORDERS, id);
  cbp_updatehash("GET", url, NULL);
  snprintf(url, 499, "%s/%s", cbp.api_orders, id);

  if(curl_request(CRT_GET, 0, url, cbp.headers, NULL, cbp_callback_getOrder, NULL, NULL) > -1)
    return(SUCCESS);
  return(FAIL);
}

int
cbp_getOrders(void)
{
  cbp_updatehash("GET", CBP_API_ORDERS, NULL);
  if(curl_request(CRT_GET, 0, cbp.api_orders, cbp.headers, NULL, cbp_callback_getOrders, NULL, NULL) > -1)
    return(SUCCESS);
  return(FAIL);
}

int
cbp_orderPost(struct order *ord)
{
  char body[1000];
  int result, x;

  snprintf(body, 999, "{\n"
      "  \"product_id\": \"%s\",\n"
      "  \"type\": \"%s\",\n"
      "  \"side\": \"%s\",\n"
      "  \"post_only\": \"%s\",\n",
      ord->product,
      (ord->type == ORT_MARKET) ? "market" : "limit",
      (ord->state & ORS_LONG) ? "buy" : "sell",
      (ord->state & ORS_POSTONLY) ? "true" : "false");

  x = strlen(body);

  if((ord->type == ORT_MARKET) && ord->funds)
    snprintf(body + x, 999 - x, "  \"funds\": \"%.11f\"", ord->funds);
  else
    snprintf(body + x, 999 - x,
        "  \"size\": \"%.11f\",\n"
        "  \"price\": \"%.11f\"",
        ord->size, ord->price);

  x= strlen(body);

  if(ord->state & ORS_STOP)
    snprintf(body + x, 999 - x,
        ",\n  \"stop\": \"%s\",\n"
        "  \"stop_price\": \"%.11f\"",
        (ord->state & ORS_LONG) ? "entry" : "loss", ord->stop_price);

  x = strlen(body);

  snprintf(body + x, 999 -x, "\n}\n");

  cbp_updatehash("POST", CBP_API_ORDERS, body);
  cbp.headers[4] = "Accept: application/json";
  cbp.headers[5] = "Content-Type: application/json";
  cbp.headers[6] = NULL;

  printf("\n\n ---------------------------------\n%s\n-----------------------\n", body);

  result = 0;
  result = curl_request(CRT_POST, 0, cbp.api_orders, cbp.headers, NULL, cbp_callback_orderPost, body, (void *)ord);

  cbp.headers[4] = NULL;
  cbp.headers[5] = NULL;

  return((result == -1) ? FAIL : SUCCESS);
}

int
cbp_orderCancel(const struct order *o)
{
  int result;
  char temp[500];

  snprintf(temp, 499, "%s/%s", CBP_API_ORDERS, o->id);
  cbp_updatehash("DELETE", temp, NULL);
  cbp.headers[4] = "Accept: application/json";

  snprintf(temp, 499, "%s/%s", cbp.api_orders, o->id);
  result = curl_request(CRT_DELETE, 0, temp, cbp.headers, NULL, cbp_callback_orderCancel, NULL, (void *)o);

  cbp.headers[4] = NULL;

  return((result == -1) ? FAIL : SUCCESS);
}

// allocate per-market memory
struct cbp_pmv *
cbp_allocpmv(struct market *mkt)
{
  // allocate memory for per-market data if it hasn't been done yet
  if(mkt->exchdata == NULL)
  {
    if((mkt->exchdata = (void *)mem_alloc(sizeof(struct cbp_pmv))) == NULL)
    {
      logger(C_EXCH, CRIT, "cbp_getTrades", "unable to allocate memory");
      return(NULL);
    }
    mkt->exchdata_size = sizeof(struct cbp_pmv);

    // Init per-market vars memory
    memset((void *)mkt->exchdata, 0, sizeof(struct cbp_pmv));
  }
  return((struct cbp_pmv *)mkt->exchdata);
}

// this function submits a curl request to get the /trades endpoint
int
cbp_getTrades(struct market *mkt)
{
  struct cbp_pmv *pmv = (struct cbp_pmv *)mkt->exchdata;
  const char *rheaders[] = { "cb-after: ", NULL };
  char api_url[200];

  if(mkt->state & MKS_NEWEST)
  {
    snprintf(api_url, 199, "%s/%s-%s/%s", cbp.api_products, mkt->asset, mkt->currency, CBP_API_TRADES);
    pmv->restpage = 0L;
  }
  else
  {
    if(pmv->restpage)
      snprintf(api_url, 199, "%s/%s-%s/%s?after=%lu", cbp.api_products, mkt->asset, mkt->currency, CBP_API_TRADES, pmv->restpage);
    else
      logger(C_EXCH, CRIT, "cbp_getTrades", "request for older data but resetpage = 0");
  }

  if(curl_request(CRT_GET, 0, api_url, NULL, rheaders, cbp_callback_trades, NULL, (void *)mkt) > -1)
    return(SUCCESS);
  return(FAIL);
}

// This function is called when the exchange framework needs more data
// CBP API requires that you use the /trades endpoint to stay uptodate
// and the candles endpoint "not frequently". So in learning mode, we use
// /candles but after learning mode ends we use /trades. complex :(
int
cbp_getCandles(struct market *mkt)
{
  struct cbp_pmv *pmv;
  char api_url[300], str[50], str2[50];

  if(!(cbp.exch->state & EXS_PUB))
  {
    logger(C_EXCH, CRIT, "cbp_getTrades", "api has not been initialized");
    return(FAIL);
  }

  if((pmv = cbp_allocpmv(mkt)) == NULL)
    return(FAIL);

  // use candles endpoint
  if(mkt->state & MKS_LEARNING)
  {
    if(pmv->api_start)
      pmv->api_start = pmv->api_start - (mkt->gran * CBP_MAXCPP);
    else
      pmv->api_start = mkt->cc_time - (mkt->gran * CBP_MAXCPP);

    time2str(str, pmv->api_start, false);
    time2str(str2, pmv->api_start + (mkt->gran * (CBP_MAXCPP - 1)), false);

    snprintf(api_url, 299, "%s/%s-%s/%s?granularity=%d&start=%s&end=%s", cbp.api_products, mkt->asset, mkt->currency, CBP_API_CANDLES, mkt->gran, str, str2);

    if(curl_request(CRT_GET, 0, api_url, NULL, NULL, cbp_callback_candles, NULL, (void *)mkt) > -1)
      return(SUCCESS);
    return(FAIL);
  }
  else
    return(cbp_getTrades(mkt));
}

// note: malloc()'s are per-market, and thus free()'d in market.c on clean
int
cbp_close(void)
{
  logger(C_EXCH, INFO, "cbp_close", "coinbase pro api has been closed");
  return(SUCCESS);
}

int
cbp_init(void)
{
  int x = 0;

  memset((void *)&cbp, 0, sizeof(cbp));
 
  cbp.exch = exch_find("cbp");

  if(!(cbp.decsec_len = base64_decode(cbp.decsec, global.conf.cbp_secret, strlen(global.conf.cbp_secret))))
  {
    if(!(global.state & WMS_CM))
      logger(C_ANY, WARN, "cbp_init", "invalid api secret: priv api endpoints unavailable");
  }

  else
  {
    for(; x < CURL_MAXHEADERS; x++)
      cbp.headers[x] = cbp.head[x];

    snprintf(cbp.head[0], 300, "CB-ACCESS-KEY: %s", global.conf.cbp_key);
    snprintf(cbp.head[3], 300, "CB-ACCESS-PASSPHRASE: %s", global.conf.cbp_passphrase);
  }

  snprintf(cbp.api_accounts, 100, "%s/%s", CBP_API, CBP_API_ACCOUNTS);
  snprintf(cbp.api_products, 100, "%s/%s", CBP_API, CBP_API_PRODUCTS);
  snprintf(cbp.api_fees, 100, "%s/%s", CBP_API, CBP_API_FEES);
  snprintf(cbp.api_orders, 100, "%s/%s", CBP_API, CBP_API_ORDERS);

  logger(C_ANY, INFO, "cbp_init", "coinbase pro api is initializing");
  return(SUCCESS);
}

void
cbp_register(struct exchange *exch)
{
  strncpy(exch->name, "cbp", 14);
  strncpy(exch->longname, "Coinbase Pro", 49);
  exch->init = cbp_init;
  exch->close = cbp_close;
  exch->getAccounts = cbp_getAccounts;
  exch->getCandles = cbp_getCandles;
  exch->getOrder = cbp_getOrder;
  exch->getOrders = cbp_getOrders;
  exch->getProducts = cbp_getProducts;
  exch->getFees = cbp_getFees;
  exch->getTicker = cbp_getTicker;
  exch->orderPost = cbp_orderPost;
  exch->orderCancel = cbp_orderCancel;
}
