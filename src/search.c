/* 
 * search.c -- Search the web
 * 
 * Copyright (C) 2009-2010  Martin Wolters et al.
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
#include "rrand.h"

unsigned long lastcall = 0;

void http_cb(http_req_t *req) {
   char *target, *txt = NULL, *url = NULL;
   xmlDoc *doc;
   xmlNode *rt, *se, *it;
#ifdef SEARCH_RANDOM
   int cnt = 0;
#endif
  
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

   if (!req->rcontent || !req->rcontentlen || !req->uri) {
	irc_privmsg(target, "HTTP No data");
	goto free_out;
   }

   doc = xmlReadMemory(req->rcontent, req->rcontentlen, req->uri, NULL,
      XML_PARSE_RECOVER|XML_PARSE_NOERROR|XML_PARSE_NOWARNING|XML_PARSE_NONET);   
   if (!doc)
      goto free_out;
   rt = xmlDocGetRootElement(doc);
   se = find_node(rt, "Section", XML_ELEMENT_NODE); 
   it = find_node(se, "Item", XML_ELEMENT_NODE);
#ifdef SEARCH_RANDOM
   cnt = count_nodes(it);
   if (cnt) {
      it = nth_node(it, rrand(cnt));
      txt = text_node(find_node(it, "Text", XML_ELEMENT_NODE));
      url = text_node(find_node(it, "Url", XML_ELEMENT_NODE));
   }
   if (cnt && txt && url) 
#else
   txt = text_node(find_node(it, "Text", XML_ELEMENT_NODE));
   url = text_node(find_node(it, "Url", XML_ELEMENT_NODE));
   if (txt && url)
#endif
      irc_privmsg(target, "%s: %s", txt, url);
   else
      irc_privmsg(target, "No search results");
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
      if (!tail_cmd(&in->tail, "search")) {
         now = time(NULL);
         if (!CHECKAUTH(in->sender, UL_OP) && lastcall > now - 10) {
            irc_privmsg(to_sender(in), "Please wait a moment");
            return 0;
         }
         if (!in->tail || !in->tail[0])
            return 0;
         p = config_get("search.so", "url");
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
PLUGIN_DEF(NULL, NULL, reply, NULL, NULL, PLUGIN_NODEP, PLUGIN_HELP("!search - Search the web"));

