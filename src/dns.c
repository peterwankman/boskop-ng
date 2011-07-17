/* 
 * dns.c -- DNS Resolver
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "dns.h"
#include "io.h"

/*
 * TODO:
 * - Some RCODE aren't fatal, resend query
 * - Caching
 * - Recursiv requests
 * - IPv6 nameservers
 */

dns_setup_t dns_setup;
int dns_initalized = 0;

LIST_HEAD(dns_queries);

char *dns_parse_query(char *buf, char *ptr, char *end, char *host, int hlen,
   dns_query_t** qry) {

   if (!buf || !ptr || !end || !host || !hlen)
      return NULL;

   if (end - ptr < 2)
      return NULL;
   ptr = dns_expand(ptr, buf, end, host, host+hlen);
   if (!ptr)
      return NULL;
   if (end - ptr < sizeof(dns_query_t))
      return NULL;
   *qry = (dns_query_t*)ptr;
   ptr += sizeof(dns_query_t);
   return ptr;
}

char *dns_parse_answer(char *buf, char *ptr, char *end, char *host, int hlen,
   dns_record_t **rec) {

   if (!buf || !ptr || !end || !host || !hlen)
      return NULL;

   if (end - ptr < 2)
      return NULL;
   ptr = dns_expand(ptr, buf, end, host, host+hlen);
   if (!ptr)
      return NULL;
   if (end - ptr < sizeof(dns_record_t))
      return NULL;
   *rec = (dns_record_t*)ptr;
   ptr += sizeof(dns_record_t);
   return ptr;
}

int dns_canread(void *ptr)
{
   dns_req_t *req;
   dns_header_t *hptr;
   dns_record_t *rptr;
   dns_query_t *qptr;
   struct sockaddr_in sin;
   struct in_addr addr[10];
#ifdef IPV6
   struct in6_addr addr6[10];
#endif
   char buf[DNSBUF], tmp[512], answ[512], *p;
   int ret, i, cnt, num;
   unsigned int len = sizeof(sin);

   ret = recvfrom(dns_setup.s, buf, sizeof(buf), 0, (struct sockaddr *)&sin,
            &len);
   if (ret < sizeof(dns_header_t)) {
      return 0;
   }
   for (i = 0; i < dns_setup.nscount; ++i)
      if (!memcmp(&dns_setup.nslist[i].sin_addr, &sin.sin_addr,
         sizeof(struct in_addr))) 
      break;
   if (i == dns_setup.nscount) {
      return 0;
   }
   
   hptr = (dns_header_t*)buf;
   p = buf + sizeof(dns_header_t);
   req = dns_findid(ntohs(hptr->id));
   if (!req)
      return 0;
   switch(req->type) {
      case DNS_T_HOST2IP:
#ifdef IPV6
      case DNS_T_HOST2IP6:
#endif
         if (ntohs(hptr->rcode)) {
            list_del(&req->elm);
#ifdef IPV6
            if (req->type == DNS_T_HOST2IP6) {
               req->cbi6(NULL, 0, req->ptr);
            } else {
#endif
               req->cbi(NULL, 0, req->ptr);
#ifdef IPV6
            }
#endif
	    break;
	}
         
         cnt = ntohs(hptr->qdcount);
         for(i = 0; i < cnt; ++i){
            if (!(p = dns_parse_query(buf, p, buf+ret, tmp, sizeof(tmp),
                  &qptr)))
               break;
         }
         cnt = ntohs(hptr->ancount);
         num = 0;
         for(i = 0; i < cnt; ++i) {
            if (!(p = dns_parse_answer(buf, p, buf+ret, tmp, sizeof(tmp),
                  &rptr)))
               break;
#ifdef IPV6
            if (req->type == DNS_T_HOST2IP6) {
               if (ntohs(rptr->class) == CLASS_IN && ntohs(rptr->type) == TYPE_AAAA) {
                  memcpy(&addr6[num], rptr->rdata,
                     MIN(sizeof(addr6[0]), ntohs(rptr->rdlength)));
                  ++num;
               }
            } else {
#endif
               if (ntohs(rptr->class) == CLASS_IN && ntohs(rptr->type) == TYPE_A) {
                  memcpy(&addr[num], rptr->rdata,
                     MIN(sizeof(addr[0]), ntohs(rptr->rdlength)));
                  ++num;
               }
#ifdef IPV6
            }
#endif
            p += ntohs(rptr->rdlength);
         }

         list_del(&req->elm);
#ifdef IPV6
         if (req->type == DNS_T_HOST2IP6) {
            req->cbi6(addr6, num, req->ptr);
         } else {
#endif
	   req->cbi(addr, num, req->ptr); 
#ifdef IPV6
         }
#endif
         break;
      case DNS_T_IP2HOST:
#ifdef IPV6
      case DNS_T_IP62HOST:
#endif
         if (ntohs(hptr->rcode))
            req->cbh(NULL, req->ptr);
         cnt = ntohs(hptr->qdcount);
         for(i = 0; i < cnt; ++i){
            if (!(p = dns_parse_query(buf, p, buf+ret, tmp, sizeof(tmp),
                  &qptr)))
               break;
         }
         cnt = ntohs(hptr->ancount);
         answ[0] = '\0';
         for(i = 0; i < cnt; ++i) {
            if (!(p = dns_parse_answer(buf, p, buf+ret, tmp, sizeof(tmp),
                  &rptr)))
               break;
            if (ntohs(rptr->class) == CLASS_IN
                  && ntohs(rptr->type) == TYPE_PTR) {
               dns_expand(p, buf, p+ntohs(rptr->rdlength), answ,
                  answ+sizeof(answ));   
               break;
            }
            p += ntohs(rptr->rdlength);
         }
	 list_del(&req->elm);
         req->cbh(answ[0]?answ:NULL, req->ptr);
         break;
      default:
	 list_del(&req->elm);
         break;
   }
   free(req);
   return 0;
}

