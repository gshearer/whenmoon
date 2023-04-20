#ifndef WM_SOCKC

// exported functions
extern void sock_cleanup(void);
extern void sock_hook(void);
extern void sock_init(void);
extern int sock_new(const char *, const char *, const int, const void (*)(const int, char *), const void (*)(const int), const void (*)(const int));
extern int sock_write(const int, const char *, ...);

// exported variables
extern struct sockets sock[];

#else

#include "common.h"
#include "init.h"
#include "irc.h"
#include "misc.h"

#include <fcntl.h>
#include <arpa/inet.h>

struct sockets sock[SOCK_MAX];

#endif
