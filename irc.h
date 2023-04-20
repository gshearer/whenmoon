#ifndef WM_IRCC

// exported functions
extern int irc_admin(void);
extern int irc_addAdmin(const char *);
extern void irc_broadcast(const char *, ...);
extern int irc_find_chan(const int, const char *);
extern void irc_hook(void);
extern void irc_init(void);
extern void irc_reply(const char *, ...);
extern void irc_reply_str(const char *);
extern void irc_reply_who(const int, const char *, const char *, ...);

// exported variables
extern struct ircserver irc[];

#else

#include "common.h"
#include "init.h"
#include "ircu.h"
#include "misc.h"
#include "sock.h"

#include <fcntl.h>
#include <arpa/inet.h>

struct ircserver irc[IRC_MAX];

#endif