int dns_canwrite(void *ptr)
{
   dns_req_t *req, *nreq;
   unsigned long now = time(NULL);
 
   list_for_each_entry_safe(req, nreq, &dns_queries, elm) {
      if (req->state == DNS_S_NONE || (req->state == DNS_S_SENT
            && req->lasttry < now - 3)) {
         if (req->retries > 5) {
            list_del(&req->elm);
            free(req);
            continue;
         }
         (void)sendto(dns_setup.s, req->buf, req->len, 0,
            (struct sockaddr *)&dns_setup.nslist[dns_setup.nsnext],
            sizeof(struct sockaddr_in));
         req->state = DNS_S_SENT;
         ++req->retries;
         req->lasttry = now;
         dns_setup.nsnext = (dns_setup.nsnext + 1) % dns_setup.nscount;
         return 0;
      }
   }
   (void)io_wantwrite(dns_setup.s, NULL);
   return 0;
}

void dns_duty(unsigned long now)
{
   if (dns_setup.lastduty < now - 3) {
      if (!list_empty(&dns_queries)) 
         (void)io_wantwrite(dns_setup.s, dns_canwrite);
      else
         (void)io_wantwrite(dns_setup.s, NULL);
      dns_setup.lastduty = now;
   }
}

int dns_init(void)
{
   char buf[512], *p, *n;
   FILE *f;
   int st = 0;
   
   if (dns_initalized)
      return 0;
 
   dns_setup.nscount = 0;
   dns_setup.s = -1;
   dns_setup.curid = 0;
   dns_setup.nsnext = 0;
   dns_setup.lastduty = time(NULL);
   
   f = fopen(RESOLV_CONF, "r");
   if (!f)
      return -1;
   while(dns_setup.nscount < NSMAX) {
      if (!fgets(buf, sizeof(buf), f))
         break;
      if (!strncmp(buf, "nameserver", 10) && isspace(buf[10])) {
         p = buf+11;
         while (isspace(*p))
            ++p;
         n = p;
         while(*n && !isspace(*n))
            ++n;
         *n = '\0';
         if (inet_aton(p, &dns_setup.nslist[dns_setup.nscount].sin_addr) != 0) {
            dns_setup.nslist[dns_setup.nscount].sin_family = AF_INET;
            dns_setup.nslist[dns_setup.nscount].sin_port = htons(PORT_DOMAIN);
            ++dns_setup.nscount;
         }
      }
   }
   (void)fclose(f);

   if (dns_setup.nscount == 0)
      return -1;

   dns_setup.s = socket(AF_INET, SOCK_DGRAM, 0);
   if (dns_setup.s == -1)
      return -1;

   (void)setsockopt(dns_setup.s, SOL_SOCKET, SO_BROADCAST, &st, st);
   if (io_register(dns_setup.s) == -1) {
      (void)close(dns_setup.s);
      dns_setup.s = -1;
      return -1;
   }

   if (io_settimer(dns_duty) == -1) {
      (void)io_close(&dns_setup.s);
      return -1;
   }

   if (atexit(dns_destroy)) {
      (void)io_close(&dns_setup.s); 
      (void)io_unsettimer(dns_duty);
      return -1;
   }

   dns_initalized = 1;
   (void)io_wantread(dns_setup.s, dns_canread);
   return 0;
}


