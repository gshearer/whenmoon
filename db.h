#ifndef WM_DBC

// exported functions
extern void db_cleanup(void);
extern void db_hook(void);
extern void db_init(void);

#else

#include "common.h"
#include "init.h"
#include "misc.h"

#endif
