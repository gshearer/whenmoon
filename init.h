#ifndef WM_INITC

// provides these functions
extern void exit_program(const char *, const int);
extern void init_globals(void);

// provides these variables
extern struct globals global;

#else

#include "common.h"
#include "bt.h"
#include "coinpp.h"
#include "curl.h"
#include "db.h"
#include "exch.h"
#include "irc.h"
#include "market.h"
#include "misc.h"
#include "sock.h"
#include "strat.h"
#include "version.h"

pthread_mutex_t mutex_globalnow = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_logger = PTHREAD_MUTEX_INITIALIZER;

struct globals global;

#endif
