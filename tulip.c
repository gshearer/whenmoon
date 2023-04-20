#define WM_TULIPC
#include "tulip.h"

double
tulip_adx(const int size, const int interval, const double *high, const double *low, const double *close, double *output)
{
  const double options[] = { interval };
  const double *inputs[] = { high, low, close };
  double *outputs[] = { output };

  if(ti_adx(size, inputs, options, outputs) != TI_OKAY)
    exit_program("tulip_adx", FAIL);

  return(output[size - (((interval - 1) * 2) + 1)]);
}

double
tulip_ema(const int size, const int interval, const double *input, double *output)
{
  const double options[] = { interval };
  const double *inputs[] = { input };
  double *outputs[] = { output };

  if(ti_ema(size, inputs, options, outputs) != TI_OKAY)
    exit_program("tulip_ema", FAIL);

  return(output[size - 1]);
}

void
tulip_fisher(const int size, const int interval, const double *high, const double *low, double *out_fish, double *out_sig)
{
  const double options[] = { interval };
  const double *inputs[] = { high, low };
  double *outputs[] = { out_fish, out_sig };

  if(ti_fisher(size, inputs, options, outputs) != TI_OKAY)
    exit_program("tulip_fisher", FAIL);
}

void
tulip_macd(const int size, const int fast, const int slow, const int signal, const double *input, double *out_macd, double *out_sig, double *out_hist)
{
  const double options[] = { fast, slow, signal };
  const double *inputs[] = { input };
  double *outputs[] = { out_macd, out_sig, out_hist };

  if(ti_macd(size, inputs, options, outputs) != TI_OKAY)
    exit_program("tulip_macd", FAIL);
}

double
tulip_rsi(const int size, const int interval, const double *input, double *output)
{
  const double options[] = { interval };
  const double *inputs[] = { input };
  double *outputs[] = { output };

  if(ti_rsi(size, inputs, options, outputs) != TI_OKAY)
    exit_program("tulip_rsi", FAIL);

  return(output[size - interval - 1]);
}

double
tulip_vwma(const int size, const int interval, const double *close, double *volume, double *output)
{
  const double options[] = { interval };
  const double *inputs[] = { close, volume };
  double *outputs[] = { output };

  if(ti_vwma(size, inputs, options, outputs) != TI_OKAY)
    exit_program("tulip_vwma", FAIL);

  return(output[size - interval]);
}
