#define WM_CURL
#include "curl.h"

// important note.... curl_callbacks must be thread safe :-)

static size_t
curl_callback_header(char *buf, size_t size, size_t nitems, void *userdata)
{
  struct curl_queue *cq = (struct curl_queue *)userdata;
  size_t len = size * nitems;
  char payload[len + 1];
  int x = 0;

  if(len < 10)
    return(len);

  memset((void *)payload, 0, len + 1);
  memcpy((void *)payload, buf, len);
  payload[len] = '\0';

  for(; x < CURL_MAXHEADERS && cq->rheaders[x][0] != '\0'; x++)
  {
    char *t = strcasestr(payload, cq->rheaders[x]);
    char *z = cq->result[x];

    if(t != NULL)
    {
      int c = 0;

      t += strlen(cq->rheaders[x]);
      while(*t >= '!' && *t <= '~' && ++c < 99)
        *z++ = *t++;
      *z = '\0';
      logger(C_CURL, DEBUG3, "curl_callback_header", "matched '%s' with result '%s'", cq->rheaders[x], cq->result[x]);
    }
  }
  return(len);
}

// This function is called by libcurl when it has received data from a GET
// It's called write because libcurl uses it to write that data to our mem
size_t
curl_callback_write(void *contents, size_t size, size_t nmemb, void *userdata)
{
  size_t realsize = size * nmemb;
  struct curl_queue *cq = (struct curl_queue *)userdata;

  cq->recv = (char *)mem_realloc(cq->recv, cq->recv_ptr + realsize + 1, cq->recv_ptr);
  memcpy(&(cq->recv[cq->recv_ptr]), contents, realsize);
  cq->recv_ptr += realsize;
  cq->recv_mem += realsize + 1;
  cq->recv[cq->recv_ptr] = '\0';
  return(realsize);
}

// This function is called by libcurl when sending data (us to them)
// It's called read because libcurl uses it to read our data we wish to send
static size_t
curl_callback_read(void *dest, size_t size, size_t nmemb, void *userdata)
{
  struct curl_queue *cq = (struct curl_queue *)userdata;
  size_t buffer_size = size * nmemb;

  if(cq->send_size)
  {
    size_t to_write = cq->send_size;
    if(to_write > buffer_size)
      to_write = buffer_size;
    memcpy(dest, (void *)cq->send, to_write);
    cq->send_ptr += to_write;
    cq->send_size -= to_write;
    return(to_write);
  }
  return(0);
}

