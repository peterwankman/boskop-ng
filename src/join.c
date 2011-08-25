/* 
 * join.c -- Plugin for channel handling
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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "irc.h"
#include "util.h"
#include "conf.h"
#include "auth.h"
#include "join.h"

#define REJOIN_FREQ 181
#define MAX_RETRIES 5

int max_retries;
int rejoin_freq;

typedef struct {
   char nick[NICKLEN+1];
   char user[USERLEN+1];
   char host[HOSTLEN+1];
   int flags;
   struct list_head elm;
} member_t;

typedef struct {
   char name[CHANLEN+1];
   char passwd[PASSLEN+1];
   int userlimit;
   int flags;
   int retries;
   struct list_head elm;
   struct list_head members;
} channel_t;

static time_t last_rejoin;
static LIST_HEAD(channels);


static char uflag2chr(int flags)
{
   if (flags & UFL_OP)
      return '@';
   if (flags & UFL_HOP)
      return '%';
   if (flags & UFL_VOICE)
      return '+';
   return ' ';
}

static inline int is_userflag(char c)
{
   if (c == '@' || c == '%' || c == '+')
      return 1;
   return 0;
}

static member_t *member_find(channel_t *ch, const char *nick)
{
   member_t *m;

   if (!ch || !nick || list_empty(&ch->members))
      return NULL;
   
   while (is_userflag(*nick))
      ++nick;

   list_for_each_entry(m, &(ch->members), elm) {
      if (!strcasecmp(nick, m->nick))
         return m;
   }
   return NULL;
}

static member_t *member_add(channel_t *ch, const char *nick, const char *user, const char *host, int flags)
{
   member_t *m;
   
   if(!ch || !nick)
      return NULL;
   ch->retries = 0;

   if ((m = member_find(ch, nick)))
      return m;

   m = malloc(sizeof(member_t));
   if (!m)
      return NULL;

   m->flags = flags;
   
   while(*nick == '@') {
      m->flags |= UFL_OP;
      ++nick;
   }
   while(*nick == '%') {
      m->flags |= UFL_HOP;
      ++nick;
   }
   while(*nick == '+') {
      m->flags |= UFL_VOICE;
      ++nick;
   }

   strnzcpy(m->nick, nick, NICKLEN+1);
   if (user)
      strnzcpy(m->user, user, USERLEN+1);
   else
      m->user[0] = '\0';

   if (host)
      strnzcpy(m->host, host, HOSTLEN+1);
   else
      m->host[0] = '\0';

   list_add(&m->elm, &ch->members);
   return m;
}

static void member_del(member_t *m)
{
   if (!m)
      return;

   list_del(&m->elm);
   free(m);
}

static void member_delall(channel_t *ch)
{
   member_t *m, *n;
   
   if (!ch || list_empty(&ch->members))
      return;

   list_for_each_entry_safe(m, n, &ch->members, elm)
      member_del(m);
}

static int member_cnt(channel_t *ch)
{
   member_t *m;
   int n = 0;
  
   if (!ch || list_empty(&ch->members))
      return 0;
  
   list_for_each_entry(m, &(ch->members), elm)
      ++n;
   return n;
}

static channel_t *channel_find(const char *name)
{
   channel_t *ch;

   if (!name || list_empty(&channels))
      return NULL;

   list_for_each_entry(ch, &channels, elm)
      if (!strcasecmp(name, ch->name))
         return ch;
   return NULL;
}

static channel_t *channel_add(char *name, char *passwd)
{
   channel_t *ch;
   char *n, *p;

   if (!name || !name[0])
      return NULL;

   if (!passwd && (passwd = strchr(name, ' '))) {
      *(passwd++) = '\0';
      if ((p = strchr(passwd, ' ')))
         *p = '\0';
   }
   
   while ((n = strchr(name, ','))) {
      *(n++) = '\0';
      if (passwd) {
         if ((p = strchr(passwd, ',')))
            *(p++) = '\0';
      } else {
         p = NULL;
      }
      (void)channel_add(name, passwd);
      name = n;
      passwd = p;
   }

   if ((ch = channel_find(name)))
      return ch;

   ch = calloc(1,sizeof(channel_t));
   if (!ch)
      return NULL;

   strnzcpy(ch->name, name, CHANLEN+1);
   if (passwd)
      strnzcpy(ch->passwd, passwd, PASSLEN+1);
   INIT_LIST_HEAD(&ch->members);
   list_add(&ch->elm, &channels);
   return ch;
}

static void channel_del(channel_t *ch)
{
   if (!ch)
      return;
   member_delall(ch);
   list_del(&ch->elm);
   free(ch);
}

static void channel_modes(channel_t *ch, const char *mode, const char **argv)
{
   member_t *m;
   int ind = 0;
   const char *mo = mode+1;
   
   if (!ch)
      return;
   
   if (mode[0] == '+') {
      while (*mo) {
         switch(*mo) {
            case 'o':
               if ((m = member_find(ch, argv[ind++])))
                  m->flags |= UFL_OP;
               break;
            case 'h':
               if ((m = member_find(ch, argv[ind++])))
                  m->flags |= UFL_HOP;
               break;
            case 'v':
               if ((m = member_find(ch, argv[ind++])))
                  m->flags |= UFL_VOICE;
               break;
            case 'k':
               strnzcpy(ch->passwd, argv[ind++], PASSLEN+1);
               break;
            case 'l':
               ch->userlimit = atoi(argv[ind++]);
               break;
            case 'b':
               ++ind;
               break;
            case 'p':
               ch->flags |= CFL_PRIVATE;
               break;
            case 's':
               ch->flags |= CFL_SECRET;
               break;
            case 'i':
               ch->flags |= CFL_INVITE;
               break;
            case 't':
               ch->flags |= CFL_TOPICOP;
               break;
            case 'n':
               ch->flags |= CFL_NOEXMSG;
               break;
            case 'm':
               ch->flags |= CFL_MOD;
               break;
         }
         ++mo;
      }
   } else if (mode[0] == '-') {
      while (*mo) {
         switch(*mo) {
               case 'o':
               if ((m = member_find(ch, argv[ind++])))
                  m->flags &= ~UFL_OP;
               break;
            case 'h':
               if ((m = member_find(ch, argv[ind++])))
                  m->flags &= ~UFL_HOP;
               break;
            case 'v':
               if ((m = member_find(ch, argv[ind++])))
                  m->flags &= ~UFL_VOICE;
               break;
            case 'k':
               ch->passwd[0] = '\0';
               ++ind;
               break;
            case 'l':
               ch->userlimit = 0;
               break;
            case 'b':
               ++ind;
               break;
            case 'p':
               ch->flags &= ~CFL_PRIVATE;
               break;
            case 's':
               ch->flags &= ~CFL_SECRET;
               break;
            case 'i':
               ch->flags &= ~CFL_INVITE;
               break;
            case 't':
               ch->flags &= ~CFL_TOPICOP;
               break;
            case 'n':
               ch->flags &= ~CFL_NOEXMSG;
               break;
            case 'm':
               ch->flags &= ~CFL_MOD;
               break;
         }
      ++mo;
      }
   }
}

static int channel_cnt(void)
{
  channel_t *ch;
  int n = 0;
  
  if (list_empty(&channels))
    return 0;
  
  list_for_each_entry(ch, &channels, elm)
      ++n;
  return n;
}

static void try_rejoin(int reconnected)
{
   channel_t *ch;
   if (!list_empty(&channels) && irc_query("qauth", 1)) {
      list_for_each_entry(ch, &channels, elm) {
        if (reconnected) {
          irc_send("JOIN %s %s", ch->name, nstr(ch->passwd));
          ch->retries = 0;
        } else if (list_empty(&ch->members) && ch->retries < max_retries) {
          irc_send("JOIN %s %s", ch->name, nstr(ch->passwd));
          ++ch->retries;
        }
     } 
  }
}

static void nickchange(const char *old, const char *new)
{
   channel_t *ch;
   member_t *m, *n;

   if (list_empty(&channels))
      return;

   list_for_each_entry(ch, &channels, elm) {
      if (list_empty(&ch->members))
         continue;
      list_for_each_entry_safe(m, n, &(ch->members), elm) {
         if (!strcasecmp(m->nick, old)) {
            if (!new) {
               member_del(m);
            } else {
               strnzcpy(m->nick, new, NICKLEN+1);
            }
         }
      }
   }
}

int query(info_t * in, va_list ap)
{
   channel_t *ch;
   member_t *m;
   char **p;
   int n, cmd;
   
   cmd = va_arg(ap, int);
   switch(cmd) {
      case JCMD_CFL:
         ch = channel_find(va_arg(ap, char*));
         if (ch)
            return ch->flags;
         break;
      case JCMD_UFL:
         ch = channel_find(va_arg(ap, char*));
         m = member_find(ch, va_arg(ap, char*));
         if (m)
            return m->flags;
         break;
      case JCMD_LIMIT:
         ch = channel_find(va_arg(ap, char*));
         if (ch)
            return ch->userlimit;
         break;
      case JCMD_KEY:
         ch = channel_find(va_arg(ap, char*));
         p = va_arg(ap, char**);
         if (ch) {
            *p = ch->passwd;
            return strlen(ch->passwd);
         }
         break;
      case JCMD_CCNT:
         return channel_cnt();
         break;
      case JCMD_UCNT:
         ch = channel_find(va_arg(ap, char*));
         if (ch)
            return member_cnt(ch);
         break;
      case JCMD_CNAME:
        n = va_arg(ap, int);
        p = va_arg(ap, char**);
        list_for_each_entry(ch, &channels, elm) {
         if (n-- <= 0) {
           *p = ch->name;
           return strlen(ch->name);
         }
        }
      case JCMD_UNAME:
        ch = channel_find(va_arg(ap, char*));
        n = va_arg(ap, int);
        p = va_arg(ap, char**);
        if (ch) {
          list_for_each_entry(m, &(ch->members), elm) {
            if (n-- <= 0) {
               *p = m->nick;
               return strlen(m->nick);
            }
          }
        }
      default:
         break;
   }
   return -1;
}

int reply(info_t * in) {
   channel_t *ch;
   member_t *m;
   char *p;

   if (in->cmd == cmd_numeric && in->numeric == 1) {
      if (!list_empty(&channels)) {
         list_for_each_entry(ch, &channels, elm)
            member_delall(ch);
      }
      try_rejoin(1);
   } else if (in->cmd == cmd_join) {
      member_add(channel_add((in->argv[0])?in->argv[0]:in->tail, NULL), in->sender_nick, in->sender_user, in->sender_host, UFL_NONE);
   } else if(in->cmd == cmd_numeric && in->numeric == 353) {
      ch = channel_add(in->argv[2], NULL);
      while (in->tail && in->tail[0]) {
         if ((p = strchr(in->tail, ' ')))
            *(p++) = '\0';
            member_add(ch, in->tail, NULL, NULL, 0);
         in->tail = p;
      }
      irc_send("MODE %s", ch->name);
   } else if (in->cmd == cmd_part) {
     if (!strcasecmp(in->sender_nick, in->me))
      channel_del(channel_find(in->argv[0]));
     else
       member_del(member_find(channel_find(in->argv[0]), in->sender_nick)); 
   } else if(in->cmd == cmd_kick) {
     if (!strcasecmp(in->argv[1], in->me))
        member_delall(channel_find(in->argv[0]));
     else
       member_del(member_find(channel_find(in->argv[0]), in->argv[1])); 
   } else if (in->cmd == cmd_privmsg) {
       in->tail = skip_nick(in->tail, in->me);
      if (!is_channel(in->argv[0]) && !tail_cmd(&in->tail, "channels")) {
	 if (!CHECKAUTH(in->sender, UL_OP))
	    return 0;
         if (list_empty(&channels)) {
            irc_notice(in->sender_nick, "No channels in channellist");
         } else {
            list_for_each_entry(ch, &channels, elm)
               irc_notice(in->sender_nick, "%s%s", ch->name, list_empty(&ch->members)?" (not joined)":"");
            irc_notice(in->sender_nick, "End of channellist");
         }
      } else if (!is_channel(in->argv[0]) && !tail_cmd(&in->tail, "members")) {
         if (!CHECKAUTH(in->sender, UL_OP))
	    return 0;
         ch = channel_find(tail_getp(&in->tail));
         if (ch) {
            if (list_empty(&ch->members)) {
               irc_notice(in->sender_nick, "Channel %s not joined", ch->name);
            } else {
               list_for_each_entry(m, &ch->members, elm)
                  irc_notice(in->sender_nick, "%c%s", uflag2chr(m->flags), m->nick);
               irc_notice(in->sender_nick, "End of memberlist of %s", ch->name);
            }
         } else {
            irc_notice(in->sender_nick, "No such channel %s", in->tail);
         }
      } else if (!tail_cmd(&in->tail, "join")) {
         if (!CHECKAUTH(in->sender, UL_OP)) {
            irc_notice(in->sender_nick, "Join permission denied.");
            return 0;
         }
		
         p = tail_getp(&in->tail);
         channel_add(p, tail_getp(&in->tail));
		 irc_privmsg(to_sender(in), "Done.");
      } else if (!tail_cmd(&in->tail, "part")) {
         if (!CHECKAUTH(in->sender, UL_OP)) {
            irc_notice(in->sender_nick, "Part permission denied.");
            return 0;
         }
         p = tail_getp(&in->tail);
         channel_del(channel_find(p));
         irc_send("PART %s", p);
		 irc_privmsg(to_sender(in), "Done.");
      }
   } else if (in->cmd == cmd_nick) {
      nickchange(in->sender_nick, in->tail);
   } else if (in->cmd == cmd_quit) {
      nickchange(in->sender_nick, NULL);
   } else if (in->cmd == cmd_mode && (ch = channel_find(in->argv[0]))) {
     channel_modes(ch, in->argv[1], (const char**)&(in->argv[2]));
   } else if (in->cmd == cmd_numeric && in->numeric == 324 && (ch = channel_find(in->argv[1]))) {
     channel_modes(ch, in->argv[2], (const char**)&(in->argv[3]));
   }
   return 0;
}

int init(void) {
  int n, i;
  char *p;

   n  = config_getcnt("join.so", "channel");
   for (i = 0; i < n; ++i)
      channel_add(config_getn("join.so", "channel", i), NULL);
   last_rejoin = time(NULL);

   p = config_get("join.so", "retries");
   if (p && (i = atoi(p)) > 0) {
    max_retries = i;
   } else {
    max_retries = MAX_RETRIES;
   }
   
   p = config_get("join.so", "frequency");
   if (p && (i = atoi(p)) > 0) {
    rejoin_freq = i; 
   } else {
    rejoin_freq = REJOIN_FREQ;
   }
   return 0;
}

void destroy(void) {
   channel_t *ch, *n;
   list_for_each_entry_safe(ch, n, &channels, elm)
      channel_del(ch);
}

void timer(unsigned long ts) {
   if (ts - rejoin_freq > last_rejoin) {
      try_rejoin(0);
      last_rejoin = ts;
   }
}

PLUGIN_DEF(init, destroy, reply, timer, query, PLUGIN_DEP("nick.so"), PLUGIN_NOHELP)

