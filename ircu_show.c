#define WM_IRCU_SHOWC
#include "ircu_show.h"


void
show_marketsma(const int mid)
{
  struct market *mkt = &markets[mid];
  int x = 0;

  irc_reply("int  last         sma-%-3d  sma-%-3d  sma-%-3d  sma-%-3d  sma-%-3d", global.conf.sma1, global.conf.sma2, global.conf.sma3, global.conf.sma4, global.conf.sma5);
  irc_reply("---------------------------------------------------------------");
      
  for(; x < CINTS_MAX; x++)
  {
    struct candle *c = mkt->cint[x].cc;

    if(mkt->cint[x].tc >= HISTSIZE)
      irc_reply("%-03d  %-10.9f  %-7.2f  %-7.2f  %-7.2f  %-7.2f  %-7.2f", c->close, mkt->cint[x].length, c->sma1_diff, c->sma2_diff, c->sma3_diff, c->sma4_diff, c->sma5_diff);
  }
}


void
show_mktcandleshdr(void)
{
  irc_reply("id  int1  int2  int3  int4  int5  int6  int7");
  irc_reply("--------------------------------------------");
}

void
show_mktcandles(const int mid)
{
  struct market *mkt = &markets[mid];

  float x0 = (mkt->cint[0].tc >= HISTSIZE) ? 100 : (float)mkt->cint[0].tc / (float)HISTSIZE * 100;
  float x1 = (mkt->cint[1].tc >= HISTSIZE) ? 100 : (float)mkt->cint[1].tc / (float)HISTSIZE * 100;
  float x2 = (mkt->cint[2].tc >= HISTSIZE) ? 100 : (float)mkt->cint[2].tc / (float)HISTSIZE * 100;
  float x3 = (mkt->cint[3].tc >= HISTSIZE) ? 100 : (float)mkt->cint[3].tc / (float)HISTSIZE * 100;
  float x4 = (mkt->cint[4].tc >= HISTSIZE) ? 100 : (float)mkt->cint[4].tc / (float)HISTSIZE * 100;
  float x5 = (mkt->cint[5].tc >= HISTSIZE) ? 100 : (float)mkt->cint[5].tc / (float)HISTSIZE * 100;
  float x6 = (mkt->cint[6].tc >= HISTSIZE) ? 100 : (float)mkt->cint[6].tc / (float)HISTSIZE * 100;

  irc_reply("%-2d  %3.0f%%  %3.0f%%  %3.0f%%  %3.0f%%  %3.0f%%  %3.0f%%  %3.0f%%", mid, x0, x1, x2, x3, x4, x5, x6);
}
