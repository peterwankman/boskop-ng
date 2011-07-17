/* 
 * sock.c -- Buffered socket
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

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "defs.h"
#include "sock.h"
#include "rrand.h"
#include "io.h"
#include "dns.h"

void bufsock_dns(struct in_addr *addr, int num, void *ptr)
{
   bufsock_t *bs = (bufsock_t*)ptr;
   
   if (num > 0) {
      if (bs->state == BS_DNS) {
         if (bufsock_tcp_conn(bs, &addr[rrand(num)]) == -1 && bs->cb_err)
            bs->cb_err(bs->ptr);
      }
   } else {
      bs->state = BS_ERR;
      if (bs->cb_err)
         bs->cb_err(bs->ptr);
   } 
}

#ifdef IPV6
void bufsock_dns6(struct in6_addr *addr, int num, void *ptr)
{
   bufsock_t *bs = (bufsock_t*)ptr;

   if (num > 0) {
      if (bs->state == BS_DNS) {
         if (bufsock_tcp6_conn(bs, &addr[rrand(num)]) == -1) {
            if (!bs->host || dns_host2ip(bs->host, bufsock_dns, bs) == -1) {
               bs->state = BS_ERR;
               if (bs->cb_err)
                  bs->cb_err(bs->ptr);
            }
         }
      }
   } else {
      if (!bs->host || dns_host2ip(bs->host, bufsock_dns, bs) == -1) {
         bs->state = BS_ERR;
         if (bs->cb_err)
            bs->cb_err(bs->ptr);
      }
   }
}
#endif

int bufsock_canwrite(void *ptr)
{
   char *p;
   int len, ret;
   bufsock_t *bs = (bufsock_t*)ptr;

   if (bs->state < BS_READY) {
      bs->state = BS_READY;
      if (bs->cb_ready)
         bs->cb_ready(bs->ptr);
   }

   while((len = buffer_map(&bs->sendbuf, &p)) > 0) {
      ret = send(bs->s, p, len, 0);
      if (ret <= 0) {
         if (ret == 0 || errno != EAGAIN) {
            io_close(&bs->s);
            bs->state = BS_ERR;
            if (bs->cb_err)
               bs->cb_err(bs->ptr);
         }
         return 0;
      } else {
         buffer_delete(&bs->sendbuf, ret);
      }
   }
   io_wantwrite(bs->s, NULL);
   return 0;
}

int bufsock_canread(void *ptr)
{
   char buf[BUFSIZE];
   int ret;
   bufsock_t *bs = (bufsock_t*)ptr;
  
   while ((ret = recv(bs->s, buf, sizeof(buf), 0)) > 0) {
      if (buffer_put(&bs->recvbuf, buf, ret) == -1)
         break;
   }
   
   if (ret == 0) {
      io_close(&bs->s);
      bs->state = BS_DISCON;
      if (bs->cb_err)
         bs->cb_err(bs->ptr);
      return 0;
   } else if (ret == -1 && errno != EAGAIN) {
      io_close(&bs->s);
      bs->state = BS_ERR;
      if (bs->cb_err)
         bs->cb_err(bs->ptr);
      return 0;
   }
   
   if (buffer_length(&bs->recvbuf) > 0 && bs->cb_data)
      bs->cb_data(bs->ptr);
   return 0;
}

int bufsock_error(bufsock_t *bs)
{
   return bs->state == BS_ERR;
}

int bufsock_ready(bufsock_t *bs)
{
   return bs->state == BS_READY;
}

void bufsock_discon(bufsock_t *bs)
{
   int len, ret;
   char *p;
   
   bs->state = BS_DISCON;

   if (bs->s < 0)
      return;
   
   while((len = buffer_map(&bs->sendbuf, &p)) > 0) {
      ret = send(bs->s, p, len, 0);
      if (ret <= 0)
         break;
      buffer_delete(&bs->sendbuf, ret);
   }

   io_close(&bs->s);
   buffer_clear(&bs->sendbuf);
   buffer_clear(&bs->recvbuf);
}

int bufsock_tcp_conn(bufsock_t *bs, struct in_addr *addr)
{
   struct sockaddr_in sin;
   
   memcpy(&sin.sin_addr, addr, sizeof(struct in_addr));
   sin.sin_family = AF_INET;
   sin.sin_port = htons(bs->port);

   if (bs->s != -1)
	io_close(&bs->s);
   
   bs->state = BS_ERR;
   bs->s = socket (PF_INET, SOCK_STREAM, IPPROTO_TCP);
   if (bs->s == -1)
      return -1;

   if (fcntl(bs->s, F_SETFL, O_NONBLOCK) == -1) {
      close(bs->s);
      bs->s = -1;
      return -1;
   }
   
   if (connect(bs->s, (struct sockaddr *)&sin, sizeof(sin)) == -1
         && errno != EINPROGRESS) {
      close(bs->s);
      bs->s = -1;
      return -1;
   }

   if (io_register(bs->s) == -1) {
      close(bs->s);
      bs->s = -1;
      return -1;
   }

   io_setptr(bs->s, bs);
   io_wantwrite(bs->s, bufsock_canwrite);   
   io_wantread(bs->s, bufsock_canread);
   bs->state = BS_CONN;
   return 0;
}

#ifdef IPV6
int bufsock_tcp6_conn(bufsock_t *bs, struct in6_addr *addr)
{
   struct sockaddr_in6 sin;

   memset(&sin, 0, sizeof(sin));
   memcpy(&sin.sin6_addr, addr, sizeof(struct in6_addr));
   sin.sin6_family = AF_INET6;
   sin.sin6_port = htons(bs->port);

   if (bs->s != -1)
	   io_close(&bs->s);
   
   bs->state = BS_ERR;
   bs->s = socket (PF_INET6, SOCK_STREAM, IPPROTO_TCP);
   if (bs->s == -1)
      return -1;

   if (fcntl(bs->s, F_SETFL, O_NONBLOCK) == -1) {
      close(bs->s);
      bs->s = -1;
      return -1;
   }

   if (connect(bs->s, (struct sockaddr *)&sin, sizeof(sin)) == -1
         && errno != EINPROGRESS) {
      close(bs->s);
      bs->s = -1;
      return -1;
   }
   if (io_register(bs->s) == -1) {
      close(bs->s);
      bs->s = -1;
      return -1;
   }

   io_setptr(bs->s, bs);
   io_wantwrite(bs->s, bufsock_canwrite);
   io_wantread(bs->s, bufsock_canread);
   bs->state = BS_CONN;
   return 0;
}
#endif

int bufsock_tcp_connect(bufsock_t *bs, const char *host, int port)
{
   struct in_addr addr;
#ifdef IPV6   
   struct in6_addr addr6;
#endif

   buffer_clear(&bs->sendbuf);
   buffer_clear(&bs->recvbuf);
 
   bs->port = port;
   if (bs->host)
      free(bs->host);
   bs->host = strdup(host);
#ifdef IPV6
   if (inet_pton(AF_INET6, host, &addr6)) {
      return bufsock_tcp6_conn(bs, &addr6);
   } else
#endif
   if (inet_aton(host, &addr)) {
      return bufsock_tcp_conn(bs, &addr);
   } else {
      if (
#ifdef IPV6
            dns_host2ip6(host, bufsock_dns6, bs) == -1
#else
            dns_host2ip(host, bufsock_dns, bs) == -1
#endif
) { 
         bs->state = BS_ERR;
         return -1;
      }
      bs->state = BS_DNS;
      return 0;
   }
}

void bufsock_free(bufsock_t *bs)
{
   dns_cancel((void*)bs);
   bufsock_discon(bs);
   buffer_free(&bs->sendbuf);
   buffer_free(&bs->recvbuf);
   
   if (bs->host)
      free(bs->host);
   free(bs);
}

bufsock_t *bufsock_new(int sbufsiz, int rbufsiz,
      void (*cb_err)(void*),
      void (*cb_data)(void*),
      void (*cb_ready)(void*),
      void *ptr)
{
   bufsock_t *bs;

   bs = calloc(1, sizeof(bufsock_t));
   if (!bs)
      return NULL;

   if (buffer_init(&bs->sendbuf, sbufsiz) == -1) {
      free(bs);
      return NULL;
   }
   if (buffer_init(&bs->recvbuf, rbufsiz) == -1) {
      buffer_free(&bs->sendbuf);  
      free(bs);
      return NULL;
   }
   bs->s = -1;
   bs->cb_err = cb_err;
   bs->cb_data = cb_data;
   bs->cb_ready = cb_ready;
   bs->ptr = ptr;
   return bs;
}

int bufsock_full(bufsock_t *bs)
{
   return buffer_length(&bs->recvbuf) == buffer_size(&bs->recvbuf);
}

int bufsock_length(bufsock_t *bs)
{
   return buffer_length(&bs->recvbuf); 
}

void bufsock_delete(bufsock_t *bs, int len)
{
   buffer_delete(&bs->recvbuf, len);
}

int bufsock_get(bufsock_t *bs, char *ptr, int len)
{
   return buffer_get(&bs->recvbuf, ptr, len);
}

int bufsock_put(bufsock_t *bs, const char *ptr, int len)
{
   int ret;

   ret =  buffer_put(&bs->sendbuf, ptr, len);
   if (ret != -1)
      io_wantwrite(bs->s, bufsock_canwrite);
   return ret;
}

int bufsock_printf(bufsock_t *bs, const char *fmt, ...)
{
   int ret;
   va_list ap;

   va_start(ap, fmt);
   ret = buffer_vprintf(&bs->sendbuf, fmt, ap);
   va_end(ap);
   if (ret != -1)
      io_wantwrite(bs->s, bufsock_canwrite);
   return ret;
}

int bufsock_findchr(bufsock_t *bs, char c)
{
   return buffer_findchr(&bs->recvbuf, c);
}

