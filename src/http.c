/* 
 * http.c -- web support
 * 
 * Copyright (C) 2009  Martin Wolters et al.
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

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>

#include "defs.h"
#include "http.h"
#include "dns.h"
#include "io.h"
#include "rrand.h"

static char __int2hex[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
                            'A', 'B', 'C', 'D', 'E', 'F' };

int http_escape_str(const char *str, char *buf, int buflen)
{
   char c, *end = buf + buflen;
   
   while ((c = *(str++))) {
      if (isalnum(c)) {
         if (buf >= end)
            return -1;
         *(buf++) = c;
      } else {
         if (buf+2 >= end)
            return -1;
         *(buf++) = '%';
         *(buf++) = __int2hex[((unsigned char)c >> 4) & 0xf];
         *(buf++) = __int2hex[ (unsigned char)c & 0xf ];
      }
   }
   if (buf >= end)
      return -1;
   *(buf++) = '\0';
   return 0;
}

http_param_t *http_param_new(const char *name, const char *value)
{
   http_param_t *p;
   
   p = malloc(sizeof(http_param_t));
   if (!p)
      return NULL;

   p->name = strdup(name);
   if (!p->name) {
      free(p);
      return NULL;
   }

   p->value = strdup(value);
   if (!p->value) {
      free(p->name);
      free(p);
      return NULL;
   }
   return p;
}

void http_param_free(http_param_t *p)
{
   if (!p)
      return;
   free(p->value);
   free(p->name);
   free(p);
}

http_param_t *http_param_find (struct list_head *head, char *name)
{
   http_param_t *p;

   list_for_each_entry(p, head, elm) {
      if (!strcasecmp(name, p->name))
         return p;
   }
   return NULL;
}

char *http_method2str(http_method_t m)
{
   switch(m) {
      case HTTPM_POST:
         return "POST";
      case HTTPM_HEAD:
         return "HEAD";
      case HTTPM_GET:
      default:
         return "GET";
   }
}

void http_cb_ready(void *ptr)
{
   http_req_t *req = (http_req_t *) ptr;
   http_param_t *p;

   bufsock_printf(req->bs, "%s %s HTTP/1.1\r\n",
         http_method2str(req->method), req->uri);
   bufsock_printf(req->bs, "Host: %s\r\n", req->host);
   list_for_each_entry(p, &req->sheader, elm)
      bufsock_printf(req->bs, "%s: %s\r\n", p->name, p->value);
   if (req->scontent && req->scontentlen)
      bufsock_printf(req->bs, "Content-Length: %u\r\n",
         req->scontentlen);
   bufsock_printf(req->bs, "Connection: close\r\n");
   bufsock_printf(req->bs, "\r\n");
   if (req->scontent && req->scontentlen)
      bufsock_put(req->bs, req->scontent, req->scontentlen);
}

void http_parse(http_req_t *req, int nomoredata)
{
   char buf[BUFSIZE], *p;
   int pos, len;
   http_param_t *pr;

   if (req->state == HTTPST_DONE)
      return;

   if (req->status <= 0) {
      pos = bufsock_findchr(req->bs, '\n');
      if (pos == -1)
         goto out;
      len = bufsock_get(req->bs, buf, MIN(pos, sizeof(buf)));
      bufsock_delete(req->bs, pos - len);
      buf[len-1] = '\0';
      if (len > 1 && buf[len-2] == '\r')
         buf[len-2] = '\0';
      p = strchr(buf, ' ');
      if (p) {
         req->status = atoi(p);
      } else {
         req->status = 999;
      }
   }
   while (!req->header_done) {
      pos = bufsock_findchr(req->bs, '\n');
      if (pos == -1)
         goto out;
      len = bufsock_get(req->bs, buf, MIN(pos, sizeof(buf)));
      bufsock_delete(req->bs, pos - len);
      buf[len-1] = '\0';
      if (len > 1 && buf[len-2] == '\r')
         buf[len-2] = '\0';
      if (!strlen(buf)) {
         req->header_done = 1;
         pr = http_param_find(&req->rheader, "transfer-encoding");
         if (pr && !strcasecmp("chunked", pr->value)) {
            req->trans_enc = 1;
         } else {
            pr = http_param_find(&req->rheader, "content-length");
            if (pr) {
               req->rcontentlen = atoi(pr->value);
            }
         }
         continue;
      }
      p = strchr(buf, ':');
      if (p && *(p+1) == ' ') {
         *p = '\0';
         pr = http_param_new(buf, p+2);
         if (pr) {
            list_add(&pr->elm, &req->rheader);
         } else {
            req->state = HTTPST_ERR;
            return;
         }
      }
   }
   if (req->trans_enc == 1) {
      for(;;) {
         if (!req->chunklen) {
            char buf[10];
            int i;
            i = bufsock_findchr(req->bs, '\n');
            if (i == -1)
              break; 
            if (i > sizeof(buf) || i < 2) {
               req->state = HTTPST_ERR;
               break;
            }
            if (i == -1) {
               if (bufsock_length(req->bs) >= sizeof(buf)) {
                  req->state = HTTPST_ERR;
               }
               break;
            }

            bufsock_get(req->bs, buf, i);
            buf[i-1] = '\0';
            if (sscanf(buf, "%x", &req->chunklen) != 1) {
               req->state = HTTPST_ERR;
               break;
            }
            if (!req->chunklen) {
               req->state = HTTPST_DONE;
               break;
            }
         }

         if (bufsock_length(req->bs) < req->chunklen+2)
            break;
      
         if (req->rcontent)
            req->rcontent = realloc(req->rcontent,
                  req->rcontentlen+req->chunklen);
         else
            req->rcontent = malloc(req->rcontentlen+req->chunklen);

         if (!req->rcontent) {
            req->state = HTTPST_ERR;
            req->rcontentlen = 0;
            break;
         } else {
            bufsock_get(req->bs, &req->rcontent[req->rcontentlen],
                  req->chunklen);
            bufsock_delete(req->bs, 2); /* Just drop CRLF */
            req->rcontentlen += req->chunklen;
         }
         req->chunklen = 0;
      }
   } else {
      if (req->rcontentlen &&
            bufsock_length(req->bs) >= req->rcontentlen) {
         req->rcontent = malloc(req->rcontentlen);
         if (!req->rcontent) {
            req->state = HTTPST_ERR;
            req->rcontentlen = 0;
         } else {
            bufsock_get(req->bs, req->rcontent, req->rcontentlen);
            req->state = HTTPST_DONE;
         }
      }
   }

