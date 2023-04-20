#ifndef WM_TULIPC

// exported functions
extern double tulip_adx(const int, const int, const double *, const double *, const double *, double *);
extern double tulip_ema(const int, const int, const double *, double *);
extern double tulip_fisher(const int, const int, const double *, double *, double *, double *);
extern double tulip_macd(const int, const int, const int, const int, const double *, double *, double *, double *);
extern double tulip_rsi(const int, const int, const double *, double *);
extern double tulip_vwma(const int, const int, const double *, double *, double *);

// exported variables
extern double tulip_inputs[TULIP_ARRAYS][HISTSIZE];
extern double tulip_outputs[TULIP_ARRAYS][HISTSIZE];

#else

#include "common.h"
#include "tulipindicators/indicators.h"
#include "init.h"

// arrays for passing data to/from tulip lib 
double tulip_inputs[TULIP_ARRAYS][HISTSIZE];
double tulip_outputs[TULIP_ARRAYS][HISTSIZE];

#endif
