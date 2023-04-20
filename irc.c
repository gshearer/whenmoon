#define WM_IRCC
#include "irc.h"

int
irc_find_chan(const int i, const char *n)
{
  int x = 0;

  for(; x < IRC_MAXCHANS; x++)
    if(strcasecmp(n, irc[i].chan[x]) == 0)
      return(x);
  return(-1);
}

static int
irc_findSock(int sock)
{
  int x = 0;
  for(; x < IRC_MAX && irc[x].sock != sock; x++);
  return((x == IRC_MAX) ? -1 : x);
}

static void
irc_failed(const int sock)
{
  int myerrno = errno;
  int x = irc_findSock(sock);

  irc[x].state = IRC_NEW;
  irc[x].failed++;
  irc[x].sock = -1;
  irc[x].timer = global.now;

  if(myerrno)
    logger(C_IRC, WARN, "irc_failed", "[slot %d] connection to %s:%s failed: %s", x, irc[x].server, irc[x].port, strerror(myerrno));
  else
    logger(C_IRC, WARN, "irc_failed", "[slot %d] connection to %s:%s closed", x, irc[x].server, irc[x].port);
}

int
irc_admin(void)
{
  int x = global.is, y = 0;
  char nick[20], *t;

  return(true); // TODO: redo this
  strncpy(nick, irc[x].from, 19);

  if((t = strchr(nick, '!')))
    *t = '\0';

  for(;y < IRC_MAXADMINS; y++)
  {
    if(irc[x].admin[y].nick[0] == '\0')
      continue;
    if(!strcasecmp(irc[x].admin[y].nick, nick))
      return(true);
  }
  return(false);
}

int
irc_addAdmin(const char *who)
{
  int x = 0;

  for(; x < IRC_MAXADMINS && irc[global.is].nick[x][0] != '\0'; x++);

  if(x < IRC_MAXADMINS)
  {
    char *t = strchr(who, '!');

    if(t == NULL)
      return(-1);

    *t = '\0';

    strncpy(irc[global.is].admin[x].nick, who, 19);
    irc[global.is].admin[x].timer = global.now;
    logger(C_IRC, CRIT, "irc_add_admin", "%s recognized as an admin on %s:%s", who, irc[global.is].server, irc[global.is].port);
    return(x);
  }
  logger(C_IRC, CRIT, "irc_add_admin", "max admins reached (%s on %s:%s)", who, irc[global.is].server, irc[global.is].port);
  return(-1);
}

void
irc_broadcast(const char *format, ...)
{
  char buf[1000];
  va_list ptr;
  int x = 0;

  va_start(ptr, format);
  vsnprintf(buf, 999, format, ptr);
  va_end(ptr);

  for(;x < IRC_MAX; x++)
  {
    if(irc[x].state == IRC_ACTIVE)
    {
      int y = 0;
      for(;y < IRC_MAXCHANS; y++)
        if(irc[x].chan[y][0] == '#' || irc[x].chan[y][0] == '&')
          sock_write(irc[x].sock, "PRIVMSG %s :%s", irc[x].chan[y], buf);
    }
  }
}

static void
irc_checkAdmins(void)
{
  int x = 0;

  for(;x < IRC_MAX; x++)
  {
    int y = 0;

    for(; y < IRC_MAXADMINS; y++)
    {
      if(irc[x].admin[y].nick[0] != '\0' && global.now - irc[x].admin[y].timer >= IRC_ADMINTIME)
      {
        irc_broadcast("Admin privs have expired for %s.", irc[x].admin[y].nick);
        irc[x].admin[y].timer = (time_t)0;
        irc[x].admin[y].nick[0] = '\0';
      }
    }
  }
}

void
irc_reply(const char *format, ...)
{
  int x = global.is;
  char buf[1000];
  va_list ptr;

  if(irc[x].reply != NULL)
  {
    snprintf(buf, 999, "PRIVMSG %s :", irc[x].reply);

    va_start(ptr, format);
    vsnprintf(buf + strlen(buf), 998 - strlen(buf) - 1, format, ptr);
    va_end(ptr);

    logger(C_IRC, DEBUG2, "irc_reply", "%s:%s >>%s<<", irc[x].server, irc[x].port, buf);

    strcat(buf, "\n");

    sock_write(irc[x].sock, "%s", buf);
  }
}

