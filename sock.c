#define WM_SOCKC
#include "sock.h"

static void
sock_failed(int x)
{
  int myerrno = errno;

  close(sock[x].fd);
  sock[x].fd = -1;
  sock[x].ins = sock[x].in;
  sock[x].timer_io = global.now;
  sock[x].state = SS_UNUSED;

  if(myerrno)
    logger(C_SOCK, INFO, "sock_failed", "[slot %d] connection to %s:%d failed: %s", x, sock[x].host, sock[x].dport, strerror(myerrno));
  else
    logger(C_SOCK, INFO, "sock_failed", "[slot %d] connection to %s:%d closed", x, sock[x].host, sock[x].dport);

  if(sock[x].close != NULL)
    sock[x].close(x);
}

static void
sock_estab(int x)
{
  sock[x].state = SS_ESTAB;
  sock[x].timer_io = sock[x].timer_begin = global.now;
  logger(C_SOCK, INFO, "sock_estab", "session to %s:%d established", sock[x].host, sock[x].dport);
  if(sock[x].estab != NULL)
    sock[x].estab(x);
}

static void
sock_read(int x)
{
  int bufsize = sizeof(sock[x].in) - (sock[x].ins - sock[x].in) - 1;
  int length;

  if(sock[x].fd == -1)
  {
    logger(C_SOCK, CRIT, "sock_read", "called with invalid fd for %s:%d", sock[x].host, sock[x].dport);
    return;
  }

  if((length = read(sock[x].fd, sock[x].ins, bufsize)) < 1)
  {
    sock_failed(x);
    return;
  }

  sock[x].read_bytes += length;

  logger(C_SOCK, DEBUG3, "sock_read", "read %d bytes from %s:%d", length, sock[x].host, sock[x].dport);

  if(strpbrk(sock[x].in, "\x0a\x0d")) /* CR of LF exists */
  {
    char *t = sock[x].in;

    *(sock[x].ins + length) = '\0';

    while(true)
    {
      sock[x].ins = t;

      while(*t && *t != '\r' && *t != '\n')
        t++;

      if(*t)
      {
        while(*t && (*t == '\r' || *t == '\n'))
          *t++ = '\0';
        if(sock[x].read != NULL)
          sock[x].read(x, sock[x].ins);
      }
      else
        break;
    }
    strcpy(sock[x].in, sock[x].ins);
    sock[x].ins = sock[x].in + (t - sock[x].ins);
  }

  else  /* no CR or LF found */
  {
    if(sizeof(sock[x].in) - ((sock[x].ins + length) - sock[x].in) < 2)
    {
      *(sock[x].in + (sizeof(sock[x].in) - 1)) = '\0';
      sock[x].ins = sock[x].in;
    }
    else
      sock[x].ins = sock[x].ins + length;
  }
}

int
sock_write(const int x, const char *format, ...)
{
  char buf[10000];
  int write_len, write_res;
  va_list ptr;

  if(x == -1)
    return(-1);

  va_start(ptr, format);
  vsnprintf(buf, 9998, format, ptr);
  va_end(ptr);

  logger(C_SOCK, DEBUG4, "sock_write", "%s:%d >>%s<<", sock[x].host, sock[x].dport, buf);

  strcat(buf, "\n");
  write_len = strlen(buf);
  sock[x].write_bytes += write_len;

  if((write_res = write(sock[x].fd, buf, write_len)) == -1)
    logger(C_SOCK, CRIT, "sock_write", "error writing to host %s:%d (errno = %d:%s)", sock[x].host, sock[x].dport, errno, strerror(errno));

  return(write_res);
}

static struct addrinfo *
ip_resolve(const char *host, const char *port)
{
  struct addrinfo hints, *resolved;
  int rescode;

  memset(&hints, 0, sizeof(struct addrinfo));

  hints.ai_family = AF_UNSPEC;  /* support both IPv4 and IPv6 mkay */
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;
  hints.ai_addrlen = hints.ai_flags = 0;
  hints.ai_addr = NULL;
  hints.ai_canonname = NULL;
  hints.ai_next = NULL;

  if((rescode = getaddrinfo(host, port, &hints, &resolved)) != 0)
  {
    logger(C_SOCK, WARN, "ip_resolve", "unable to resolve host '%s': %s", host, gai_strerror(rescode));
    return(NULL);
  }

  return(resolved);
}

static int
sock_connect(const int x)
{
  int sockflag;

  if((sockflag = fcntl(sock[x].fd, F_GETFL)) == -1)
  {
    logger(C_SOCK, CRIT, "sock_connect", "slot #%d: unable to get socket status: %s", x, strerror(errno));
    close(sock[x].fd);
    exit_program("sock_connect", FAIL);
  }

  if(fcntl(sock[x].fd, F_SETFL, sockflag | O_NONBLOCK) == -1)
  {
    logger(C_SOCK, CRIT, "sock_connect", "slot #%d: unable to set non-blocking operation (errno %d:%s", x, errno, strerror(errno));
    close(sock[x].fd);
    exit_program("sock_connect", FAIL);
  }

  if(connect(sock[x].fd, sock[x].addr.ai_addr, sock[x].addr.ai_addrlen) == -1)
  {
    if(errno == EINPROGRESS || errno == EWOULDBLOCK)
    {
      sock[x].state = SS_INPROG;
      logger(C_SOCK, INFO, "sock_connect", "slot #%d: connection to %s:%d is in progress", x, sock[x].host, sock[x].dport);
    }
    else
    {
      sock_failed(x);
      return(-1);
    }
  }
  else
    sock_estab(x);
  return(x);
}