void dns_destroy(void)
{
   dns_req_t *req;
   
   if (!dns_initalized)
      return;

   while (!list_empty(&dns_queries)) {
      req = list_entry(dns_queries.next, dns_req_t, elm);
      list_del(&req->elm);
      free(req);
   }

   if (dns_setup.s != -1)
   	io_close(&dns_setup.s);

   io_unsettimer(dns_duty);
   dns_initalized = 0;
}

dns_req_t *dns_findid(unsigned short id)
{
   dns_req_t *req;

   if (list_empty(&dns_queries))
      return NULL;
   
   list_for_each_entry(req, &dns_queries, elm) {
      if (req->id == id)
         return req;
   }
   return NULL;
}

unsigned short dns_genid(void)
{
   unsigned short ret;
   int r = rand(), i = 0;
   
   do {
      ret = dns_setup.curid + i++ + (r & 0xffff);
   } while(dns_findid(ret));
   return ret;
}

char *dns_comp(const char *host, char *comp, char *end)
{
   int x;

   if (comp > end)
      return NULL;

   while(*host) {
      for (x = 0; host[x]; ++x)
         if (host[x] == '.')
            break;
      if (x > 63)
         return NULL;
      if (comp+x > end)
         return NULL;
      *(comp++) = x;
      while (x--) *(comp++) = *(host++);
      if (*host == '.') ++host; 
   }
   if (comp+1 > end)
      return NULL;
   *(comp++) = '\0';
   return comp;
}

char *dns_expand(char *comp, char *base, char *end, char *host, char *hend)
{
   unsigned short x;

   if (comp > end || host > hend)
      return NULL;

   while (*comp) {
      if (comp < end) {
         x = ntohs(*((unsigned short*)comp));
         if ((x & 0xc000) == 0xc000) {
            x = (x & ~0xc000);
            if (base + x >= comp)
               return NULL;
            if (!dns_expand(base +x, base, end, host, hend))
               return NULL;
            host = host + strlen(host);
            return comp + 2;
         }
      }
      x = *((unsigned char*)comp++);
      if (comp+x > end)
         return NULL;
      if (host+x+1 > hend)
         return NULL;
      while (x--) *(host++) = *(comp++);
      if (!*comp)
         *host = '\0';
      else
         *(host++) = '.';
   }
   return comp + 1;
}

dns_req_t *dns_query( const char *host, unsigned short class,
      unsigned short type)
{
   dns_req_t *req;
   dns_header_t *hptr;
   char *p;

   if (dns_setup.nscount == 0 || dns_setup.s == -1)
      return NULL;
   req = malloc(sizeof(dns_req_t));
   if (!req)
      return NULL;
   memset(req, 0, sizeof(dns_req_t));

   req->id = dns_genid();
   hptr = (dns_header_t*)req->buf;
   p = (char*)req->buf + sizeof(dns_header_t);
   memset(hptr, 0, sizeof(dns_header_t));
   hptr->id = htons(req->id);
   hptr->rd = 1;
   hptr->qdcount = htons((1));
   if (!(p = dns_comp(host, p, req->buf + DNSBUF))){
      free(req);
      return NULL;
   }
   *((unsigned short*)p) = htons(type);
   p += sizeof(unsigned short);
   *((unsigned short*)p) = htons(class);
   p += sizeof(unsigned short);
   req->len = p - req->buf;
   list_add(&req->elm, &dns_queries);
   (void)io_wantwrite(dns_setup.s, dns_canwrite);
   return req;
}

