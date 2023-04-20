
#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <libpq-fe.h>

// general defines
// -------------------------------------------------------------------------

// logging channels -- update these in misc.h also
#define C_ANY     0        // always log
#define C_MAIN    (1<<0)   // main program flow
#define C_ACCT    (1<<1)   // accounts
#define C_CURL    (1<<2)   // libcurl & servicers
#define C_DB      (1<<3)   // database
#define C_EXCH    (1<<4)   // exchange api
#define C_IRC     (1<<5)   // IRC core
#define C_IRCU    (1<<6)   // IRC user funs
#define C_MKT     (1<<7)   // market / crypto trading
#define C_MISC    (1<<8)   // misc
#define C_SOCK    (1<<9)   // network I/O
#define C_COINPP  (1<<10)  // coin paprika api
#define C_CPANIC  (1<<11)  // crypto panic api
#define C_CONF    (1<<12)  // config reader
#define C_CM      (1<<13)  // candlemaker
#define C_BT      (1<<14)  // backtest
#define C_STRAT   (1<<15)  // strategies
#define C_MEM     (1<<16)  // mem management
#define C_ORD     (1<<17)  // order framework
#define C_CANDLE  (1<<18)  // candle.c

// logging severity
#define CRIT     0
#define WARN     1
#define INFO     2
#define DEBUG1   3
#define DEBUG2   4
#define DEBUG3   5
#define DEBUG4   6

// misc
#define SUCCESS  0
#define FAIL     1

#define false    0
#define true     1

#define DOWN     1
#define UP       2

// operational state flags
#define WMS_END   (1<<0)   // Exit main loop
#define WMS_TERM  (1<<1)   // Terminate program
#define WMS_CM    (1<<2)   // CandleMaker mode
#define WMS_BT    (1<<3)   // Backtest mode

#define BTUARGS_MAX        20       // Maximum number of user args in case file

// global state struct
// --------------------------------------------------------------------------

struct globals
{
  struct // config store
  {
    unsigned int logchans;
    int logsev;
    int tablesize;            // getdtablesize()
    char version[200];
    char verstring[200];
    char dbhost[200];
    char dbuser[200];
    char dbpass[200];
    char dbname[200];
    char adminpass[200];      // password for admin privs

    char cptoken[200];
    char cbp_key[200];
    char cbp_passphrase[200];
    char cbp_secret[200];
    char cbp_profile[200];

    char logpath[200];        // log file folder
    int dbport;

  } conf;

  time_t start, now, cpptimer, dbtimer;

  int state;               // see above for various state flags
  int is;                  // most recent IRC server processed
  int exitmode;

  int num_cryptos;         // how many cryptos tracked by coinpp module

  int curl_threads;
  PGconn *dbconn;          /* postgresql handle */

  // internal program stats
  struct
  {
    unsigned long int free, malloc, realloc, calloc;
    size_t heap;
  } stats;

  char ctmp[5000];
};

// database
// --------------------------------------------------------------------------

#define DB_IDLETIME       300    // if idle more than this, close

// sockets
// --------------------------------------------------------------------------

#define SOCK_MAX          20     /* maximum number of connections */
#define SOCK_TIMEOUT      30     /* number of seconds to wait for conn */
#define SOCK_BUFLEN       50000  /* maximum buf size for line read */

/* socket states */
#define SS_UNUSED         0
#define SS_INPROG         (1<<0)
#define SS_ESTAB          (1<<1)

/* socket types */
#define ST_IRC            0
#define ST_GENERIC        (1<<0)

struct sockets
{
  int fd;

  time_t timer_io;
  time_t timer_begin;

  struct addrinfo addr;
  char host[50];

  unsigned short int type;
  unsigned short int state;
  unsigned short int dport;

  unsigned long int read_bytes;
  unsigned long int write_bytes;

  char in[SOCK_BUFLEN];
  char *ins;

  const void (*read)(const int, char *);
  const void (*estab)(const int);
  const void (*close)(const int);
};


/* libcurl
 * ------------------------------------------------------------------------ */