int
sock_new(const char *host, const char *port, const int type, const void (*readfunc)(const int, char *), const void (*estabfunc)(const int), const void (*closefunc)(const int))
{
  struct addrinfo *resolved, *rt;
  int x = 0;

  for(;x < SOCK_MAX && sock[x].state != SS_UNUSED; x++);

  if(x == SOCK_MAX)
  {
    logger(C_SOCK, CRIT, "sock_new", "out of sockets connecting to %s:%s", host, port);
    return(-1);
  }

  if((resolved = ip_resolve(host, port)) == NULL)
  {
    logger(C_SOCK, WARN, "sock_new", "connection to %s:%d aborted (resolver failed)", host, port);
    return(-1);
  }

  /* loop through linked list of results - could be multiple layer-3 protos */
  for(rt = resolved; rt; rt = rt->ai_next)
    if((sock[x].fd = socket(rt->ai_family, rt->ai_socktype, rt->ai_protocol)) != -1)
      break;

  if(!rt)
  {
    logger(C_SOCK, CRIT, "sock_new", "slot #%d: error allocating socket (errno %d:%s)", x, errno, strerror(errno));
    freeaddrinfo(resolved);
    exit_program("sock_new", FAIL);
  }

  memcpy((void *)&sock[x].addr, (void *)rt, sizeof(struct addrinfo));
  freeaddrinfo(resolved);

  sock[x].timer_io = sock[x].timer_begin = global.now;
  sock[x].type = type;
  sock[x].read_bytes = sock[x].write_bytes = 0L;
  sock[x].ins = sock[x].in;
  sock[x].dport = atoi(port);

  sock[x].read = readfunc;
  sock[x].estab = estabfunc;
  sock[x].close = closefunc;

  inet_ntop(sock[x].addr.ai_family, (void *)&sock[x].addr.ai_addr, sock[x].host, sock[x].addr.ai_addrlen);

  return(sock_connect(x));
}

void
sock_hook(void)
{
  unsigned short int rd_fds = 0, wr_fds = 0;
  struct timeval timeout;
  fd_set rd, wr;
  int x = 0;

  FD_ZERO(&rd);
  FD_ZERO(&wr);

  timeout.tv_usec = 250000;
  timeout.tv_sec = 0;

  for(; x < SOCK_MAX; x++)
  {
    if(sock[x].state == SS_UNUSED)
      continue;

    if(sock[x].state == SS_INPROG)
    {
      wr_fds++;
      FD_SET(sock[x].fd, &wr);
    }

    if(sock[x].state == SS_ESTAB)
    {
      rd_fds++;
      FD_SET(sock[x].fd, &rd);
    }
  }

  logger(C_SOCK, DEBUG3, "sock_hook", "calling select() with %hu read fds %hu write fds", rd_fds, wr_fds);

  select(global.conf.tablesize, &rd, &wr, NULL, &timeout);

  global.now = time(NULL);

  for(x = 0; x < SOCK_MAX; x++)
  {
    if(sock[x].state == SS_INPROG && FD_ISSET(sock[x].fd, &wr))
    {
      socklen_t socklen = sizeof(int);
      int sockstat = 0, getsockstat = 0;

      if((getsockstat = getsockopt(sock[x].fd, SOL_SOCKET, SO_ERROR, (void *)&sockstat, (socklen_t *)&socklen)) == 0)
      {
        if(sockstat)
        {
          errno = sockstat;
          sock_failed(x);
        }
        else
          sock_estab(x);
      }
      else
      {
        logger(C_SOCK, CRIT, "sock_hook", "getsockopt() failed (errno %d:%s)", getsockstat, errno, strerror(errno));
        exit_program("sock_hook", FAIL);
      }
    }

    if( (sock[x].state == IRC_ESTAB || sock[x].state == IRC_ACTIVE) && FD_ISSET(sock[x].fd, &rd) )
      sock_read(x);
  }
}

void
sock_cleanup(void)
{
  int x = 0;

  for(; x < SOCK_MAX; x++)
  {
    if(sock[x].fd != -1)
    {
      close(sock[x].fd);
      sock[x].fd = -1;
    }
  }
}

void
sock_init(void)
{
  int x = 0;

  memset((void *)sock, 0, sizeof(struct sockets) * SOCK_MAX);
  for(; x < SOCK_MAX; x++)
  {
    sock[x].state = SS_UNUSED;
    sock[x].fd = -1;
  }

  logger(C_ANY, INFO, "sock_init", "tcp/ip sockets initialized (max sockets: %d)", SOCK_MAX);
}
