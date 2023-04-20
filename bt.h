#ifndef WM_BTC

// exported functions
void backtest(const int, const char *[]);
void bt_cleanup(void);

#else

#include <sys/stat.h>

#include "common.h"
#include "candle.h"
#include "colors.h"
#include "exch.h"
#include "init.h"
#include "market.h"
#include "mem.h"
#include "misc.h"
#include "strat.h"

#define BTDS_MAX           25        // Maximum loadable data sets
#define BTT_MAX            256       // Max threads
#define BTT_MAXCASES       20000000

#define BTT_UNUSED        0
#define BTT_ACTIVE        1
#define BTT_DIE           2
#define BTT_CLEAN         3

struct bt_plot
{
  const struct market *mkt;
  const char *path;

  double price;      // Y axis
  time_t stamp;      // X axis

  int cint;          // candle length
  int idx;           // array index of first candle to plot
  int islong;        // true == long false == short
  int trade_num;     // number of trade
};

static struct bt_case 
{
  float uargs[BTUARGS_MAX];
  int num_uargs;
  struct mkt_stats results;
} *bt_cases = NULL, *best_case = NULL, best_cases[3];

static struct bt_dataset
{
  struct bt_hdr hdr;
  struct candle *cstore[CINTS_MAX];
  struct mkt_stats results;
  struct bt_case *best_case;
} *bt_datasets = NULL;

static struct bt_thread
{
  struct bt_case *bc;
  struct bt_dataset *ds;
  struct mkt_stats results;

  int num, state;

  pthread_t thread;
  pthread_mutex_t mutex;
} *bt_threads;

static unsigned char bt_flip = false;
static FILE *fp_index = NULL, *fp_wldiff = NULL;
static unsigned long int bt_numcases = 0L, bt_numcandles = 0;
static int bt_numdatasets = 0, bt_numthreads = 0, trade_num = 0;
static struct strategy *bt_strat = NULL;
static time_t t_begin;
static double bt_start_cur = 0, bt_minfunds;
static float bt_fee = 0;

#endif
