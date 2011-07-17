/* 
 * sock.h -- sock.c header file
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

#ifndef _SOCK_H_
#define _SOCK_H_

#include <netinet/in.h>

#ifdef TLS
#include <gnutls/gnutls.h>
struct bufsock_tls {
  union {
    gnutls_anon_client_credentials_t anoncred;
    gnutls_certificate_credentials_t xcred;
  };
  gnutls_session_t session;
};
#else
struct bufsock_tls {
};
#endif
#include "buffer.h"

typedef enum {
   BS_ERR = -1,
   BS_DISCON = 0,
   BS_DNS = 1,
   BS_CONN = 2,
   BS_READY = 3
} bufsockstate_t;

typedef struct bufsock {
   int s;
   bufsockstate_t state;
   buffer_t sendbuf, recvbuf;
   char *host;
   int port;
   void (*cb_err)(void*);
   void (*cb_data)(void*);
   void (*cb_ready)(void*);
   void *ptr;
   struct bufsock_tls tls;
} bufsock_t;

int bufsock_error(bufsock_t *bs);
int bufsock_ready(bufsock_t *bs);
void bufsock_discon(bufsock_t *bs);
int bufsock_tcp_conn(bufsock_t *bs, struct in_addr *addr);
#ifdef IPV6
int bufsock_tcp6_conn(bufsock_t *bs, struct in6_addr *addr);
#endif
int bufsock_tcp_connect(bufsock_t *bs, const char *host, int port);
void bufsock_free(bufsock_t *bs);
bufsock_t *bufsock_new(int sbufsiz, int rbufsiz,
      void (*cb_err)(void*),
      void (*cb_data)(void*),
      void (*cb_ready)(void*),
      void *ptr);

int bufsock_full(bufsock_t *bs);
int bufsock_length(bufsock_t *bs);
void bufsock_delete(bufsock_t *bs, int len);
int bufsock_get(bufsock_t *bs, char *ptr, int len);
int bufsock_put(bufsock_t *bs, const char *ptr, int len);
int bufsock_printf(bufsock_t *bs, const char *fmt, ...);
int bufsock_findchr(bufsock_t *bs, char c);

#endif
