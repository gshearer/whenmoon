#define WM_MISCC
#include "misc.h"

void
banner(void)
{
  puts("\n    ▄▄▌ ▐ ▄▌ ▄ .▄▄▄▄ . ▐ ▄ • ▌ ▄ ·.              ▐ ▄ \n"
       "    ██· █▌▐███▪▐█▀▄.▀·•█▌▐█·██ ▐███▪▪     ▪     •█▌▐█\n"
       "    ██▪▐█▐▐▌██▀▐█▐▀▀▪▄▐█▐▐▌▐█ ▌▐▌▐█· ▄█▀▄  ▄█▀▄ ▐█▐▐▌\n"
       "    ▐█▌██▐█▌██▌▐▀▐█▄▄▌██▐█▌██ ██▌▐█▌▐█▌.▐▌▐█▌.▐▌██▐█▌\n"
       "     ▀▀▀▀ ▀▪▀▀▀ · ▀▀▀ ▀▀ █▪▀▀  █▪▀▀▀ ▀█▄▀▪ ▀█▄▀▪▀▀ █▪\n\n"
       "              Cryptocurrency Trading Bot\n"
       "     Written by George Shearer (george@shearer.tech)\n\n");
}

int
numargs(const char *str)
{
  int numargs = (str != NULL && *str != '\0' && *str != ' ') ? 1 : 0;
  const char *a = str;

  while((a = strchr(a, ' ')))
  {
    numargs++;
    a++;
  }
  return(numargs);
}

void
d2h(char *storage, const double n)
{
  int decimals = 3;
  const char *units[] = { "", "K", "M", "B", "T" , "Q" };
  const char *fmt[] = { "%1.*lf%s", "%1.*lf" };
  const char *unit;

  int digits = (n == 0) ? 0 : 1 + floor(log10l(fabs(n)));

  int exp = (digits <= 4) ? 0 : 3 * ((digits - 1) / 3);

  double m = n / powl(10, exp);

  if(m - (long int)n == 0)
    decimals = 0;

  unit = ((exp / 3) < 6) ? units[exp / 3] : "?";

  sprintf(storage, fmt[exp<3], decimals, m, unit);
}

const char *
logchan(const unsigned int chan)
{
  int x = 0;

  for(; true; x++)
  {
    if(logchans[x].name == NULL)
      return("UNDEFINED");
    if(logchans[x].chan == chan)
      return(logchans[x].name);
  }
}

const char *
logsev(const unsigned int sev)
{
  int x = 0;

  for(; true; x++)
  {
    if(logsevs[x].name == NULL)
      return("UNDEFINED");
    if(logsevs[x].severity == sev)
      return(logsevs[x].name);
  }
}

void
logger(const unsigned int chan, const int sev, const char *func, const char *format, ...)
{
  if(!(global.state & WMS_BT) && (global.state & WMS_END))
    pthread_mutex_lock(&mutex_logger);

  if(((global.conf.logchans & chan) || chan == 0) && global.conf.logsev >= sev)
  {
    va_list ptr;
    time_t now;

    if(global.state & WMS_BT)
      global.now = now = time(NULL);
    else
    {
      if(!(global.state & WMS_END))
        pthread_mutex_lock(&mutex_globalnow);
      now = global.now;
      pthread_mutex_unlock(&mutex_globalnow);
    }

    va_start(ptr, format);

    printf("[%lu %s%6.6s%s] %s(): ", now, 
        (sev == CRIT) ? RED : (sev == WARN) ? YELLOW : "",
        logsev(sev),
        (sev == CRIT || sev == WARN) ? ARESET : "",
        func);

    vprintf(format, ptr);
    puts("");
    va_end(ptr);
  }
  if(!(global.state & WMS_BT))
    pthread_mutex_unlock(&mutex_logger);
}

const char *
signal_value(const int sig) {
  switch(sig) {
    case SIGCHLD:
      return("SIGCHLD");
    case SIGTERM:
      return("SIGTERM");
    case SIGHUP:
      return("SIGHUP");
    case SIGINT:
      return("SIGINT");
    case SIGBUS:
      return("SIGBUS");
    case SIGSEGV:
      return("SIGSEGV");
    case SIGPIPE:
      return("SIGPIPE");
    case SIGABRT:
      return("SIGABRT");
    default:
      return("UNKNOWN");
  }
}

void
signal_catcher(int sig)
{
  logger(C_MISC, CRIT, "signal_catcher", "received signal %s (%d)", signal_value(sig), sig);

  if(sig == SIGCHLD)
    return;

  global.state |= WMS_END;

  exit_program("signal_catcher", FAIL);
}

