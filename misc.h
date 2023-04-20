#ifndef WM_MISCC

// provides these functions
extern void banner(void);
extern const char *cint2str(const int);
extern int numargs(const char *);
extern int cint_length(int);
extern void d2h(char *, const double);
extern int expstr(char *, const void (*)(const int, const float *));
extern const char *find_cpuname(void);
extern int find_precision(double);
extern const char *human_size(size_t);
extern const char *logchan(const unsigned int);
extern const char *logsev(const unsigned int);
extern void logger(const unsigned int, const int, const char *, const char *, ...);
extern const char *signal_value(const int);
extern void set_signals(void);
extern const char *stopwatch_time(const time_t);
extern void mkupper(char *);
extern void url_encode_string(const char *, const char *);
extern void salt_random(void);
extern size_t base64_encode(char *, const unsigned char *, size_t);
extern size_t base64_decode(unsigned char *, const char *, size_t);
extern char *trim0(char *, const int, const double);
extern void time2str(char *, const time_t, const int);
extern void time2strf(char *, const char *, const time_t, const int);
extern void update_clock(void);

// provides these variables

#else

#include "common.h"
#include "colors.h"

#include <stdint.h>
#include <ctype.h>
#include <signal.h>
#include <math.h>

/* external vars */
extern struct globals global;
extern struct ircserver irc[];

extern pthread_mutex_t mutex_globalnow;
extern pthread_mutex_t mutex_logger;

/* external funs */
extern void exit_program(const char *, const int);
extern void irc_signoff(const char *);

/* for cosmetics */
struct logchannels
{
  unsigned int chan;
  const char *name;
} logchans[] = {
  { C_MAIN,   "MAIN"   },
  { C_ACCT,   "ACCT"   },
  { C_CURL,   "CURL"   },
  { C_DB,     "DB"     },
  { C_EXCH,   "EXCH"   },
  { C_IRC,    "IRC"    },
  { C_IRCU,   "IRCU"   },
  { C_MKT,    "MKT"    },
  { C_MISC,   "MISC"   },
  { C_SOCK,   "SOCK"   },
  { C_COINPP, "COINPP" },
  { C_CPANIC, "CPANIC" },
  { C_CONF,   "CONF"   },
  { C_CM,     "CM"     },
  { C_BT,     "BT"     },
  { C_STRAT,  "STRAT"  },
  { C_MEM,    "MEM"    },
  { C_ORD,    "ORD"    },
  { 0,        NULL     }
};

struct
{
  unsigned int severity;
  const char *name;
} logsevs[] = {
  { CRIT,   "CRIT"   },
  { WARN,   "WARN"   },
  { INFO,   "INFO"   },
  { DEBUG1, "DEBUG1" },
  { DEBUG2, "DEBUG2" },
  { DEBUG3, "DEBUG3" },
  { DEBUG4, "DEBUG4" },
  { 0,      NULL     }
};

/* tables for base 64 */
static unsigned char b64_encode[] = {
  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
  'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
  'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
  'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
  'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
  'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
  'w', 'x', 'y', 'z', '0', '1', '2', '3',
  '4', '5', '6', '7', '8', '9', '+', '/'
};

static unsigned char b64_decode[] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 62, 0, 0, 0,
  63, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 0, 0, 0, 0, 0, 0, 0, 0, 1,
  2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
  22, 23, 24, 25, 0, 0, 0, 0, 0, 0, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35,
  36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static int b64_mod[] = { 0, 2, 1 };

struct exp_arg
{
  int steps;
  float cur, start, step, end;
};

#endif
