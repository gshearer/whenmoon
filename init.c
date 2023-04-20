#define WM_INITC
#include "init.h"

void
exit_program(const char *func, const int status)
{
  if(global.state & WMS_TERM)
  {
    fprintf(stderr, "signal loop detected: ungraceful exit :-(\n");
    fflush(stderr);
    exit(status);
  }

  global.state |= WMS_TERM;

  if(status == FAIL)
  {
    if(errno)
      logger(C_MAIN, CRIT, "exit_program", "called by %s() with status fail: %s", func, strerror(errno));
    else
      logger(C_MAIN, CRIT, "exit_program", "called by %s() with status fail", func);
  }

  logger(C_MAIN, INFO, "exit_program", "[preclean] free:%lu malloc:%lu realloc:%lu calloc:%lu heap: %lu", global.stats.free, global.stats.malloc, global.stats.realloc, global.stats.calloc, global.stats.heap);

  bt_cleanup();
  sock_cleanup();
  db_cleanup();
  curl_cleanup();
  market_cleanup();
  exch_cleanup();

  logger(C_MAIN, INFO, "exit_program", "[postclean] free:%lu malloc:%lu realloc:%lu calloc:%lu heap: %lu", global.stats.free, global.stats.malloc, global.stats.realloc, global.stats.calloc, global.stats.heap);

  if(!status)
    logger(C_MAIN, CRIT, "exit_program", "graceful exit");
  else
    logger(C_MAIN, CRIT, "exit_program", "ending execution with status %d", status);

  exit(status);
}

void
init_globals(void)
{
  memset((void *)&global, 0, sizeof(struct globals));

  global.conf.tablesize = getdtablesize();

  global.start = global.now = time(NULL);

  strncpy(global.conf.version, VERSION, 199);
  snprintf(global.conf.verstring, 200, "whenmoon version %s (build %s) compiled by %s on %s (%s) using %s", VERSION, BUILDNUM, BUILDUSER, BUILDHOST, BUILDOS, BUILDCC);

  global.dbconn = NULL;
  global.is = -1;

  // default all channels enabled
  //global.conf.logchans = C_MAIN | C_MEM | C_ACCT | C_CURL | C_DB | C_EXCH | C_IRC | C_IRCU | C_MKT | C_MISC | C_SOCK | C_COINPP | C_CPANIC | C_CONF | C_CM | C_BT | C_STRAT | C_ORD;

  global.conf.logchans = C_MAIN | C_ORD | C_EXCH | C_MKT | C_STRAT | C_MISC | C_IRCU | C_CANDLE | C_EXCH;

  global.conf.logsev = DEBUG3;

  salt_random();
}