#define CURL_MAXHEADERS   10    // maximum custom headers
#define CURL_MAXTHREADS   2     // max num of service threads
#define CURL_MAXREDIRS    3     // max num of http redirects to follow
#define CURL_MAXQUEUE     300   // max items in queue
#define CURL_MAXRETRIES   10    // give up after this many attempts
#define CURL_THREADTTL    300   // max time to live in seconds
#define CURL_TIMEOUT      30    // fetch timeout
#define CURL_RETRYSLEEP   10    // sleep this long between attempts

/* curl thread states */
#define CTS_UNUSED        1
#define CTS_ACTIVE        2
#define CTS_IDLE          3
#define CTS_RETIRED       4
#define CTS_DIE           5

struct curl_thread
{
  pthread_t thread;
  int state;
  time_t begin;
  pthread_mutex_t mutex;
  pthread_mutex_t mutex_wake;
  pthread_cond_t wake;
};

/* curl queue states */
#define CQS_UNUSED        0
#define CQS_WAITING       (1<<0)
#define CQS_INPROG        (1<<1)
#define CQS_SUCCESS       (1<<2)
#define CQS_FAIL          (1<<3)

/* curl request types */
#define CRT_GET           1    // recv
#define CRT_HEAD          2    // headers only
#define CRT_POST          3    // send
#define CRT_DELETE        4    // requires custom header

struct curl_queue
{
  char errmsg[300];                         // curl error message
  char reply[100];                          // who to respond to on irc
  char url[1000];
  char *recv, *send;                        // malloc'd per request
  size_t send_size, send_ptr, send_mem;
  size_t recv_size, recv_ptr, recv_mem;
  int irc;                                  // which IRC server to respond
  int state;                                // request state (CQS_XYZ)
  int type;                                 // request type (CRT_XYZ)
  int retries;                              // fetch attempts
  int http_rc;                              // HTTP response code
  time_t begin;                             // timestamp of request
  time_t last;                              // timestamp of last attempt

  const void (*callback)(const int);        // user callback

  char headers[CURL_MAXHEADERS + 1][100];   // custom HTTP headers
  char rheaders[CURL_MAXHEADERS + 1][100];  // look for these response headers
  char result[CURL_MAXHEADERS + 1][100];    // store results from ^ here

  pthread_mutex_t mutex;                    // thread lock

  void *userdata;                           // per req user data
};


/* irc
 * ------------------------------------------------------------------------ */

#define IRC_MAX           5       /* maximum simultaneous IRC servers */
#define IRC_HOLDDOWN      30      /* IRC server reconnect holddown timer */
#define IRC_REGTIMEOUT    10      /* time to wait for IRC server to begin */
#define IRC_NICKTIME      30      /* how often to attempt preferred nickname */
#define IRC_MAXCHANS      20      /* channels per server */
#define IRC_MAXNICKS      10      /* maximum nicknames per server */
#define IRC_MAXADMINS     5       /* maximum nicks who can use !identify */
#define IRC_ADMINTIME     3600    /* how long a user can have admin privs */

/* irc server states */
#define IRC_UNUSED        0
#define IRC_NEW           (1<<0)
#define IRC_ESTAB         (1<<1)
#define IRC_ACTIVE        (1<<2)

/* irc msg type also used for ircuser function type */
#define IRC_PUBLIC        0
#define IRC_PRIVATE       (1<<0)
#define IRC_PUBPRIV       (1<<1)

// Populated by ircu.h
struct ircuserfunc
{
  const char *cmd;
  const char *alias;
  int args;
  int admin;
  int type;
  const void (*func)(void);
  const char *usage;
};

struct ircserver
{
  unsigned short int state;

  time_t timer;
  time_t begin;
  time_t nicktimer;

  char *from;
  char *reply;
  char *args;

  char server[200];
  char port[10];
  char opername[20];
  char operpass[50];
  char nick[IRC_MAXNICKS][20];
  char chan[IRC_MAXCHANS][40];
  char chankey[IRC_MAXCHANS][200];
  char curnick[20];

  struct
  {
    char nick[20];
    time_t timer;
  } admin[IRC_MAXADMINS];

  int nicknum;

