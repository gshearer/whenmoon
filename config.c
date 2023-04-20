#define WM_CONFIGC
#include "config.h"

int
read_config(const char *path)
{
  FILE *fp;
  int linenum = 0, isrv = -1, chan = 0, nick = 0;
  char buf[1000], *t;

  if((fp = fopen(path, "r")) == NULL)
  {
    logger(C_CONF, CRIT, "read_config", "Could not open \"%s\" for read.", path);
    return(FAIL);
  }

  while(!feof(fp) && fgets(buf, 1000, fp))
  {
    char *last;
    linenum++;

    if(buf[0] == '#')
      continue;

    last = buf + strlen(buf) - 1;
    *(last--) = '\0';

    if((t = strchr(buf, '=')))
    {
      char *y;

      *t = '\0';

      if((y = strchr(t + 1, '"')))
      {
        y++;

        if(*last == '"')
        {
          int x;

          *last = '\0';

          for(x = 0; cd[x].key != NULL && strncasecmp(buf, cd[x].key, 999); x++);

          if(cd[x].key == NULL)
          {
            logger(C_CONF, WARN, "read_config", "unknown directive '%s' at line %d (ignored)", buf, linenum);
            continue;
          }

          logger(C_CONF, DEBUG3, "read_config", "matched key '%s' with value '%s'", buf, y);

          switch(x)
          {
            case IRCSERVER:
              strncpy(irc[++isrv].server, y, 199);
              irc[isrv].state = IRC_NEW;
              nick = chan = 0;
              irc[isrv].timer = irc[isrv].begin = (time_t)0;
              irc[isrv].failed = 0;
              irc[isrv].sock = -1;
              irc[isrv].from = irc[isrv].reply = irc[isrv].args = NULL;
              irc[isrv].nicknum = 0;
              break;
            case IRCPORT:
              strncpy(irc[isrv].port, y, 9);
              break;
            case IRCOPERNAME:
              strncpy(irc[isrv].opername, y, 19);
              break;
            case IRCOPERPASS:
              strncpy(irc[isrv].operpass, y, 49);
              break;
            case IRCCHAN:
              strncpy(irc[isrv].chan[chan++], y, 39);
              break;
            case IRCCHANKEY:
              strncpy(irc[isrv].chankey[chan], y, 199);
              break;
            case IRCNICK:
              strncpy(irc[isrv].nick[nick++], y, 19);
              break;
            case LOGCHANS:
              global.conf.logchans = atoi(y);
              break;
            case LOGSEV:
              global.conf.logsev = atoi(y);
              break;
            case LOGPATH:
              strncpy(global.conf.logpath, y, 199);
              break;
            case DBHOST:
              strncpy(global.conf.dbhost, y, 199);
              break;
            case DBPORT:
              global.conf.dbport = atoi(y);
              break;
            case DBUSER:
              strncpy(global.conf.dbuser, y, 199);
              break;
            case DBPASS:
              strncpy(global.conf.dbpass, y, 199);
              break;
            case DBNAME:
              strncpy(global.conf.dbname, y, 199);
              break;
            case ADMINPASS:
              strncpy(global.conf.adminpass, y, 199);
              break;
            case CBPKEY:
              strncpy(global.conf.cbp_key, y, 199);
              break;
            case CBPPASSPHRASE:
              strncpy(global.conf.cbp_passphrase, y, 199);
              break;
            case CBPSECRET:
              strncpy(global.conf.cbp_secret, y, 199);
              break;
            case CPTOKEN:
              strncpy(global.conf.cptoken, y, 199);
              break;
            case CBPPROFILE:
              strncpy(global.conf.cbp_profile, y, 199);
              break;
            default:
              logger(C_CONF, WARN, "read_config", "error on line %d (x == %d key == %s)", linenum, x, cd[x].key);
              break;
          }
        }
      }
    }
  }

  fclose(fp);

  logger(C_CONF, INFO, "read_config", "read %d lines from %s", linenum, path);

  if(isrv == -1)
  {
    logger(C_CONF, CRIT, "read_config", "no irc servers defined");
    exit_program("read_config", FAIL);
  }

  return(SUCCESS);
}
