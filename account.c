#define WM_ACCOUNT
#include "account.h"

// make all known accounts for particular exchange stale so we can compare
// the incoming update
void
account_mkstale(const struct exchange *exch)
{
  int x = 0;
  for(; x < ACCOUNT_MAX; x++)
    if(accounts[x].exch == exch && accounts[x].currency[0] != '\0')
      accounts[x].state |= ACS_STALE;
}

struct account *
account_find(const struct exchange *exch, const char *currency)
{
  int x = 0;

  for(; x < ACCOUNT_MAX; x++)
    if(accounts[x].exch == exch && strcasecmp(accounts[x].currency, currency) == 0)
      return(&accounts[x]);
  return(NULL);
}

struct account *
account_new(struct exchange *exch, const char *currency)
{
  int x = 0;

  for(; x < ACCOUNT_MAX; x++)
  {
    if(accounts[x].currency[0] == '\0')
    {
      strncpy(accounts[x].currency, currency, 19);
      mkupper(accounts[x].currency);
      accounts[x].exch = exch;
      return(&accounts[x]);
    }
  }
  return(NULL);
}

void
account_update(struct exchange *exch, const char *currency, const double balance, const double available, const double hold)
{
  struct account *a = account_find(exch, currency);

  if(a == NULL)
    a = account_new(exch, currency); // TODO event hook

  if(a != NULL)
  {
    a->exch = exch;
    a->balance = balance;
    a->available = available;
    a->hold = hold;
  }
}

void
account_init(void)
{
  memset((void *)accounts, 0, sizeof(struct account) * ACCOUNT_MAX);
  logger(C_ACCT, INFO, "account_init", "account balance manager initialized (%d max)", ACCOUNT_MAX);
}