  int msgtype;                          /* 0 == public 1 == private */
  int failed;
  int sock;
};


/* cryptos
 * structures for accessing market wide resources such as coinmarketcap
 * ------------------------------------------------------------------------ */

#define MAX_CRYPTOS       20000  /* maximum number of cryptos to store per poll */
#define MAX_CPNEWS        5      /* number of entries to print when accessed */
#define COINPP_INTERVAL   120    /* coinpaprika polling interval */

struct crypto_global_market
{
  double mcap;
  double btc_mcap;
  double vol;
  float btc_dom;
  float mcap_24h;
  float vol_24h;
  float mcap_ath;
  float vol_ath;
  int assets;
};

struct crypto
{
  char id[50];
  char name[50];
  char sym[20];
  double price;
  double ath;
  double vol;
  double mcap;
  float ath_diff, vol_diff, mcap_diff;
  float change_15m, change_1h, change_24h, change_7d, change_30d, change_1y;
  int rank;
};


/* markets and data
 * ------------------------------------------------------------------------ */

#define INITCANDLES  20000   // how many candles to first request
#define HISTSIZE       200   // should match your longest indicator
#define MARKET_MAX      32   // max num of simultaneous markets
#define CINTS_MAX        7   // 1m 5m 15m 30m 1h 4h 1d
#define TULIP_ARRAYS     5   // ararys for accessing tulip indicators
#define TREND_MAX       10
#define CINT_SCORE_MAX  18       // number of points possible per cint: market_score()

struct trend
{
  unsigned char dir;
  int count;
  int prev;
};

struct trade
{
  time_t time;
  unsigned short int side;
  unsigned long int id;
  double price, size;
};

// HEY remember if you change this struct you invalidate your candle downloads
struct candle
{
  double open, high, low, close, volume, vwp;
  float size, pc_diff, asize;

  float last2, last3, last5, last10, last25, last50, last100, last150, all;

  // price moving averages
  double sma1, sma2, sma3, sma4, sma5;
  float sma1_diff, sma2_diff, sma3_diff, sma4_diff, sma5_diff;
  float sma1_2_diff, sma1_3_diff, sma1_4_diff, sma1_5_diff;
  float sma2_3_diff, sma2_4_diff, sma2_5_diff;
  float sma3_4_diff, sma3_5_diff;
  float sma4_5_diff;

  // simple comparison between current candle sma and two candles ago
  float sma1_angle, sma2_angle, sma3_angle, sma4_angle, sma5_angle;

  // volume moving averages
  double vsma1, vsma2, vsma3, vsma4, vsma5;
  float vsma1_diff, vsma2_diff, vsma3_diff, vsma4_diff, vsma5_diff;
  float vsma1_2_diff, vsma1_3_diff, vsma1_4_diff, vsma1_5_diff;
  float vsma2_3_diff, vsma2_4_diff, vsma2_5_diff;
  float vsma3_4_diff, vsma3_5_diff;
  float vsma4_5_diff;

  // volume weighted moving averages
  double vwma1, vwma2, vwma3, vwma4, vwma5;
  float vwma1_sma1_diff, vwma2_sma2_diff, vwma3_sma3_diff, vwma4_sma4_diff, vwma5_sma5_diff;

  // interval high & low
  double ci_high, ci_low;

  float pct_below_high, pct_above_low;

  float hlm_diff; // (ci_high + ci_low) / 2 vs current close

  float vg1, vg2, vg3, vg4, vg5; // % of vol that's green

  // various indicators
  double macd, macd_s, macd_h;
  double adx, fish, fish_sig, rsi, rsi_sma;

  // current price vs interval high/low & ath/atl
  float high_diff, low_diff, high_low_diff;

  // used to track current long order - this is duplicated
  // in market struct on purpose, this one is for logging
  float profit, profit_pct, profit_pct_high, profit_pct_low;

  // generic testing vars
  double test1, test2, test3, test4, test5;

  int score;     // market_score()
  int trades;
  int num;
  int low_age, high_age;

  // trend tracking
  struct trend t_fish;
  struct trend t_sma1_2;
  struct trend t_sma1_5;

