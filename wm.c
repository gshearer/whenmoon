#define WM_WMC
#include "wm.h"

void
whenmoon(const char *config)
{
  init_globals();
  set_signals();

  banner();
  logger(C_MAIN, INFO, "whenmoon", "irc bot initializing");

  db_init();
  sock_init();
  curl_init();
  irc_init();
  account_init();
  exch_init();
  market_init();
  strat_init();
  coinpp_init();
  cpanic_init();

  if(read_config(config) == FAIL)
    exit_program("whenmoon", FAIL);

  exch_initExch(NULL);

  logger(C_MAIN, INFO, "whenmoon", "entering main loop - let us begin!");

  // main loop -- call hooks
  while(!(global.state & WMS_END))
  {
    update_clock();
    sock_hook();
    curl_hook();
    db_hook();
    irc_hook();
    exch_hook();
    market_hook();
    //coinpp_hook();
  }
  
  exit_program("whenmoon", SUCCESS);
}
