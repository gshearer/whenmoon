#ifndef WM_CANDLEC

// exported functions
void candle_html(const struct market *, const char *, const double, const double);
void candle_log(const struct market *, const int);
void candle_mkempty(struct candle *, const struct candle *);
void candle_plot(const struct market *, const int, const int);
void candle_store(const struct market *, struct candle []);
char *candle_str(const struct candle *, const int, const int);
time_t candle_time(const time_t, const int, const int);

#else

#include "common.h"
#include "colors.h"
#include "init.h"
#include "misc.h"

#endif