  time_t time; // start of candle interval
};


// array of structs to store 1m, 5m, 15m, 30m, etc candle ints
struct candle_int
{
  struct candle *c;              // candle store (HISTSIZE)
  struct candle *lc;             // latest candle
  struct candle nc;              // storage for incoming candle

  int lc_slot;
  int tc;                        // total candles seen by this interval
                                 //
  //int nc;                        // next candle (insert array)
  int ic;                        // interval counter
  int length;                    // interval size in 1-min candles
  //int idx[HISTSIZE];             // index of candles (newest to oldest)

  int bt_seen;                   // candles seen during backtest only
 
  int buy_num;       // cnum of last long
  int sell_num;      // cnum of last short

  double low, high, volume;      // totals for HISTSIZE candles
  time_t t_low, t_high;          // timestamps of high & low
 
  int low_num, high_num;

  struct trend t_fish;
  struct trend t_sma1_2;
  struct trend t_sma1_5;
};

// Orders
// --------------------------------------------------------------------------

#define ORDERS_MAX      100           // how many orders we can track simultaneously

// Order types
#define ORT_LIMIT       1
#define ORT_MARKET      2

// Order states
#define ORS_OPEN        (1<<0)        // order is confirmed on exch book
#define ORS_LONG        (1<<1)        // order is long otherwise short
#define ORS_PENDING     (1<<2)        // waiting for exchange confirmation
#define ORS_STALE       (1<<3)        // made stale by incoming poll
#define ORS_FAILED      (1<<4)        // server rejected order or curl fail
#define ORS_UPDATE      (1<<5)        // check with known orders & update/add
#define ORS_POSTONLY    (1<<6)        // guarnatee maker fee
#define ORS_DELETING    (1<<7)        // waiting for exchange confirmation
#define ORS_CLOSED      (1<<8)        // order was executed
#define ORS_STOP        (1<<9)        // stop order
#define ORS_PAPER       (1<<10)       // dont submit to exchange

struct order
{
  int type;              // order type
  int state;             // order state
  int array;             // array id
  char msg[100];         // message from exchange
  char product[100];     // ASSET-CURRENCY
  char id[100];          // usually a uuid provided by exchange

  time_t create;         // order create time
  time_t change;         // last time order status changed

  double price;       // the price we wish to buy or sell at
  double size;        // quantity
  double fee;         // you suck coinbase
  double filled;      // how many we have so far
  double stop_price;  // stop trigger price
  double funds;       // how much money the user wants to spend mktorder 

  struct market *mkt;    // if NULL then this is a manual or discovered order
  struct exchange *exch; // to which supported exchange this order belongs
};

// advice types
#define ADVICE_NONE    0    // do nothing
#define ADVICE_SHORT   1    // sell
#define ADVICE_LONG    2    // buy

struct advice
{
  const char *signal;       // desc of reason for advice
  const char *msg;          // message for user (bot mode)
  double price;             // order price
  float pct;
  int advice;               // advice type
};

// Markets
// --------------------------------------------------------------------------

// market state flags
#define MKS_ACTIVE         (1<<0)
#define MKS_NEWEST         (1<<1)   // flag tells exch to get newest data next
#define MKS_LEARNING       (1<<2)   // learning mode
#define MKS_ENDLEARN       (1<<3)   // need to process candle history
#define MKS_MOREDATA       (1<<4)   // keep polling until this is absent
#define MKS_POLLED         (1<<5)   // active api poll
#define MKS_LONG           (1<<6)   // are we long or short?
#define MKS_ORDER          (1<<7)   // active order on the books
#define MKS_TRIMSTOP       (1<<8)   // stoploss has been trimmed
#define MKS_READY          (1<<9)   // initial candle download has completed
#define MKS_DOCTEST        (1<<10)  // reserved

