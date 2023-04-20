#ifndef WM_MEMC

// exported functions
void mem_free(void *, const size_t);
void *mem_alloc(const size_t);
void *mem_calloc(const size_t, const size_t);
void *mem_realloc(void *, const size_t, const size_t);

#else

#include "common.h"

/* external vars */
extern struct globals global;

/* external funs */
extern void exit_program(const char *, const int);
extern void logger(const unsigned int, const int, const char *, const char *, ...);

static pthread_mutex_t mutex_mem_stats;

#endif