out:
   if (nomoredata) {
      if (req->trans_enc == 0 && !req->rcontentlen) {
        req->rcontentlen = bufsock_length(req->bs);
	if (!req->rcontentlen) {
		req->state = HTTPST_ERR;
	} else {
        	req->rcontent = malloc(req->rcontentlen);
         	if (!req->rcontent) {
            		req->state = HTTPST_ERR;
            		req->rcontentlen = 0;
         	} else {
            		bufsock_get(req->bs, req->rcontent, req->rcontentlen);
            		req->state = HTTPST_DONE;
         	}
	}
      } else {
         if (req->state != HTTPST_DONE) {
            req->state = HTTPST_ERR;
            req->rcontentlen = 0;
         }
      }
      if (req->state == HTTPST_ERR && req->status == 200)
         req->status = 0;
   }
}

void http_cb_err(void *ptr)
{
   http_req_t *req = (http_req_t*)ptr;

   bufsock_discon(req->bs);
   http_parse(req, 1);
   if(req->state != HTTPST_DONE)
      req->state = HTTPST_ERR;
   if (req->cb)
      req->cb(req);
}

void http_cb_data(void *ptr)
{
   http_req_t *req = (http_req_t*)ptr;

   http_parse(req, 0);
   if (req->state != HTTPST_ERR && req->state != HTTPST_DONE &&
            !bufsock_full(req->bs))
      return;
   bufsock_discon(req->bs);
   http_parse(req, 1);
   if(req->state != HTTPST_DONE)
      req->state = HTTPST_ERR;
   if (req->cb)
      req->cb(req);
}

