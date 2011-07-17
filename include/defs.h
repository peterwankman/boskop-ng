/* 
 * defs.h -- global definitions
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

#ifndef DEFS_H_
#define DEFS_H_

#include <stdio.h>
#include <stdarg.h>
#include <sys/time.h>

#include "list.h"
#include "sock.h"

#ifndef MIN
#define MIN(x,y)  ((x<y)?(x):(y))
#endif

#define nstr(x)         ((x)?(x):"")
#define to_sender(x)    (is_channel((x)->argv[0])?(x)->argv[0]:(x)->sender_nick)

#define DEFAULT_CONFIG	"boskop.conf"
#define LINEDELAY   1000000
#define CONNTIMEOUT (360*1000000)
#define BUFSIZE 4096
#define MAXPLUG 128
#define BUFLEN 1024
#define CHANLEN 32
#define NICKLEN 32
#define USERLEN 32
#define PASSLEN 128
#define HASHLEN 22
#define HOSTLEN 128
#define SENDERLEN NICKLEN+1+USERLEN+1+HOSTLEN+1


#define  INFO_MAXARG 25

typedef enum {
   cmd_unknown, cmd_numeric, cmd_join, cmd_part,
   cmd_privmsg, cmd_notice, cmd_kick, cmd_mode,
   cmd_invite, cmd_ping, cmd_nick, cmd_quit
} class_t;

typedef struct {
   char *me;
   char sender[SENDERLEN+1];
   char *sender_nick;
   char *sender_user;
   char *sender_host;
   class_t cmd;
   int numeric;
   char *argv[INFO_MAXARG];
   int argc;
   char *tail;
} info_t;

typedef struct {
   struct list_head elm;
   void *self;
   char *name;
   int (*init) ();
   void (*destroy) ();
   int (*reply) (info_t *);
   void (*timer) (unsigned long now);
   int (*query) (info_t *, va_list ap);
   char **depends;
   char **help;
} plugin_t;

typedef struct {
   bufsock_t *bs; 
   char me[NICKLEN+1];
   struct list_head linequeue;
   struct timeval lastline, lastdata;
} irc_t;

typedef struct {
   struct list_head elm;
   int len;
   char buf[0];
} ircline_t;


#define PLUGIN_DEF(i, d, r, t, q, dep, h) \
char *__plugin__depends[]= dep; \
char *__plugin_help[] = h;      \
plugin_t plugin_info = {          \
    .elm      = { NULL, NULL },   \
    .self     = NULL,             \
    .name     = NULL,             \
    .init     = (i),              \
    .destroy  = (d),              \
    .reply    = (r),              \
    .timer    = (t),              \
    .query    = (q),              \
    .depends  = __plugin__depends,\
    .help     = __plugin_help     \
  };

#define PLUGIN_INIT(i)     plugin_info.init = (i)
#define PLUGIN_DESTROY(d)  plugin_info.destroy = (d)
#define PLUGIN_TIMER(t)    plugin_info.timer = (t)
#define PLUGIN_REPLY(r)    plugin_info.reply = (r)
#define PLUGIN_QUERY(q)    plugin_info.query = (q)
#define PLUGIN_DEP(x...)   { x, NULL }
#define PLUGIN_NODEP       { NULL }
#define PLUGIN_HELP(x...)  { x, NULL }
#define PLUGIN_NOHELP      { NULL }

extern plugin_t plugin_info;

#endif

