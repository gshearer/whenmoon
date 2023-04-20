#ifndef WM_COINPPC

// exported functions

extern void coinpp_callback_tickers(const int);
extern void coinpp_callback_global(const int);
extern void coinpp_hook(void);
extern void coinpp_init(void);

// exported variables
extern struct crypto cryptos[];
extern struct crypto cryptos_prev[];
extern struct crypto_global_market crypto_global;

#else

#include "common.h"
#include "curl.h"
#include "init.h"
#include "misc.h"

#include <json-c/json.h>

#define COINPP_GLOBAL_API       "https://api.coinpaprika.com/v1/global"
#define COINPP_TICKERS_API      "https://api.coinpaprika.com/v1/tickers"

struct crypto cryptos[MAX_CRYPTOS];
struct crypto cryptos_prev[MAX_CRYPTOS];
struct crypto_global_market crypto_global;

#endif
