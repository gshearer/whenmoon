#define WM_CPANICC
#include "cpanic.h"

void
cpanic_curl_callback(const int x)
{
  enum json_tokener_error jerr = json_tokener_success;
  json_object *cpanic_json;
  int y = 0;

  if(cq[x].recv == NULL || cq[x].state == CQS_FAIL)
  {
    irc_reply_who(cq[x].irc, cq[x].reply, "Query failed: %s", cq[x].errmsg);
    return;
  }

  cpanic_json = json_tokener_parse_verbose(cq[x].recv, &jerr);

  if(jerr == json_tokener_success)
  {
    json_object *news_array = json_object_object_get(cpanic_json, "results");

    if(json_object_is_type(news_array, json_type_array) == true)
    {
      int num = json_object_array_length(news_array);

      while(y < num && y < MAX_CPNEWS)
      {
        json_object *tmp = json_object_array_get_idx(news_array, y);

        if(tmp != NULL)
        {
          const char *title;

          if((title = json_object_get_string(json_object_object_get(tmp, "title"))) != NULL)
          {
            json_object *tmp2 = json_object_object_get(tmp, "source");
            const char *source;

            if(tmp2 != NULL && (source = json_object_get_string(json_object_object_get(tmp2, "domain"))))
            {
              irc_reply_who(cq[x].irc, cq[x].reply, "[%s] %s", source, title);
              y++;
              continue;
            }
          }
        }
        break;
      }
    }
  }

  if(!y)
  {
    irc_reply_who(cq[x].irc, cq[x].reply, "Query returned no data :~(?");
    logger(C_CPANIC, CRIT, "cpanic_curl_callback", "API error (parsing json)");
  }

  if(cpanic_json != NULL)
    json_object_put(cpanic_json);
}

int
cpanic_news(const char *sym)
{
  char url[1000];

  if(sym != NULL)
    snprintf(url, 999, "%s?auth_token=%s&public=true&kind=news&currencies=%s", CPANIC_API, global.conf.cptoken, sym);
  else
    snprintf(url, 999, "%s?auth_token=%s&public=true&kind=news&filter=rising", CPANIC_API, global.conf.cptoken);

  return((curl_request(CRT_GET, 0, url, NULL, NULL, cpanic_curl_callback, NULL, NULL) == -1) ? FAIL : SUCCESS);
}

void
cpanic_init(void)
{
  logger(C_ANY, INFO, "cpanic_init", "cryptopanic.com api initialized");
}
