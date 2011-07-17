/* 
 * http.h -- http.c header
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

#ifndef HTTP_H_
#define HTTP_H_

#include <netinet/in.h>
#include "list.h"
#include "sock.h"

typedef enum {
   HTTPM_GET = 0,
   HTTPM_POST,
   HTTPM_HEAD
} http_method_t;

typedef enum {
   HTTPST_ERR = -1,
   HTTPST_NONE = 0,
   HTTPST_DNS,
   HTTPST_WRITE,
   HTTPST_READ,
   HTTPST_DONE,
} http_state_t;

typedef struct {
   char *name, *value;
   struct list_head elm;
} http_param_t;

typedef struct http_req {
   void (*cb)(struct http_req *);
   http_method_t method;
   char *uri;
   char *host;
   int port;
   int status;
   http_state_t state;
   struct list_head sheader, rheader;
   int header_done;
   char *scontent, *rcontent;
   int trans_enc;
   unsigned int scontentlen, rcontentlen, chunklen;
   bufsock_t *bs;
   void *ptr;
} http_req_t;

int http_escape_str(const char *str, char *buf, int buflen);
http_param_t *http_param_new(const char *name, const char *value);
void http_param_free(http_param_t *p);
http_param_t *http_param_find (struct list_head *head, char *name);
char *http_method2str(http_method_t m);
void http_mkrequest(http_req_t *req);
int http_conn(http_req_t *req, struct in_addr *addr);
void http_parse(http_req_t *req, int nomoredata);
void http_dns(struct in_addr *addr, int num, void *ptr);
int http_canread(void *ptr);
int http_canwrite(void *ptr);
http_req_t *http_new(void (*cb)(http_req_t *));
void http_free(http_req_t *req);
int http_do(http_req_t *req, const char *uri, http_method_t method);
int http_addheader(http_req_t *req, const char *name, const char *value);
int http_get(http_req_t *req, const char *uri);
int http_post(http_req_t *req, const char *uri, char *content, int contentlen);
int http_head(http_req_t *req, const char *uri);
void http_setptr(http_req_t *req, void *ptr);
void *http_getptr(http_req_t *req);

#endif