// market options
#define MKO_AI             (1<<0)   // automated trading
#define MKO_TRAILSTOP      (1<<1)   // stoploss trails close
#define MKO_PAPER          (1<<2)   // paper trading
#define MKO_STRAT          (1<<3)   // strategy advice enable
#define MKO_LOG            (1<<4)   // enable candle & event logging
#define MKO_PLOT           (1<<5)   // generate html output
#define MKO_GETOUT         (1<<6)   // sell as soon as profit > 0 and disable AI
#define MKO_NOAIAFTER      (1<<7)   // disable AI after next short

struct mkt_stats
{
  int wins, losses, trades;
  float profit, wla, fees;
};

struct market
{
  int state;         // market state (bits are MKS_XYZ)
  int opt;           // market options (bits are MKO_XYZ)
  int gran;          // length of candles in seconds
  int irc;           // what irc server market is tied to
  int c_array;       // array slot of current candle
  int init_candles;  // candles requested for learning or history export
  int bt_state;      // market state during backtest
  
  int num_candles;   // total number of candles processed
  int empty_candles; // total number of empty candles
  int exposed;       // candles in MKS_LONG
  int sleepcount;    // wake countdown in candles
  int order_candles; // candle count waiting for order fill
 
  double minprofit;  // price of asset required to beat exchange fees

  // used to track current long order
  double profit, profit_pct, profit_pct_high, profit_pct_low;

  double ht;       // this tracks the high seen while exposed (trailstop)

  double buy_price;    // price of asset at time of long
  double buy_size;     // quantity of asset requested at long
  double buy_funds;    // (buy_size * buy_price) + fee
  time_t buy_time;     // timestamp of current candle when order closed

  double sell_price;   // price of asset at time of short
  double sell_size;    // quantity of asset at time of short
  time_t sell_time;    // timestamp of current candle when order closed

  double p_fee;        // fee to use for paper trading

  float ua[BTUARGS_MAX];

  char name[80];
  char asset[20];
  char currency[20];
  char reply[20];

  time_t start;             // UTC EPOCH start of market
  time_t cc_time;           // current candle time
  time_t ltt;               // timestamp of last trade processed
  time_t ott;               // opening trade time

  void *exchdata;   // storage for exchange API's per market globals
  size_t exchdata_size;
  void *stratdata;  // storage for strategy associated with this market
  size_t stratdata_size;

  FILE *log;   // text file output of market candles & events

  struct candle_int cint[CINTS_MAX];    // candle "interval" (1-min, 5-min, etc)
  struct candle *cc, *pc;               // pointers to cur & prev candle
  struct candle *candles;               // candle storage for learning mode

  // CM uses these to store downloaded candles in memory so that they can be
  // written to a file in a desirable order for quick backtesting
  struct candle *cstore[CINTS_MAX];     // candle store for each int used during CM
  int cstore_cnt[CINTS_MAX];            // candle counter for each int
 
  struct candle long_candle[CINTS_MAX * 3];   // cc for each int at long
  struct candle short_candle[CINTS_MAX * 3];  // cc for each int at short

  struct exchange *exch;   // exchange this market is associated to
  struct strategy *strat;  // strategy providing advice for this market
  struct product *p;       // product associated to this market
  struct order *o;         // pointer to order if MKS_ORDER
  struct advice a;         // last advice given from strat
 
  struct mkt_stats paper_stats;
  struct mkt_stats real_stats;

  struct // various stats for both real and paper trading
  {
    int wins, losses, trades;
    int p_wins, p_losses, p_trades;

    double start_cur, profit, fees;
    double p_start_cur, p_profit, p_fees;
  } stats;

  double u1, u2, u3; // backtest user vars for html output

  // per-market user configurable options
  struct
  {
    double funds; // how much currency to use on long's (takes priority)

    float stoploss, stoploss_adjust, stoploss_new, greed;
    float minprofit;

    float minfunds; // if an order is less than this, dont bother

    int aisleep; // how long AI should sleep after a close
    int orderwait; // how many candles to wait for an order fill
    int maxexposed; // maximum exposure
    int pct; // percentage of balance to use for longs

    // market defaults
    int sma1, sma2, sma3, sma4, sma5;
    int rsi_interval, rsi_oversold, rsi_underbought;
    int macd_fast, macd_slow, macd_signal;
    int fish_interval;
    int adx_interval;
    int vwma_interval;
  } u;
};

