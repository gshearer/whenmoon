#ifndef WM_CONFIGC

// exported functions
extern int read_config(const char *);

#else

#include "common.h"
#include "init.h"
#include "irc.h"
#include "misc.h"

#define UNKNOWN       0
#define IRCSERVER     1
#define IRCPORT       2
#define IRCNICK       3
#define IRCOPERNAME   4
#define IRCOPERPASS   5
#define IRCCHAN       6
#define IRCCHANKEY    7
#define LOGPATH       8
#define LOGCHANS      9
#define LOGSEV        10
#define DBHOST        11
#define DBPORT        12
#define DBUSER        13
#define DBPASS        14
#define DBNAME        15
#define ADMINPASS     16
#define CBPKEY        17
#define CBPPASSPHRASE 18
#define CBPSECRET     19
#define CPTOKEN       20
#define CBPPROFILE    21

static struct
{
  char *key;
  unsigned short int value;
} cd[] = {
  { "UNKNOWN",     UNKNOWN },
  { "IRCSERVER",   IRCSERVER },
  { "IRCPORT",     IRCPORT },
  { "IRCNICK",     IRCNICK },
  { "IRCOPERNAME", IRCOPERNAME },
  { "IRCOPERPASS", IRCOPERPASS },
  { "IRCCHAN",     IRCCHAN },
  { "IRCCHANKEY",  IRCCHANKEY },
  { "LOGPATH",     LOGPATH },
  { "LOGCHANS",    LOGCHANS },
  { "LOGSEV",      LOGSEV },
  { "DBHOST",      DBHOST },
  { "DBPORT",      DBPORT },
  { "DBUSER",      DBUSER },
  { "DBPASS",      DBPASS },
  { "DBNAME",      DBNAME },
  { "ADMINPASS",   ADMINPASS },
  { "CBPKEY",      CBPKEY },
  { "CBPPASSPHRASE", CBPPASSPHRASE },
  { "CBPSECRET",   CBPSECRET },
  { "CPTOKEN",     CPTOKEN },
  { "CBPPROFILE",  CBPPROFILE },
  { NULL,          0 },
};

#endif