// this function is called from inside the curl servicer thread
// x corresponds to which array element in curl_queue[] this belongs
void
curl_perform(int x)
{
  CURLcode curl_result = CURLE_FAILED_INIT;
  struct curl_slist *hlist = NULL;
  char curl_err[CURL_ERROR_SIZE];
  const char *str;
  long int http_rc = 0, hc = 0;
  CURL *curl_handle;

  if((curl_handle = curl_easy_init()) == NULL)
  {
    cq[x].state |= CQS_FAIL;
    strcpy(cq[x].errmsg, "failed to initialize libcurl handle");
    logger(C_CURL, CRIT, "curl_perform", cq[x].errmsg);
    return;
  }

  // add custom headers
  for(;cq[x].headers[hc] != NULL && cq[x].headers[hc][0] != '\0'; hc++)
    hlist = curl_slist_append(hlist, cq[x].headers[hc]);

  if(hc) // if we have custom headers, tell libcurl
    curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, hlist);

  // if response headers to look for, tell libcurl to use our callback
  if(cq[x].rheaders[0][0] != '\0')
  {
    curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, curl_callback_header);
    curl_easy_setopt(curl_handle, CURLOPT_HEADERDATA, (void *)&cq[x]);
  }

  // standard options for all req types
  curl_easy_setopt(curl_handle, CURLOPT_ERRORBUFFER, curl_err);
  curl_easy_setopt(curl_handle, CURLOPT_MAXREDIRS, CURL_MAXREDIRS);
  curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1);
  curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, CURL_TIMEOUT);
  curl_easy_setopt(curl_handle, CURLOPT_URL, cq[x].url);
  curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libp0ada/1.0");
  curl_err[0] = '\0';

  switch(cq[x].type)
  {
    case CRT_POST:
      curl_easy_setopt(curl_handle, CURLOPT_POST, 1L);
      curl_easy_setopt(curl_handle, CURLOPT_READFUNCTION, curl_callback_read);
      curl_easy_setopt(curl_handle, CURLOPT_READDATA, (void *)&cq[x]);
      curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, curl_callback_write);
      curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&cq[x]);
      logger(C_CURL, DEBUG4, "curl_perform", ">>> SEND PAYLOAD <<<\n---------------------------------------------------------\n%s\n---------------------------------------------------------", cq[x].send);
      break;
    case CRT_HEAD:
      curl_easy_setopt(curl_handle, CURLOPT_NOBODY, 1L);
      curl_easy_setopt(curl_handle, CURLOPT_HEADER, 1L);
    case CRT_DELETE:
      curl_easy_setopt(curl_handle, CURLOPT_CUSTOMREQUEST, "DELETE");
    case CRT_GET:
    default:
      curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, curl_callback_write);
      curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&cq[x]);
      break;
  }

  logger(C_CURL, DEBUG2, "curl_perform", "invoking libcurl for: %s", cq[x].url);
  curl_result = curl_easy_perform(curl_handle);
  curl_easy_cleanup(curl_handle);

  if(hc) // if we had headers, free libcurl mem
    curl_slist_free_all(hlist);

  // lock mem space so this thread can update with results
  pthread_mutex_lock(&cq[x].mutex);

  if(curl_result == CURLE_OK) // libcurl itself was successful
  {
    curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &http_rc);
    cq[x].state |= CQS_SUCCESS;
    cq[x].http_rc = http_rc;
    strcpy(cq[x].errmsg, "success");
    logger(C_CURL, DEBUG2, "curl_perform", "http response code %d for: %s", http_rc, cq[x].url);
    logger(C_CURL, DEBUG4, "curl_perform", ">>> RECEIVE PAYLOAD <<<\n---------------------------------------------------------\n%s\n---------------------------------------------------------", cq[x].recv);
    pthread_mutex_unlock(&cq[x].mutex);
    return;
  }

  if(strlen(curl_err))
    str = curl_err;
  else
    str = curl_easy_strerror(curl_result);

  strncpy(cq[x].errmsg, str, 300);
  logger(C_CURL, WARN, "curl_perform", "failed to get '%s': %s (%d)", cq[x].url, str, curl_result);
  cq[x].state |= CQS_FAIL;
  cq[x].http_rc = 0;
  if(cq[x].recv != NULL)
  {
    mem_free(cq[x].recv, cq[x].recv_mem);
    cq[x].recv = NULL;
    //cq[x].recv_mem = 0L;
  }
  pthread_mutex_unlock(&cq[x].mutex);
}

// thread entry point
void *
curl_servicer(void *ptr)
{
  long int me = (long int)ptr;

  logger(C_CURL, DEBUG3, "curl_servicer", "thread #%lu activated", me + 1);

  while(true)
  {
    int x = 0, serviced = 0;

    for(; x < CURL_MAXQUEUE; x++)
    {
      pthread_mutex_lock(&ct[me].mutex);

      if(ct[me].state == CTS_DIE)
      {
        pthread_mutex_unlock(&ct[me].mutex);
        logger(C_CURL, DEBUG3, "curl_servicer", "thread #%lu has retired", me + 1);
        ct[me].state = CTS_RETIRED;
        pthread_exit(NULL);
      }

      ct[me].state = CTS_ACTIVE;
      pthread_mutex_unlock(&ct[me].mutex);

      pthread_mutex_lock(&cq[x].mutex);
      if(cq[x].state & CQS_WAITING)
      {
        cq[x].state |= CQS_INPROG;
        cq[x].state &= ~CQS_WAITING;
        cq[x].last = global.now;
        logger(C_CURL, DEBUG3, "curl_servicer", "thread #%lu servicing: %s", me + 1, cq[x].url);
        pthread_mutex_unlock(&cq[x].mutex);
        curl_perform(x);
        serviced++;
        continue;
      }
      pthread_mutex_unlock(&cq[x].mutex);
    }

    if(serviced == 0) // no requests to process, consider retiring thread(s)
    {
      time_t now = time(NULL);

      pthread_mutex_lock(&ct[me].mutex);

      if(now - ct[me].begin >= CURL_THREADTTL)
      {
        ct[me].state = CTS_RETIRED;
        pthread_mutex_unlock(&ct[me].mutex);
        logger(C_CURL, DEBUG3, "curl_servicer", "thread #%lu has exceeded TTL of %d secs, retiring", me + 1, CURL_THREADTTL);
        pthread_exit(NULL);
      }

      logger(C_CURL, DEBUG3, "curl_servicer", "thread #%lu has gone to sleep", me + 1);
      ct[me].state = CTS_IDLE;
      pthread_mutex_unlock(&ct[me].mutex);
      pthread_mutex_lock(&ct[me].mutex_wake);
      pthread_cond_wait(&ct[me].wake, &ct[me].mutex_wake);
      pthread_mutex_unlock(&ct[me].mutex_wake);
      logger(C_CURL, DEBUG3, "curl_servicer", "thread #%lu has woke from slumber", me + 1);
    }
  }
}