void
set_signals(void)
{
  signal(SIGTERM, signal_catcher);
  signal(SIGHUP,  signal_catcher);
  signal(SIGTERM, signal_catcher);
  signal(SIGINT,  signal_catcher);
  signal(SIGBUS,  signal_catcher);
  signal(SIGABRT, signal_catcher);
  signal(SIGSEGV, signal_catcher);
  signal(SIGPIPE, signal_catcher);
  signal(SIGCHLD, signal_catcher);
}

void
mkupper(char *str)
{
  char *x = str;

  while(*x != '\0')
  {
    *x = toupper(*x);
    x++;
  }
}

void
url_encode_string(const char *str, const char *storage)
{
  unsigned char hex[] = "0123456789ABCDEF";
  register int i, j, len;
  unsigned char *tmp;

  len = strlen(str);
  tmp = (unsigned char *)storage;

  for (i = 0, j = 0; i < len; i++, j++)
  {
    tmp[j] = (unsigned char)str[i];
    if (tmp[j] == ' ')
      tmp[j] = '+';
    else if (!isalnum(tmp[j]) && strchr("_-.", tmp[j]) == NULL)
    {
      tmp[j++] = '%';
      tmp[j++] = hex[(unsigned char)str[i] >> 4];
      tmp[j] = hex[(unsigned char)str[i] & 0x0F];
    }
  }

  tmp[j] = '\0';
}

void
salt_random(void)
{
  FILE *fp = fopen("/dev/urandom", "r");

  if(fp != NULL)
  {
    srand(global.now + fgetc(fp));
    fclose(fp);
  }
  else
  {
    logger(C_MISC, WARN, "salt_random", "unable to access /dev/urandom");
    srand(global.now);
  }
}

size_t
base64_encode(char *out, const unsigned char *in, size_t in_len)
{
  size_t out_len = 4 * ((in_len + 2) / 3);
  int i = 0, j = 0;

  while(i < in_len)
  {
    uint32_t octet_a = (i < in_len) ? in[i++] : 0;
    uint32_t octet_b = (i < in_len) ? in[i++] : 0;
    uint32_t octet_c = (i < in_len) ? in[i++] : 0;
    uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

    out[j++] = b64_encode[(triple >> 3 * 6) & 0x3F];
    out[j++] = b64_encode[(triple >> 2 * 6) & 0x3F];
    out[j++] = b64_encode[(triple >> 1 * 6) & 0x3F];
    out[j++] = b64_encode[(triple >> 0 * 6) & 0x3F];
  }

  for(i = 0; i < b64_mod[in_len % 3]; i++)
    out[out_len - 1 - i] = '=';
  return(out_len);
}

size_t
base64_decode(unsigned char *out, const char *in, size_t in_len)
{
  int i = 0, j = 0;
  size_t out_len;

  if(in_len % 4 != 0)
    return(0);

  out_len = in_len / 4 * 3;

  if(in[in_len - 1] == '=')
    out_len--;

  if(in[in_len - 2] == '=')
    out_len--;

  while(i < in_len)
  {
    uint32_t sextet_a = (in[i] == '=') ? 0 & i++ : b64_decode[(int)in[i++]];
    uint32_t sextet_b = (in[i] == '=') ? 0 & i++ : b64_decode[(int)in[i++]];
    uint32_t sextet_c = (in[i] == '=') ? 0 & i++ : b64_decode[(int)in[i++]];
    uint32_t sextet_d = (in[i] == '=') ? 0 & i++ : b64_decode[(int)in[i++]];
    uint32_t triple = (sextet_a << 3 * 6) + (sextet_b << 2 * 6) + (sextet_c << 1 * 6) + (sextet_d << 0 * 6);

    if(j < out_len)
      out[j++] = (triple >> 2 * 8) & 0xFF;

    if(j < out_len)
      out[j++] = (triple >> 1 * 8) & 0xFF;

    if(j < out_len)
      out[j++] = (triple >> 0 * 8) & 0xFF;
  }
  return(out_len);
}

void
time2str(char *storage, const time_t when, const int localtime)
{
  struct tm tm_temp;

  if(localtime == true)
    localtime_r(&when, &tm_temp);
  else
    gmtime_r(&when, &tm_temp);
  strftime(storage, 49, "%FT%T.0Z", &tm_temp);
}

void
time2strf(char *storage, const char *fmt, const time_t when, const int localtime)
{
  struct tm tm_temp;

  if(localtime == true)
    localtime_r(&when, &tm_temp);
  else
    gmtime_r(&when, &tm_temp);
  strftime(storage, 49, fmt, &tm_temp);
}

// update clock
// note that this must be mutex'd because curl threads access global.now
void
update_clock(void)
{
  pthread_mutex_lock(&mutex_globalnow);
  global.now = time(NULL);
  pthread_mutex_unlock(&mutex_globalnow);
}

