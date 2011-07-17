/* 
 * weather.c -- Show the weater
 * 
 * Copyright (C) 2010  Martin Wolters et al.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to 
 * the Free Software Foundation, Inc.
 * 51 Franklin Street, Fifth Floor
 * Boston, MA  02110-1301, USA
 * 
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "irc.h"
#include "util.h"
#include "conf.h"
#include "http.h"
#include "auth.h"
#include "xml.h"

unsigned long lastcall = 0;

typedef struct {
   char *name;
   char *pat;
} pentry_t;

pentry_t pentries[] = {
   { "condition", "%s" },
   { "temp_c", "%s °C" },
   { "humidity", "%s" },
   { "wind_condition", "%s" },
   { NULL, NULL }
};

void http_cb(http_req_t *req) {
   char *target, *p;
   xmlDoc *doc;
   xmlNode *rt, *wt, *cc;
   char buf[BUFSIZ];
   int i, j;

   if (req->state != HTTPST_DONE && req->state != HTTPST_ERR)
      return;

   target = (char*)http_getptr(req);
   if (!target)
      goto out;

   if (req->state == HTTPST_ERR) {
      if (!req->status)
         irc_privmsg(target, "HTTP Connection error");
      else
         irc_privmsg(target, "HTTP Error: %i",req->status);
      goto free_out;
   }

   doc = xmlReadMemory(req->rcontent, req->rcontentlen, req->uri, NULL, 
      XML_PARSE_RECOVER|XML_PARSE_NOERROR|XML_PARSE_NOWARNING|XML_PARSE_NONET);   
   if (!doc)
      goto free_out;
   rt = xmlDocGetRootElement(doc);
   wt = find_node(rt, "weather", XML_ELEMENT_NODE);

   buf[0] = '\0';
   cc = find_node(wt, "current_conditions", XML_ELEMENT_NODE);
   if (cc) {
      for (i = 0; pentries[i].name; ++i) {
         p = node_prop(find_node(cc, pentries[i].name, XML_ELEMENT_NODE),
               "data");
         if (p) {
            j = strlen(buf);
            if (j) {
               buf[j++] = ',';
               buf[j++] = ' ';
            }
            snprintf(buf+j, BUFSIZ-j-2, pentries[i].pat, p);
            xmlFree(p);
         }
      }
   }
   if (*buf) {
      p = node_prop(find_node(find_node(wt, "forecast_information",
         XML_ELEMENT_NODE), "city", XML_ELEMENT_NODE), "data");
      if (p) {
         irc_privmsg(target, "%s: %s", p, buf); 
         xmlFree(p);
      } else {
         irc_privmsg(target, "%s", buf);
      }
   } else {
      irc_privmsg(target, "No results");
   }
   xmlFreeDoc(doc);
free_out:
   xmlCleanupParser();
   free(target);
out:
   http_free(req);
}

int reply(info_t * in) {
   int i;
   http_req_t *req;
   char buf[4096], *p;
   unsigned long now;

   if (in->cmd == cmd_privmsg) {
      in->tail = skip_nick(in->tail, in->me);
      if (!tail_cmd(&in->tail, "weather")) {
         now = time(NULL);
         if (!CHECKAUTH(in->sender, UL_OP) && lastcall > now - 10) {
            irc_privmsg(to_sender(in), "Please wait a moment");
            return 0;
         }
         
         if (!in->tail || !in->tail[0])
            return 0;

         p = config_get("weather.so", "url");
         if (!p || (i = strlen(p)) >= sizeof(buf))
            return 0;
         strcpy(buf, p);
         if (http_escape_str(in->tail, buf + i, sizeof(buf) -i))
            return 0;
         req = http_new(http_cb);
         if (!req)
            return 0;
         http_setptr(req, strdup(to_sender(in)));
         http_get(req, buf);
         lastcall = now;
      }
   }
   return 0;
}

/* TODO: Free all pending requests when unloading module */
PLUGIN_DEF(NULL, NULL, reply, NULL, NULL, PLUGIN_NODEP, PLUGIN_HELP("!weather <region> - Weather forecast"))