void
curl_reset(struct curl_queue *req)
{
  if(req->recv != NULL)
  {
    mem_free(req->recv, req->recv_mem);
    req->recv = NULL;
    req->recv_size = req->recv_mem = req->recv_ptr = 0L;
  }

  if(req->send != NULL)
  {
    mem_free(req->send, req->send_mem);
    req->send = NULL;
    req->send_size = req->send_mem = req->send_ptr = 0L;
  }
  req->state = CQS_UNUSED;
}

const int
curl_request(const int type, const int flags, const char *url, const char *headers[], const char *rheaders[], const void (*callback)(const int), const char *senddata, void *userdata)
{
  int x = 0, queue_size = 0;

  for(; x < CURL_MAXQUEUE; x++)
    queue_size += (cq[x].state == CQS_UNUSED) ? 0 : 1;

  for(x = 0; x < CURL_MAXQUEUE; x++)
  {
    pthread_mutex_lock(&cq[x].mutex);

    if(cq[x].state == CQS_UNUSED)
    {
      int y = 0;

      memset((void *)&cq[x], 0, sizeof(struct curl_queue));

      cq[x].begin = global.now;

      if(irc[global.is].reply != NULL)
      {
        cq[x].irc = global.is;
        strncpy(cq[x].reply, irc[global.is].reply, 99);
      }
      else
      {
        cq[x].reply[0] = '\0';
        cq[x].irc = -1;
      }

      cq[x].state = CQS_WAITING | flags;
      cq[x].callback = callback;
      cq[x].type = type;
      cq[x].userdata = userdata;

      strcpy(cq[x].errmsg, "unset");
      strncpy(cq[x].url, url, 999);

      if(type == CRT_POST)
      {
        cq[x].send_size = cq[x].send_mem = strlen(senddata);
        if((cq[x].send = mem_alloc(cq[x].send_mem)) == NULL)
        {
          curl_reset(&cq[x]);
          return(-1);
        }
        memcpy((void *)cq[x].send, (void *)senddata, cq[x].send_mem);
      }

      for(; y < CURL_MAXHEADERS; y++)
      {
        cq[x].headers[y][0] = '\0';
        cq[x].rheaders[y][0] = '\0';
        cq[x].result[y][0] = '\0';
      }

      for(y = 0; y < CURL_MAXHEADERS && headers != NULL && headers[y] != NULL && headers[y][0] != '\0'; y++)
      {
        strncpy(cq[x].headers[y], headers[y], 99);
        logger(C_CURL, DEBUG3, "curl_request", "adding header #%d: >%s<", y, cq[x].headers[y]);
      }

      for(y = 0; y < CURL_MAXHEADERS && rheaders != NULL && rheaders[y] != NULL; y++)
      {
        strncpy(cq[x].rheaders[y], rheaders[y], 99);
        logger(C_CURL, DEBUG3, "curl_request", "adding response match #%d: >%s<", y, cq[x].rheaders[y]);
      }

      pthread_mutex_unlock(&cq[x].mutex);
      logger(C_CURL, DEBUG2, "curl_request", "added type %d '%s' to slot %d (queue: %d)", type, url, x, queue_size + 1);
      return(x);
    }
    pthread_mutex_unlock(&cq[x].mutex);
  }
  logger(C_CURL, CRIT, "curl_request", "request dropped due to max curl requests (%d)", x);
  return(-1);
}


// this function is called when there is a curl request in the queue waiting
// to be serviced. it looks for a thread to perform the service if one is not
// available it is created (assuming we havent reached MAX_THREADS)
void
curl_add_thread(void)
{
  long int x = 0;

  for(; x < CURL_MAXTHREADS; x++)
  {
    pthread_mutex_lock(&ct[x].mutex);

    /* if a thread is asleep, wake instead of adding another one*/
    if(ct[x].state == CTS_IDLE)
    {
      logger(C_CURL, DEBUG3, "curl_add_thread", "waking thread %lu", x + 1);
      pthread_mutex_lock(&ct[x].mutex_wake);
      pthread_cond_signal(&ct[x].wake);
      pthread_mutex_unlock(&ct[x].mutex_wake);
      pthread_mutex_unlock(&ct[x].mutex);
      return;
    }
    pthread_mutex_unlock(&ct[x].mutex);
  }

  /* no idle thread found, add one */
  for(x = 0; x < CURL_MAXTHREADS; x++)
  {
    pthread_mutex_lock(&ct[x].mutex);

    if(ct[x].state == CTS_UNUSED)
    {
      if(pthread_create(&ct[x].thread, NULL, curl_servicer, (void *)(long int)x) == 0)
      {
        logger(C_CURL, DEBUG3, "curl_add_thread", "curl servicer #%d added", x + 1);
        ct[x].state = CTS_IDLE;
        ct[x].begin = global.now;
        global.curl_threads++;
        pthread_mutex_unlock(&ct[x].mutex);
        return;
      }
    }
    pthread_mutex_unlock(&ct[x].mutex);
  }
  logger(C_CURL, WARN, "curl_add_thread", "max curl servicers reached (%d)", global.curl_threads);
}