const char *
cint2str(const int minutes)
{
  switch(minutes)
  {
    case 1:
      return("1-min");
    case 5:
      return("5-min");
    case 15:
      return("15-min");
    case 30:
      return("30-min");
    case 60:
      return("1-hour");
    case 240:
      return("4-hour");
    case 1440:
      return("1-day");
    default:
      return("custom");
  }
}

char *
trim0(char *str, const int size, const double x)
{
  char *y;
  snprintf(str, size, "%.11f", x);
  y = str + strlen(str) - 1;
  for(; *y && *y == '0' && *y != '.'; y--);
  if(*y == '.')
    *(y + 2) = '\0';
  else
    *(y + 1) = '\0';
  return(str);
}

const char *
mkt_rate(double value)
{
  if(value <= -5)  return("rekt");
  if(value <= -2)  return("awful");
  if(value <= 0)   return("bad");
  if(value <= 2)   return("good");
  if(value <= 5)   return("great");
  return("moon");
}

int
cint_length(int cint)
{
  switch(cint)
  {
    case 0: return(1);
    case 1: return(5);
    case 2: return(15);
    case 3: return(30);
    case 4: return(60);
    case 5: return(240);
    case 6: return(1440);
  }
}

const char *
stopwatch_time(const time_t secs)
{
  int hours, minutes, seconds;

  hours = secs / 60 / 60;
  minutes = (secs - (hours * 60 * 60)) / 60;
  seconds = (secs - ((hours * 60 * 60) + (minutes * 60)));

  snprintf(global.ctmp, 49, "%02hu:%02hu:%02hu", hours, minutes, seconds);
  return(global.ctmp);
}

const char *
find_cpuname(void)
{
  FILE *fp = fopen("/proc/cpuinfo", "r");
  char *s = global.ctmp;

  if(fp != NULL)
  {
    while(!feof(fp) && fgets(s, 99, fp))
    {
      if(strstr(s, "model name") != NULL)
      {
        char *t = strchr(s, ':') + 2;

        s[strlen(s) - 1] = '\0';
        fclose(fp);
        return(t);
      }
    }
    fclose(fp);
  }
  return("unknown");
}

const char *
human_size(const size_t bytes)
{
  unsigned long int tib = pow(1024, 4);
  const char *a;
  float result;

  if(bytes >= tib)
  {
    a = "TiB";
    result = bytes / tib;
  }

  else if(bytes >= 1024 *1024 *1024)
  {
    a = "GiB";
    result = bytes / 1024 / 1024 / 1024;
  }

  else if(bytes >= 1024 * 1024)
  {
    a = "MiB";
    result = bytes / 1024 / 1024;
  }

  else
  {
    a = "KiB";
    result = bytes / 1024;
  }

  snprintf(global.ctmp, 49, "%.2f %s", result, a);
  return(global.ctmp);
}

int
find_range(char *i, struct exp_arg *a)
{
  char *x = strchr(i, ':');

  if(x != NULL)
  {
    char *y = strchr(x + 1, ':');

    *x = '\0';

    if(y != NULL)
    {
      *y = '\0';

      a->cur = a->start = strtof(i, NULL);
      a->step  = strtof(x + 1, NULL);
      a->end   = strtof(y + 1, NULL);
      a->steps = abs((a->end - a->start) / a->step) + 1;

      return(SUCCESS);
    }
  }
  return(FAIL);
}

void
expargs(struct exp_arg *args, int argc, int sarg, const void (*callback)(int, float *))
{
  float argf[argc];
  int x = sarg;

  for(; x < argc; x++)
  {
    struct exp_arg *a = &args[x];

    if(a->steps)
    {
      while(a->steps-- > 0)
      {
        expargs(args, argc, x + 1, callback);
        a->cur += a->step;
      }
      a->cur = a->start;
      a->steps = abs((a->end - a->start) / a->step) + 1;
      return;
    }
  }

  for(x = 0; x < argc; x++)
    argf[x] = args[x].cur;

  callback(argc, argf);
}

int
expstr(char *input, const void (*callback)(int, float *))
{
  struct exp_arg args[50];
  char *s = input;
  int argc = 0;

  memset((void *)args, 0, sizeof(struct exp_arg) * 50);

  while(*s != '\0' && argc < 50)
  {
    char *t;

    while(*s != '\0' && *s == ' ') s++; // ignore extra spaces
    if(*s == '\0' || *s == '\r' || *s == '\n') // EOL
      break;

    if((t = strchr(s, ' ')) != NULL)
      *t = '\0';

    if(strchr(s, ':') != NULL)
    {
      if(find_range(s, &args[argc]) == FAIL)
        return(FAIL);
    }
    else
      args[argc].cur = strtof(s, NULL);

    argc++;

    if(t == NULL)
      break;
    s = t + 1;
  }

  expargs(args, argc, 0, callback);
  return(SUCCESS);
}
