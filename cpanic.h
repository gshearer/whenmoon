#ifndef WM_CPANICC

// exported functions
extern void cpanic_curl_callback(const int);
extern void cpanic_init(void);
extern int cpanic_news(const char *);

#else

#define CPANIC_API "https://cryptopanic.com/api/v1/posts/"

#include "common.h"
#include "curl.h"
#include "init.h"
#include "irc.h"
#include "misc.h"

#include <json-c/json.h>

#endif
