#define WM_MEMC
#include "mem.h"

void
mem_free(void *ptr, const size_t bytes)
{
  if(global.state & WMS_BT)
    pthread_mutex_lock(&mutex_mem_stats);

  if(ptr != NULL)
  {
    logger(C_MEM, DEBUG1, "mem_free", "unallocating %lu bytes (%lu heap)", bytes, global.stats.heap);
    free(ptr);
    global.stats.heap -= bytes;
    global.stats.free++;
  }
  else
    logger(C_MEM, WARN, "mem_free", "called with NULL pointer :(");

  if(global.state & WMS_BT)
    pthread_mutex_unlock(&mutex_mem_stats);
}

void *
mem_alloc(const size_t bytes)
{
  void *ptr = malloc(bytes);

  if(bytes < 0)
  {
    logger(C_MEM, CRIT, "mem_alloc", "request to allocate 0 bytes?!");
    exit_program("mem_alloc", FAIL);
  }

  if(ptr == NULL)
  {
    logger(C_MEM, CRIT, "mem_alloc", "error allocating %lu bytes (OOM?)", bytes, strerror(errno));
    exit_program("mem_alloc", FAIL);
  }

  if(global.state & WMS_BT)
    pthread_mutex_lock(&mutex_mem_stats);

  global.stats.malloc++;
  global.stats.heap += bytes;

  logger(C_MEM, DEBUG1, "mem_alloc", "allocating %lu bytes (%lu heap)", bytes, global.stats.heap);

  if(global.state & WMS_BT)
    pthread_mutex_unlock(&mutex_mem_stats);

  return(ptr);
}

void *
mem_calloc(const size_t x, const size_t y)
{
  void *ptr = calloc(x, y);

  if(ptr == NULL)
  {
    logger(C_MEM, CRIT, "mem_calloc", "error allocating %lu bytes (OOM?)", x * y, strerror(errno));
    exit_program("mem_calloc", FAIL);
  }

  global.stats.calloc++;
  global.stats.heap += x * y;

  logger(C_MEM, DEBUG1, "mem_calloc", "allocating %d bytes (%d heap)", x* y, global.stats.heap);
  return(ptr);
}

void *
mem_realloc(void *ptr, const size_t new_size, const size_t old_size)
{
  void *ptr2 = realloc(ptr, new_size);
  size_t delta = new_size - old_size;

  if(ptr2 == NULL)
  {
    logger(C_MEM, CRIT, "mem_realloc", "error allocating %lu bytes (OOM?)", new_size, strerror(errno));
    exit_program("mem_realloc", FAIL);
  }

  global.stats.realloc++;
  global.stats.heap += delta;

  logger(C_MEM, DEBUG3, "mem_realloc", "new: %lu old: %lu delta: %lu heap: %lu)", new_size, old_size, delta, global.stats.heap);
  return(ptr2);
}