void
irc_reply_str(const char *in)
{
  char temp[1000], *t_start = temp, *t_end = temp + 1000, *t_cur;

  strncpy(temp, in, 999);

  while(t_start < t_end && *t_start != '\0')
  {
    char *x = strchr(t_start, '\n');

    if(x == NULL)
      break;

    *x = '\0';
    if(strlen(t_start))
      irc_reply("%s", t_start);
    t_start = x + 1;
  }

  if(*t_start != '\0')
    irc_reply("%s", t_start);
}

void
irc_reply_who(const int x, const char *who, const char *format, ...)
{
  char buf[10000];
  va_list ptr;

  if(who != NULL)
  {
    snprintf(buf, 9999, "PRIVMSG %s :", who);

    va_start(ptr, format);
    vsnprintf(buf + strlen(buf), 9998 - strlen(buf) - 1, format, ptr);
    va_end(ptr);

    logger(C_IRC, DEBUG2, "irc_reply", "%s:%s >>%s<<", irc[x].server, irc[x].port, buf);

    strcat(buf, "\n");

    sock_write(irc[x].sock, "%s", buf);
  }
}

void
irc_signoff(const char *msg)
{
  int x = 0;

  logger(C_IRC, DEBUG1, "irc_signoff", "request to close server (%s)", msg);

  for(; x < IRC_MAX; x++)
  {
    if(irc[x].state == IRC_ACTIVE)
      sock_write(irc[x].sock, "QUIT :%s", global.conf.verstring);
  }
}

static void
ircd_start(const int x, char *from, char *left)
{
  int y = 0;
  char *nick = irc[x].nick[irc[x].nicknum];

  logger(C_IRC, DEBUG3, "ircd_start", "from: %s left: %s session: %s:%s", from, left, irc[x].server, irc[x].port);

  irc[x].state = IRC_ACTIVE;
  irc[x].timer = global.now;
  strcpy(irc[x].curnick, nick);

  if(irc[x].opername[0] != '\0' && irc[x].operpass[0] != '\0')
  {
    sock_write(irc[x].sock, "OPER %s %s", irc[x].opername, irc[x].operpass);
    sock_write(irc[x].sock, "MODE %s +F", nick);
  }

  for(; y < IRC_MAXCHANS && irc[x].chan[y][0] != '\0'; y++)
    if(irc[x].chankey[y][0] != '\0')
      sock_write(irc[x].sock, "JOIN %s :%s", irc[x].chan[y], irc[x].chankey[y]);
    else
      sock_write(irc[x].sock, "JOIN %s", irc[x].chan[y]);
}

static void
ircd_ping(int x, char *from, char *left)
{
  sock_write(irc[x].sock, "PONG :%s", left);
  irc[x].timer = global.now;
}

static void
ircd_pong(int x, char *from, char *left)
{
  irc[x].timer = global.now;
}

static void
ircd_error(int x, char *from, char *left)
{
  logger(C_IRC, WARN, "ircd_error", "server %s:%s sent: %s", irc[x].server, irc[x].port, left);
}

static void
ctcp_ping(int x, char *replyto, char *args)
{
  if(args)
    sock_write(irc[x].sock, "NOTICE %s :\001PING %s\001", replyto, args);
  else
    sock_write(irc[x].sock, "NOTICE %s :\001ERRMSG PING :Your CTCP PING didn't provide a timestamp.\001", replyto);
}

static void
ctcp_version(int x, char *replyto, char *args)
{
  sock_write(irc[x].sock, "NOTICE %s :\001VERSION %s\001", replyto, global.conf.verstring);
}

static void
ctcp_clientinfo(int x, char *replyto, char *args)
{
  sock_write(irc[x].sock, "NOTICE %s :\001CLIENTINFO QuoteBot supports VERSION CLIENTINFO PING\001", replyto);
}

static void
irc_setnick(int x, int n)
{
  irc[x].nicktimer = global.now;
  sock_write(irc[x].sock, "NICK :%s", irc[x].nick[n]);
}