void
curl_join(int x)
{
  time_t now;

  pthread_join(ct[x].thread, NULL);
  now = global.now;
  global.curl_threads--;
  logger(C_CURL, DEBUG3, "curl_join", "joined thread #%d (lifetime: %d secs)", x + 1, now - ct[x].begin);
  ct[x].state = CTS_UNUSED;
}

void
curl_hook(void)
{
  int x = 0;

  // process curl queue
  for(; x < CURL_MAXQUEUE; x++)
  {
    pthread_mutex_lock(&cq[x].mutex);

    if(cq[x].state & CQS_WAITING)
      curl_add_thread();

    // serviced & successful
    else if(cq[x].state & CQS_SUCCESS)
    {
      pthread_mutex_unlock(&cq[x].mutex);
      logger(C_CURL, DEBUG2, "curl_hook", "servicer #%d successful: invoking callback", x);
      cq[x].callback(x);
      curl_reset(&cq[x]);
      continue;
    }

    // serviced & unsuccessful
    else if(cq[x].state & CQS_FAIL)
    {
      if(++cq[x].retries == CURL_MAXRETRIES)
      {
        pthread_mutex_unlock(&cq[x].mutex);
        logger(C_CURL, WARN, "curl_hook", "max retries for '%s': notifying callback", cq[x].url);
        cq[x].callback(x);
        curl_reset(&cq[x]);
        continue;
      }

      if(global.now - cq[x].last > CURL_RETRYSLEEP)
      {
        cq[x].state = CQS_WAITING;
        curl_add_thread();
      }
    }
    pthread_mutex_unlock(&cq[x].mutex);
  }

  // consider thread retirement
  for(x = 0; x < CURL_MAXTHREADS; x++)
  {
    pthread_mutex_lock(&ct[x].mutex);
    if(ct[x].state == CTS_RETIRED)
      curl_join(x);
    pthread_mutex_unlock(&ct[x].mutex);
  }
}

void
curl_cleanup(void)
{
  int x = 0;

  logger(C_CURL, CRIT, "curl_cleanup", "killing %d threads", global.curl_threads);

  for(;x < CURL_MAXTHREADS; x++)
  {
    int state;

    pthread_mutex_lock(&ct[x].mutex);
    state = ct[x].state;
    pthread_mutex_unlock(&ct[x].mutex);

    if(state == CTS_ACTIVE || state == CTS_IDLE)
    {
      pthread_mutex_lock(&ct[x].mutex);
      ct[x].state = CTS_DIE;
      pthread_mutex_unlock(&ct[x].mutex);

      if(state == CTS_IDLE) /* wake it up so it can die :) */
      {
        pthread_mutex_lock(&ct[x].mutex_wake);
        pthread_cond_signal(&ct[x].wake);
        pthread_mutex_unlock(&ct[x].mutex_wake);
      }
      curl_join(x);
    }
    pthread_mutex_destroy(&ct[x].mutex);
  }

  for(x = 0; x < CURL_MAXQUEUE; x++)
    curl_reset(&cq[x]);
}

void
curl_init(void)
{
  int x = 0;

  memset((void *)cq, 0, sizeof(struct curl_queue) * CURL_MAXQUEUE);
  memset((void *)ct, 0, sizeof(struct curl_thread) * CURL_MAXTHREADS);

  for(; x < CURL_MAXTHREADS; x++)
  {
    ct[x].state = CTS_UNUSED;
    pthread_mutex_init(&ct[x].mutex, NULL);
    pthread_mutex_init(&ct[x].mutex_wake, NULL);
    pthread_cond_init(&ct[x].wake, NULL);
  }

  for(x = 0; x < CURL_MAXQUEUE; x++)
  {
    cq[x].state = CQS_UNUSED;
    pthread_mutex_init(&ct[x].mutex, NULL);
  }

  logger(C_ANY, INFO, "curl_init", "libcurl init (queue: %d threads: %d)", CURL_MAXQUEUE, CURL_MAXTHREADS);
}
