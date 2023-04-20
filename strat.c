#define WM_STRATC
#include "strat.h"

struct strategy *
strat_find(const char *name)
{
  int x = 0;

  for(; strategies[x].name[0] != '\0' && strcasecmp(strategies[x].name, name); x++);
  return((strategies[x].name[0] == '\0') ? NULL : &strategies[x]);
}

void
strat_init(void)
{
  int x = 0, y;

  memset((void *)strategies, 0, sizeof(struct strategy) * STRAT_MAX);
  logger(C_ANY, INFO, "strat_init", "strategy manager initialized (max %d)", STRAT_MAX);

  for(; strats[x].strat_reg != NULL; x++)
  {
    // find an empty array slot
    for(y = 0; y < STRAT_MAX && strategies[y].name[0] != '\0'; y++);
    if(y < STRAT_MAX)
    {
      strats[x].strat_reg(&strategies[y]);
      logger(C_ANY, INFO, "strat_register", "registered strategy [%s]", strategies[y].name);
    }
    else
      logger(C_ANY, CRIT, "strat_register", "max strategies reached");
  }
}
