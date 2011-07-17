/* 
 * dns.h -- dns.c header
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

#ifndef DNS_H_
#define DNS_H_

#include <netinet/in.h>

#include "list.h"

/*
 * http://www.iana.org/assignments/dns-parameters
 */

#define CLASS_IN        1
#define CLASS_CS        2
#define CLASS_CH        3
#define CLASS_HS        4
#define CLASS_NONE      254
#define CLASS_ANY       255

#define TYPE_A          1
#define TYPE_NS         2
#define TYPE_MD         3
#define TYPE_MF         4
#define TYPE_CNAME      5
#define TYPE_SOA        6
#define TYPE_MB         7
#define TYPE_MG         8
#define TYPE_MR         9
#define TYPE_NULL       10
#define TYPE_WKS        11
#define TYPE_PTR        12
#define TYPE_HINFO      13
#define TYPE_MINFO      14
#define TYPE_MX         15
#define TYPE_TXT        16
#define TYPE_RP         17
#define TYPE_AFSDB      18
#define TYPE_X25        19
#define TYPE_ISDN       20
#define TYPE_RT         21
#define TYPE_NSAP       22
#define TYPE_NSAP_PTR   23
#define TYPE_SIG        24
#define TYPE_KEY        25
#define TYPE_PX         26
#define TYPE_GPOS       27
#define TYPE_AAAA       28
#define TYPE_LOC        29
#define TYPE_NXT        30
#define TYPE_EID        31
#define TYPE_NIMLOC     32
#define TYPE_SRV        33
#define TYPE_ATMA       34
#define TYPE_NAPTR      35
#define TYPE_KX         36
#define TYPE_CERT       37
#define TYPE_A6         38
#define TYPE_DNAME      39
#define TYPE_SINK       40
#define TYPE_OPT        41
#define TYPE_APL        42
#define TYPE_DS         43
#define TYPE_SSHFP      44
#define TYPE_IPSECKEY   45
#define TYPE_RRSIG      46
#define TYPE_NSEC       47
#define TYPE_DNSKEY     48
#define TYPE_DHCID      49
#define TYPE_NSEC3      50
#define TYPE_NSEC3PARAM 51
#define TYPE_HIP        55
#define TYPE_NINFO      56
#define TTYPE_RKEY      57
#define TYPE_SPF        99
#define TYPE_UINFO      100
#define TYPE_UID        101
#define TYPE_GID        102
#define TYPE_UNSPEC     103
#define TYPE_TKEY       249
#define TYPE_TSIG       250
#define TYPE_IXFR       251
#define TYPE_AXFR       252
#define TYPE_MAILB      253
#define TYPE_MAILA      254
#define TYPE_ANY        255
#define TYPE_TA         32768
#define TYPE_DLV        32769

#define OPCODE_QUERY    0
#define OPCODE_IQUERY   1
#define OPCODE_STATUS   2
#define OPCODE_NOTIFY   4
#define OPCODE_UPDATE   5

#define RCODE_NOERROR   0
#define RCODE_FORMERR   1
#define RCODE_SERVFAIL  2
#define RCODE_NXDOMAIN  3
#define RCODE_NOTIMP    4
#define RCODE_REFUSED   5
#define RCODE_YXDOMAIN  6
#define RCODE_YXRRSET   7
#define RCODE_NXRRSET   8
#define RCODE_NOTAUTH   9
#define RCODE_NOTZONE   10
#define RCODE_BADVERS   16
#define RCODE_BADSIG    16
#define RCODE_BADKEY    17
#define RCODE_BADTIME   18
#define RCODE_BADMODE   19
#define RCODE_BADNAME   20
#define RCODE_BADALG    21
#define RCODE_BADTRUNC  22

#define PORT_DOMAIN     53

#ifndef RESOLV_CONF
#       ifdef  _PATH_RESCONF
#               define  RESOLV_CONF     _PATH_RESCONF
#       else
#               define RESOLV_CONF      "/etc/resolv.conf"
#       endif
#endif

#define DNSBUF          512
#define NSMAX           10


#ifndef MIN
#define MIN(x,y)        ((x>y)?(x):(y))
#endif

typedef struct {
   unsigned short id;
   unsigned short rd:1;
   unsigned short tc:1;
   unsigned short aa:1;
   unsigned short opcode:4;
   unsigned short qr:1;
   unsigned short rcode:4;
   unsigned short cd:1;
   unsigned short ad:1;
   unsigned short reserved:1;
   unsigned short ra:1;
   unsigned short qdcount;
   unsigned short ancount;
   unsigned short nscount;
   unsigned short arcount;
}  __attribute__((packed)) dns_header_t;

typedef struct {
   unsigned short qtype;
   unsigned short qclass;
} __attribute__((packed)) dns_query_t;

typedef struct {
   unsigned short type;
   unsigned short class;
   unsigned int ttl;
   unsigned short rdlength;
   char rdata[0];
} __attribute__((packed)) dns_record_t;

typedef enum {
   DNS_S_NONE = 0,
   DNS_S_SENT
} dns_state_t;

typedef enum {
   DNS_T_NONE = 0,
   DNS_T_HOST2IP,
   DNS_T_IP2HOST,
#ifdef IPV6
   DNS_T_IP62HOST,
   DNS_T_HOST2IP6,
#endif
} dns_type_t;

typedef struct {
   dns_state_t state;
   dns_type_t type;
   unsigned short id;
   char buf[DNSBUF];
   int len, retries;
   unsigned long lasttry;
   void *ptr;
   union {
      void (*cbi) (struct in_addr *, int, void *);
      void (*cbh) (char *, void *);
#ifdef IPV6
      void (*cbi6) (struct in6_addr *, int, void *);
#endif
   };
   struct list_head elm;
} dns_req_t;


typedef struct {
   struct sockaddr_in nslist[NSMAX];
   int nscount, nsnext;
   int s;
   unsigned short curid;
   unsigned long lastduty;
} dns_setup_t;

char *dns_parse_query(char *buf, char *ptr, char *end, char *host, int hlen,
   dns_query_t** qry);
char *dns_parse_answer(char *buf, char *ptr, char *end, char *host, int hlen,
   dns_record_t **rec);
int dns_canread(void *ptr);
int dns_canwrite(void *ptr);
void dns_duty(unsigned long now);
int dns_init(void);
void dns_destroy(void);
dns_req_t *dns_findid(unsigned short id);
unsigned short dns_genid(void);
char *dns_comp(const char *host, char *comp, char *end);
char *dns_expand(char *comp, char *base, char *end, char *host, char *hend);
dns_req_t *dns_query(const char *host, unsigned short class,
   unsigned short type);
int dns_host2ip(const char *host, void (*cb)(struct in_addr*, int, void*),
      void *ptr);
int dns_ip2host(unsigned long ip, void (*cb)(char*, void*), void *ptr);
#ifdef IPV6
int dns_host2ip6(const char *host, void (*cb)(struct in6_addr*, int, void*),
      void *ptr);
int dns_ip62host(struct in6_addr*, void (*cb)(char*, void*), void *ptr);
#endif
int dns_cancel(void *ptr);

#endif