int dns_host2ip( const char *host, void (*cb)(struct in_addr*, int, void*),
      void *ptr)
{
   dns_req_t *req;
  
   if (!host || !cb)
      return -1;

   req = dns_query(host, CLASS_IN, TYPE_A);
   if (!req)
      return -1;
   req->type = DNS_T_HOST2IP;
   req->cbi = cb;
   req->ptr = ptr;
   return 0;
}

int dns_ip2host(unsigned long ip, void (*cb)(char*, void*), void *ptr)
{
   dns_req_t *req;
   char tmp[128];

   if (!ip || !cb)
      return -1;
   
   snprintf(tmp, sizeof(tmp), "%lu.%lu.%lu.%lu.in-addr.arpa",
      (ip >> 24) & 0xff, (ip >> 16) & 0xff,
      (ip >> 8) & 0xff, ip & 0xff);
   req = dns_query(tmp, CLASS_IN, TYPE_PTR);
   if (!req)
      return -1;
   req->type = DNS_T_IP2HOST;
   req->cbh = cb;
   req->ptr = ptr;
   return 0;
}

#ifdef IPV6
int dns_host2ip6(const char *host, void (*cb)(struct in6_addr*, int, void*),
      void *ptr)
{
   dns_req_t *req;

   if (!host || !cb)
      return -1;
   
   req = dns_query(host, CLASS_IN, TYPE_AAAA);
   if (!req)
      return -1;
   req->type = DNS_T_HOST2IP6;
   req->cbi6 = cb;
   req->ptr = ptr;
   return 0;
}


int dns_ip62host(struct in6_addr* ip, void (*cb)(char*, void*), void *ptr)
{
   dns_req_t *req;
   char tmp[512];

   if (!ip || !cb)
      return -1;

   snprintf(tmp, sizeof(tmp), "%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x"
         ".%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.ip6.arpa",
         ip->s6_addr[0] & 0xf, ip->s6_addr[0] >> 4, ip->s6_addr[1] & 0xf, 
         ip->s6_addr[1] >> 4, ip->s6_addr[2] & 0xf, ip->s6_addr[2] >> 4,
         ip->s6_addr[3] & 0xf, ip->s6_addr[3] >> 4, ip->s6_addr[4] & 0xf,
         ip->s6_addr[4] >> 4, ip->s6_addr[5] & 0xf, ip->s6_addr[5] >> 4,
         ip->s6_addr[6] & 0xf, ip->s6_addr[6] >> 4, ip->s6_addr[7] & 0xf,
         ip->s6_addr[7] >> 4, ip->s6_addr[8] & 0xf, ip->s6_addr[8] >> 4,
         ip->s6_addr[9] & 0xf, ip->s6_addr[9] >> 4, ip->s6_addr[10] & 0xf,
         ip->s6_addr[10] >> 4, ip->s6_addr[11] & 0xf, ip->s6_addr[11] >> 4,
         ip->s6_addr[12] & 0xf, ip->s6_addr[12] >> 4, ip->s6_addr[13] & 0xf,
         ip->s6_addr[13] >> 4, ip->s6_addr[14] & 0xf, ip->s6_addr[14] >> 4,
         ip->s6_addr[15] & 0xf, ip->s6_addr[15] >> 4);
   req = dns_query(tmp, CLASS_IN, TYPE_PTR);
   if (!req)
      return -1;
   req->type = DNS_T_IP62HOST;
   req->cbh = cb;
   req->ptr = ptr;
   return 0;
}
#endif

int dns_cancel(void *ptr)
{
   dns_req_t *req;
   
   list_for_each_entry(req, &dns_queries, elm) {
      if (req->ptr == ptr) {
         list_del(&req->elm);
         free(req);
         return 0;
      }
   }
   return -1;   
}