/* accounts (exchange balances)
 * ------------------------------------------------------------------------ */

// account state flags
#define ACS_STALE    (1<<0)         // account needs updated

#define ACCOUNT_MAX  2000           // accounts

struct account
{
  int state;                        // see ACS_ above
  struct exchange *exch;
  char currency[20];
  double balance, available, hold;
};

// strategies
// ------------------------------------------------------------------------

#define STRAT_MAX      40   // TODO: make this dynamic some day

struct strategy
{
  char name[50];
  char desc[250];
  int (*init)(struct market *);
  int (*close)(struct market *);
  void (*help)(struct market *);
  void (*set)(struct market *);
  void (*update)(struct market *);
  void (*advice)(struct market *);
};


// Products (markets)
// --------------------------------------------------------------------------

// product flags
#define PFL_CANCELONLY   (1<<0)        // can only cancel orders
#define PFL_LIMITONLY    (1<<1)        // only limit orders are accepted
#define PFL_POSTONLY     (1<<2)        // post only mode
#define PFL_DISABLED     (1<<3)        // trading disabled
#define PFL_STABLE       (1<<4)        // is a stable coin
#define PFL_STALE        (1<<5)        // requires update from new poll

#define PRODUCTS_MAX     2000

struct product
{
  char asset[20];
  char currency[20];
  char status[30];
  char status_msg[50];

  double base_min_size, base_max_size, quote_increment, base_increment, min_market_funds, max_market_funds;

  unsigned short int flags;

  struct
  {
    time_t lastpoll;
    double price, bid, ask, size, volume;
  } ticker;
};

// Exchanges (see exch.h also)
// --------------------------------------------------------------------------

#define EXCH_MAX  50

// exchange state flags
#define EXS_INIT         (1<<0)       // api is initializing
#define EXS_PUB          (1<<1)       // public api is ready
#define EXS_PRIV         (1<<2)       // private (authenticated) ready
#define EXS_ERROR        (1<<3)       // api had a crit error - disable

// polling intervals in seconds  - TODO: this should be per exchange
#define EXT_PRODUCTS     3600         // how often to get market pairs
#define EXT_ACCOUNTS       60         // how often to poll account bals
#define EXT_ORDERS         15         // how often to poll order info
#define EXT_FEES         3600         // how often to check fees
#define EXT_TICKERS       300         // how often to poll tickers

struct exchange
{
  int state;            // state flags
  double fee_maker;     // maker fee
  double fee_taker;     // taker fee
  double vol;           // volume provided by exchange

  struct product products[PRODUCTS_MAX];

  // polling timers
  time_t t_products;    // timer for get products endpoint
  time_t t_accounts;    // timer for account balance endpoint
  time_t t_orders;      // timer for open orders endpoint
  time_t t_tickers;     // timer for get all tickers
  time_t t_ticker;      // timer for each individual ticker
  time_t t_fees;        // timer for fees endpoint

  char name[15];
  char longname[50];
  int (*init)(void);                     // api init function
  int (*close)(void);                    // api close function
  int (*getAccounts)(void);              // api call to refresh balances
  int (*getOrder)(const char *);         // api call to get specific order data
  int (*getOrders)(void);                // api call to get all open orders
  int (*getCandles)(struct market *);    // api call to add a new market
  int (*getProducts)(void);              // request trade pairs
  int (*getFees)(void);                  // api call to get user's fees
  int (*getTicker)(struct product *);    // get last trade data for one pair
  int (*getTickers)(void);               // get last trade data for all pairs
  int (*orderPost)(struct order *);      // create order
  int (*orderCancel)(const struct order *); // cancel order
};

/* backtest data
 * ------------------------------------------------------------------------ */

struct bt_hdr
{
  unsigned short int soa;  // always 6809 .. the best 8-bit CPU ever
  size_t cst_size;         // sizeof(struct candle)
  int init_candles;        // num of 1-minute candles to read
  char whenmoon[9];        // always "whenmoon"
  char asset[20];
  char currency[20];
  char exchange[15];
  time_t start;            // start time of first candle
};