static void
ircd_badnick(int x, char *from, char *left)
{
  int n;

  if(irc[x].state == IRC_ACTIVE)
    return;

  n = irc[x].nicknum;

  if(irc[x].nick[n+1][0] == '\0')
  {
    logger(C_IRC, CRIT, "ircd_badnick", "out of backup nicknames - giving up");
    errno = 0;
    irc_failed(x);
    return;
  }

  irc[x].nicknum = ++n;
  logger(C_IRC, WARN, "ircd_badnick", "trying next nickname '%s'", irc[x].nick[n]);
  irc_setnick(x, n);
}

static void
ircd_nick(int x, char *from, char *left)
{
  char *y = strchr(from, '!');

  if(y != NULL)
  {
    *y = '\0';
    if(!strcasecmp(from, irc[x].curnick)) /* my nick changed */
    {
      int z = 0;

      strncpy(irc[x].curnick, left, 19);

      for(;z < IRC_MAXNICKS && strncasecmp(left, irc[x].nick[z], 19); z++);
      if(z < IRC_MAXNICKS)
      {
        irc[x].nicknum = z;
        logger(C_IRC, INFO, "ircd_nick", "confirmed new nick %s on %s:%d", left, irc[x].server, irc[x].port);
      }
      else
        logger(C_IRC, WARN, "ircd_nick", "new nick '%s' is not in config for %s:%d", left, irc[x].server, irc[x].port);
    }
  }
}

static void
ircd_ctcp(int x, char *cmd, char *t, char *reply, char *args)
{
  int command = 0;

  struct {
    const char *cmd;
    const void (*func)(int, char *, char *);
  } ctcp_cmds[] = {
    { "PING",       (void *)ctcp_ping },
    { "CLIENTINFO", (void *)ctcp_clientinfo },
    { "VERSION",    (void *)ctcp_version },
    { NULL, NULL }
  };

  cmd = t + 1;
  if((t = strchr(cmd, '\001')))
    *t = '\0';
  else if(args && (t = strchr(args, '\001')))
    *t = '\0';
  else
    return;

  for(;ctcp_cmds[command].cmd; command++)
  {
    if(!strncasecmp(ctcp_cmds[command].cmd, cmd, 199))
    {
      logger(C_IRC, DEBUG3, "ircd_ctcp", "client-to-client request from '%s' (%s)", reply, args);
      ctcp_cmds[command].func(x, reply, args);
      break;
    }
  }
  return;
}

static void
ircd_msg(int x, char *from, char *left)
{
  char *args, *reply = NULL, *cmd = NULL, *t;
  int command = 0, argcount = 0;

  if((t = strchr(left, ' ')))
  {
    *t++ = '\0';
    t = (*t == ':') ? t + 1 : t;

    if((cmd = strchr(t, ' ')))
    {
      args = cmd + 1;
      argcount = numargs(args);
    }
    else
      args = NULL;

    global.is = x; /* which IRC server req came in on */
    irc[x].from = from;
    irc[x].reply = left;
    irc[x].args = args;

    if(strncasecmp(left, irc[x].nick[irc[x].nicknum], 20))
    {
      irc[x].msgtype = IRC_PUBLIC;
      if(*t != '!')
        return;
      reply = left;
    }
    else
    {
      irc[x].msgtype = IRC_PRIVATE;
      irc[x].reply = reply = from;
    }

    if(cmd)
      *cmd = '\0';

    if(*t == '\001')
    {
      ircd_ctcp(x, cmd, t, reply, args);
      return;
    }

    cmd = (*t == '!') ? t + 1 : t;

    for(;user_cmds[command].cmd != NULL; command++)
    {
      if( !strncasecmp(user_cmds[command].cmd, cmd, 30)
          || (user_cmds[command].alias != NULL
          && !strncasecmp(user_cmds[command].alias, cmd, 5)) )
      {
        if(user_cmds[command].type == IRC_PUBLIC && irc[x].msgtype == IRC_PRIVATE)
        {
          irc_reply("This command doesn't work publicly :~(?");
          return;
        }

        if(user_cmds[command].type == IRC_PRIVATE && irc[x].msgtype == IRC_PUBLIC)
        {
          irc_reply("This command doesn't work privately :~(?");
          return;
        }

        if(argcount < user_cmds[command].args)
        {
          irc_reply("Usage: %s", user_cmds[command].usage);
          return;
        }

        if(user_cmds[command].admin == true && irc_admin() == false)
        {
          irc_reply("No. :~(?");
          logger(C_IRC, CRIT, "ircd_msg", "%s is not an admin but attempted: %s %s", from, cmd, (args == NULL) ? "none" : args);
          return;
        }

        logger(C_IRC, INFO, "ircd_msg", "%s:%d user command '%s' from '%s' (args: %s)", irc[x].server, irc[x].port, cmd, from, (args == NULL) ? "none" : args);

        user_cmds[command].func();
        irc[x].from[0] = irc[x].reply[0] = '\0';
        irc[x].args = NULL;
        global.is = -1;
        break;
      }
    }
  }
}

