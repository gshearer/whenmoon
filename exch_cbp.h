#ifndef WM_EXCH_CBP

// provides these functions
void cbp_register(struct exchange *);

#else

#include "common.h"
#include "curl.h"
#include "exch.h"
#include "init.h"
#include "mem.h"
#include "misc.h"

#include <openssl/hmac.h>
#include <json-c/json.h>

#define CBP_MAXCPP      300   // max candle history provided per poll

// https://docs.cloud.coinbase.com/exchange/docs
#define CBP_API               "https://api.exchange.coinbase.com"
#define CBP_API_PRODUCTS      "products"
#define CBP_API_ACCOUNTS      "accounts"
#define CBP_API_TRADES        "trades"
#define CBP_API_CANDLES       "candles"
#define CBP_API_FEES          "fees"
#define CBP_API_ORDERS        "orders"
#define CBP_API_TICKER        "ticker"

// globals
static struct
{
  unsigned char decsec[100];        // base64 decoded API secret
  size_t decsec_len; 

  char head[CURL_MAXHEADERS][500];  // 0 thru 3 are cbp required for auth
  const char *headers[CURL_MAXHEADERS];

  char api_fees[100];
  char api_accounts[100];
  char api_products[100];
  char api_orders[100];

  struct exchange *exch;
} cbp;

// pmv == per market variables
struct cbp_pmv
{
  time_t api_start;                 // timestamp for next api poll
  unsigned long int restpage;       // REST API pagination
};

#endif
