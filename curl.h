#ifndef WM_CURL

// provides these functions
extern void curl_cleanup(void);
extern void curl_hook(void);
extern void curl_init(void);
extern const int curl_request(const int, const int flags, const char *, const char *[], const char *[], const void (*)(const int), const char *, void *);

// provides these variables
extern struct curl_queue cq[];
extern struct curl_thread ct[];

#else

#include "common.h"
#include "init.h"
#include "irc.h"
#include "mem.h"
#include "misc.h"

#include <curl/curl.h>

struct curl_queue cq[CURL_MAXQUEUE];
struct curl_thread ct[CURL_MAXTHREADS];

#endif