http_req_t *http_new(void (*cb)(http_req_t *))
{
   http_req_t *req;

   req = calloc(1,sizeof(http_req_t));
   if (!req)
      return NULL;
   
   req->bs = bufsock_new(10* BUFSIZE, 20* BUFSIZE, http_cb_err, http_cb_data,
                  http_cb_ready, req);
   if (!req->bs) {
      free(req);
      return NULL;
   }

   INIT_LIST_HEAD(&req->sheader);
   INIT_LIST_HEAD(&req->rheader);

   req->cb = cb;
   return req;
}

void http_free(http_req_t *req)
{
   http_param_t *p;
   struct list_head *head;

   if(!req)
      return;
   
   if(req->uri)
      free(req->uri);
   
   if (req->host)
      free(req->host);

   if (req->scontent)
      free(req->scontent);

   if (req->rcontent)
      free(req->rcontent);

   while (!list_empty(&req->sheader)) {
      head = &req->sheader;
      p = list_entry(head->next, http_param_t, elm);
      list_del(&p->elm);
      http_param_free(p);
   }

   while (!list_empty(&req->rheader)) {
      head = &req->rheader;
      p = list_entry(head->next, http_param_t, elm);
      list_del(&p->elm);
      http_param_free(p);
   }
  
   bufsock_free(req->bs); 
   free(req);
}

int http_do(http_req_t *req, const char *uri, http_method_t method)
{
   char *p, *pp;

   if (!req || !uri)
      return -1;

   if (req->state != HTTPST_NONE)
      return -1;
   if (strncasecmp(uri, "http://", 7))
      return -1;
   uri += 7;
   if (!(p = strchr(uri, '/')))
      return -1;
   req->uri = strdup(p);
   if (!req->uri)
      return -1;
   pp = strchr(uri, ':');
   if (pp) {
      req->port = atoi(pp+1);
   } else {
      req->port = 80;
      pp = p;
   }
   if (pp - uri == 0) {
      free(req->uri);
      req->uri = NULL;
      return -1;
   }
   req->host = malloc(pp - uri + 1);
   if (!req->host) {
      free(req->uri);
      req->uri = NULL;
      return -1;
   }
   memcpy(req->host, uri, pp - uri);
   req->host[pp - uri] = '\0';
   return bufsock_tcp_connect(req->bs, req->host, req->port);
}

int http_addheader(http_req_t *req, const char *name, const char *value)
{
   http_param_t *p;
   if (!req)
      return -1;
   p = http_param_new(name, value);
   if (!p)
      return -1;
   list_add(&p->elm, &req->sheader);
   return 0;
}

int http_get(http_req_t *req, const char *uri)
{
   return http_do(req, uri, HTTPM_GET);
}

int http_post(http_req_t *req, const char *uri, char *content, int contentlen)
{
   if (!req)
      return -1;

   if (contentlen > 0 && content) {
      if(req->scontent)
         free(req->scontent);
      req->scontent = malloc(contentlen);
      if (!req->scontent) {
         req->scontentlen = 0;
         return -1;
      }
      memcpy(req->scontent, content, contentlen);
      req->scontentlen = contentlen;
   }
   return http_do(req, uri, HTTPM_POST);
}

int http_head(http_req_t *req, const char *uri)
{
   return http_do(req, uri, HTTPM_HEAD);
}

void http_setptr(http_req_t *req, void *ptr)
{
   if (req)
      req->ptr = ptr;
}

void *http_getptr(http_req_t *req)
{
   return (req)?(req->ptr):(NULL);
}