static void
ircd_unreg(int x, char *from, char *left)
{
  logger(C_IRC, CRIT, "ircd_unreg", "irc server %s:%d rejected registration attempt", irc[x].server, irc[x].port);
  errno = 0;
  irc_failed(x);
}

static void
irc_read(const int sock, char *buf)
{
  char *from, *cmd, *left, *stack = buf;
  int x = irc_findSock(sock);

  struct {
    const char *cmd;
    const void (*func)(int, char *, char *);
  } irc_proto[] = {
    { "001",     (void *)ircd_start },
    { "432",     (void *)ircd_badnick },
    { "433",     (void *)ircd_badnick },
    { "451",     (void *)ircd_unreg },
    { "PING",    (void *)ircd_ping },
    { "PONG",    (void *)ircd_pong },
    { "NICK",    (void *)ircd_nick },
    { "ERROR",   (void *)ircd_error },
    { "PRIVMSG", (void *)ircd_msg },
    { NULL, NULL }
  };

  if(*stack == ':')
  {
    if((cmd = strchr(stack, ' ')) == NULL)
      return;
    *cmd++ = '\0';
    from = stack + 1;
  }
  else
  {
    cmd = stack;
    from = NULL;
  }

  logger(C_IRC, DEBUG4, "irc_read", "from = >%s< cmd = >%s<", (from == NULL) ? "NULL" : from, cmd);

  if((left = strchr(cmd, ' ')))
  {
    unsigned short int command = 0;

    *left++ = '\0';
    left = (*left == ':') ? left + 1 : left;

    for(; irc_proto[command].cmd; command++)
    {
      if(!strncasecmp(irc_proto[command].cmd, cmd, 199))
      {
        irc_proto[command].func(x, from, left);
        return;
      }
    }
  }
}

static void
irc_estab(const int sock)
{
  int x = irc_findSock(sock);

  irc[x].state = IRC_ESTAB;
  irc[x].begin = global.now;

  logger(C_IRC, INFO, "irc_estab", "session to %s:%s established", irc[x].server, irc[x].port);

  sock_write(sock, "USER %s %s %s :QuoteBot pid %d ver %s", irc[x].nick[0], irc[x].nick[0], irc[x].nick[0], getpid(), global.conf.version);
  irc_setnick(x, 0);
}

void
irc_init(void)
{
  int x = 0;

  memset((void *)irc, 0, sizeof(struct ircserver) * IRC_MAX);
  for(x = 0; x < IRC_MAX; x++)
  {
    irc[x].state = IRC_UNUSED;
    irc[x].sock = -1;
  }
  
  logger(C_ANY, INFO, "irc_init", "internet relay chat initialized (max servers: %d)", IRC_MAX);
}

void
irc_hook(void)
{
  int x = 0;

  for(; x < IRC_MAX; x++)
  {
    if(irc[x].state == IRC_UNUSED)
      continue;

    if(irc[x].state == IRC_NEW)
    {
      if((global.now - irc[x].timer) >= IRC_HOLDDOWN)
      {
        irc[x].timer = global.now;
        irc[x].sock = sock_new(irc[x].server, irc[x].port, ST_IRC, irc_read, irc_estab, irc_failed);
      }
      continue;
    }

    if(irc[x].state == IRC_ESTAB)
    {
      if(global.now - irc[x].timer >= IRC_REGTIMEOUT)
      {
        logger(C_IRC, WARN, "irc_hook", "timeout waiting for registration");
        irc_failed(x);
        continue;
      }
    }

    if(irc[x].state == IRC_ACTIVE)
    {
      if(irc[x].nicknum != 0 && global.now - irc[x].nicktimer >= IRC_NICKTIME)
        irc_setnick(x, 0);
    }
  }

  irc_checkAdmins();
}
