/* 
 * reminder.c -- A reminder
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
#include "list.h"
#include "join.h"

#define MAX_TIMER       100

typedef struct {
   struct list_head lst;
   char nick[NICKLEN+1];
   char channel[CHANLEN+1];
   char msg[BUFLEN+1];
   unsigned long timeout;
} reminder_t;

static LIST_HEAD(reminders);
static unsigned int remindercnt = 0;

void timer(unsigned long ts) {
   reminder_t *t, *nt;

   list_for_each_entry_safe (t, nt, &reminders, lst) {
      if (t->timeout <= ts) {
         if (!t->channel[0]) {
            irc_privmsg(t->nick, "Reminder: %s", t->msg);
         } else {
            if (GET_USERLVL(t->channel, t->nick) != -1)
               irc_privmsg(t->channel, "%s: %s", t->nick, t->msg);
         }
         list_del(&t->lst);
         --remindercnt;
         free(t);
      }
   }  
}

int reply(info_t * in) {
   reminder_t *t;
   unsigned long duration;
   char *d, *m;
   char mod  = 0;

   if (in->cmd == cmd_privmsg) {
      if (!tail_cmd(&in->tail, "reminder")) {
         d = tail_getp(&in->tail);
         m = in->tail;
         if (!m || !m[0]) {
            irc_notice(in->sender_nick, "Usage: %sreminder <duration> <message>", getprefix());
            return 0;
         }
         if (remindercnt >= MAX_TIMER) {
            irc_notice(in->sender_nick, "Too many reminders set already");
            return 0;
         }
         if (sscanf(d, "%lu%c", &duration, &mod) < 1) {
            irc_notice(in->sender_nick, "Invalid duration");
            return 0;
         }
         switch(mod) {
            case 'm':
            case 'M':
               duration *= 60;
               break;
            case 'h':
            case 'H':
               duration *= 3600;
               break;
         }
         if (duration > 31536000) {
            irc_notice(in->sender_nick, "Duration too long");
            return 0;
         }
         t = calloc(1, sizeof(reminder_t));
         if(!t)
            return 0;
         strnzcpy(t->nick, in->sender_nick, NICKLEN+1);
         if (is_channel(in->argv[0]))
            strnzcpy(t->channel, in->argv[0], CHANLEN+1);
         strnzcpy(t->msg, m, BUFLEN+1);
         t->timeout = time(NULL)+duration;
         list_add(&t->lst, &reminders);
         ++remindercnt;
         irc_notice(in->sender_nick, "Reminder %d added.", remindercnt);
      }
   } else if (in->cmd == cmd_nick) {
      list_for_each_entry(t, &reminders, lst) {
         if (!strcasecmp(in->sender_nick, t->nick))
            strnzcpy(t->nick, in->tail, NICKLEN+1);
      }
   }
   return 0;
}

void destroy(void)
{
   reminder_t *t;
   while (!list_empty(&reminders)) {
      t = list_entry((&reminders)->next, reminder_t, lst);
      list_del(&t->lst);
      free(t);
   }
   remindercnt = 0;
}

PLUGIN_DEF(NULL, destroy, reply, timer, NULL, PLUGIN_DEP("join.so"), PLUGIN_HELP("!reminder <time> <message>"))

