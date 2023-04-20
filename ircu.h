#ifndef WM_IRCUC

// exported functions

// exported variables
extern struct ircuserfunc user_cmds[];

#else

#include "common.h"
#include "tinyexpr/tinyexpr.h"

#include "account.h"
#include "candle.h"
#include "cpanic.h"
#include "curl.h"
#include "coinpp.h"
#include "exch.h"
#include "init.h"
#include "irc.h"
#include "ircu_show.h"
#include "market.h"
#include "misc.h"
#include "order.h"
#include "sock.h"
#include "strat.h"

#include <ctype.h>

void ircu_8ball(void);
void ircu_addquote(void);
void ircu_crypto(void);
void ircu_diff(void);
void ircu_export(void);
void ircu_help(void);
void ircu_identify(void);
void ircu_join(void);
void ircu_math(void);
void ircu_market(void);
void ircu_mcap(void);
void ircu_news(void);
void ircu_order(void);
void ircu_part(void);
void ircu_ping(void);
void ircu_set(void);
void ircu_show(void);
void ircu_show2(void);
void ircu_status(void);
void ircu_version(void);

struct ircuserfunc user_cmds[] = {
//  NAME         ALIAS  ARGS  ADMIN       MSGTYPE      FUNCTION-POINTER    USAGE
  { "8ball",     "8",   1,    false,      IRC_PUBLIC,  ircu_8ball,      "[8]ball <question>" },
  { "crypto",    "c",   0,    false,      IRC_PUBPRIV, ircu_crypto,     "[c]rypto <symbol>" },
  { "diff",      "d",   2,    false,      IRC_PUBPRIV, ircu_diff,       "[d]iff num1 num2" },
  { "export",    "ex",  1,    true,       IRC_PUBPRIV, ircu_export,     "[ex]port <args>" }, 
  { "help",      NULL,  0,    false,      IRC_PUBPRIV, ircu_help,       "help [command]" },
  { "identify",  NULL,  1,    false,      IRC_PRIVATE, ircu_identify,   "identify <password>" },
  { "join",      NULL,  1,    true,       IRC_PUBPRIV, ircu_join,       "join <channel>" },
  { "math",      NULL,  1,    false,      IRC_PUBPRIV, ircu_math,       "math <expression>" },
  { "market",    "ma",  2,    true,       IRC_PUBPRIV, ircu_market,     "market <id> <subcmd> [args]" },
  { "mcap",      NULL,  0,    false,      IRC_PUBPRIV, ircu_mcap,       "displays crypto market cap data" },
  { "news",      NULL,  0,    false,      IRC_PUBPRIV, ircu_news,       "displays news from cryptopanic" },
  { "order",     NULL,  1,    true,       IRC_PUBPRIV, ircu_order,      "heh" },
  { "part",      NULL,  1,    true,       IRC_PUBPRIV, ircu_part,       "part <channel>" },
  { "ping",      NULL,  1,    false,      IRC_PUBPRIV, ircu_ping,       "ping [arg]" },
  { "set",       NULL,  2,    true,       IRC_PUBPRIV, ircu_set,        "set param arg" },
  { "show",      "sh",  1,    true,       IRC_PUBPRIV, ircu_show,       "show <what> [args]" },
  { "status",    NULL,  0,    false,      IRC_PUBPRIV, ircu_status,     "displays bot status" },
  { "version",   NULL,  0,    false,      IRC_PUBPRIV, ircu_version,    "displays bot version" },
  { NULL, NULL, 0, 0, 0, NULL, NULL }
};

#endif
