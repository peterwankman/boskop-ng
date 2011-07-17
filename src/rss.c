/* 
 * rss.c -- Fetch rss feeds
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

#define RSS_MAX   128

typedef struct {
   http_req_t *req;
   char *link, *title, *src;
} rss_rec;

int rss_cnt;
unsigned long lastup;
rss_rec  rss_r[RSS_MAX];

void rss_free_r(rss_rec *r)
{
   if (r->link)
      free(r->link);
   r->link = NULL;
   
   if (r->title)
      free(r->title);
   r->title = NULL;

   if (r->src)
      free(r->src);
   r->src = NULL;
}

int rss_up_r(rss_rec *r, const char *l, const char *t, const char *s)
{
   rss_free_r(r);
   
   if (!l || !t || !s)
      return -1;

   r->link = strdup(l);
   r->title = strdup(t);
   r->src = strdup(s);

   if (!r->link || !r->title || !r->src) {
      rss_free_r(r);
      return -1;
   }
   return 0;
}

void http_cb(http_req_t *req) {
   int rn;

   for (rn = 0; rn < rss_cnt; ++rn)
      if (rss_r[rn].req == req)
         break;

   if (rn == rss_cnt) {
      http_free(req);
      return;
   }

   if (req->state == HTTPST_DONE) {
      xmlDoc *doc;
      xmlNode *n, *tn, *ln , *rt, *cn, *sn;
      char *newlink, *newtitle, *newsrc;
      int i;

      doc = xmlReadMemory(req->rcontent, req->rcontentlen, req->uri, NULL,
               XML_PARSE_RECOVER|XML_PARSE_NOERROR|XML_PARSE_NOWARNING
               |XML_PARSE_NONET); 
      if (!doc)
         goto out;
      rt = xmlDocGetRootElement(doc);
      cn = find_node(rt, "channel", XML_ELEMENT_NODE);

      if (!(n = find_node(cn, "item", XML_ELEMENT_NODE)))
         n = find_node(rt, "item", XML_ELEMENT_NODE);
      
      if (cn && n && (sn = find_node(cn, "title", XML_ELEMENT_NODE))
               && (tn = find_node(n, "title", XML_ELEMENT_NODE))
               && (ln = find_node(n, "link", XML_ELEMENT_NODE))) {
         newsrc = text_node(sn);
         newtitle = text_node(tn);
         newlink = text_node(ln);      
         if (newsrc && newtitle && newlink) {
            if (rss_r[rn].link) {
               if (strcmp(newlink, rss_r[rn].link)) {
                  if (rss_up_r(&rss_r[rn], newlink, newtitle, newsrc) != -1)
                     for (i = 0; i < config_getcnt("rss.so", "target"); ++i)
                        irc_privmsg(config_getn("rss.so", "target", i),
                           "%s: %s %s", rss_r[rn].src, rss_r[rn].title,
                           rss_r[rn].link); 
               }
            } else {
               rss_up_r(&rss_r[rn], newlink, newtitle, newsrc);
            }
         } 
      }
      xmlFreeDoc(doc);
   }
   xmlCleanupParser();
out:
   http_free(req);
   rss_r[rn].req = NULL;
}

void timer(unsigned long ts) {
   int i;

   if (lastup < ts - 60) {
      lastup = ts;
      for (i = 0; i < rss_cnt; ++i) {
         if (rss_r[i].req)
            http_free(rss_r[i].req);
         rss_r[i].req = http_new(http_cb);
         if (http_get(rss_r[i].req, config_getn("rss.so", "source", i)) == -1){
            http_free(rss_r[i].req);
            rss_r[i].req = NULL;
         }
      }
   }
}

int reply(info_t * in) {
   int i;

   if (in->cmd == cmd_privmsg && CHECKAUTH(in->sender, UL_OP)) {
      in->tail = skip_nick(in->tail, in->me);
      if (!tail_cmd(&in->tail, "rss")) {
         for (i = 0; i < rss_cnt; ++i) {
         if (rss_r[i].link)
               irc_privmsg(to_sender(in), "%s: %s %s", rss_r[i].src,
                     rss_r[i].title, rss_r[i].link);
         }
      }
   }
   return 0;
}

int init(void) {
   lastup = 0;
   rss_cnt = MIN(config_getcnt("rss.so", "source"), RSS_MAX);
   memset(rss_r, 0, sizeof(rss_r));

   return 0;
}

void destroy(void) {
   int i;

   for (i = 0; i < rss_cnt; ++i) {
      if (rss_r[i].req)
         http_free(rss_r[i].req);
      rss_free_r(&rss_r[i]);
   }
}

PLUGIN_DEF(init, destroy, reply, timer, NULL, PLUGIN_NODEP, PLUGIN_NOHELP);

