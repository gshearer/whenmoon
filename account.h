#ifndef WM_ACCOUNT

// function export
extern void account_init(void);
extern struct account *account_find(const struct exchange *, const char *);
extern void account_mkstale(struct exchange *);
extern struct account *account_new(struct exchange *, const char *);
extern void account_update(struct exchange *, const char *, const double, const double, const double);

// variables export
extern struct account accounts[];

#else

#include "common.h"
#include "exch.h"
#include "market.h"
#include "misc.h"

struct account accounts[ACCOUNT_MAX];

#endif
