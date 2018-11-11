/* 
 * blinddate.c -- Initiates conversations
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

#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "irc.h"
#include "auth.h"
#include "util.h"
#include "conf.h"
#include "rrand.h"

static char channel[CHANLEN+1];
static char nick[2][NICKLEN+1];
static int blinddate = 0;

static int mmemcpy(char **dest, const char *src, int len, int *free)
{
   len = MIN(len, *free);
   if (len > 0)
      memcpy(*dest, src, len);
   *free -= len;
   *dest += len;
   return len;
}

static void talk(int nnick, char *line, info_t *in) {
   char buf[BUFSIZ];
   char *m, *o, *rp = line, *wp = buf;
   int free = BUFSIZ-1;

   if (strlen(in->me) && strlen(nick[nnick?0:1]) 
	   && strcasecmp(in->me, nick[nnick?0:1])) {
   for(;;) {
      m = strcasestr(rp, in->me);
      o = strcasestr(rp, nick[nnick?0:1]);
      
      if (m && (!o || m < o)) {
         mmemcpy(&wp, rp, m-rp, &free);
         rp = m+strlen(in->me);
         mmemcpy(&wp, nick[nnick], strlen(nick[nnick]), &free);
      } else if (o) {
         mmemcpy(&wp, rp, o-rp, &free); 
         rp = o+strlen(nick[nnick?0:1]);
         mmemcpy(&wp, in->me, strlen(in->me), &free);
      } else {
         break;
      }
   }
   }
   mmemcpy(&wp, rp, strlen(rp), &free);
   *wp = '\0';

   irc_privmsg(nick[nnick], "%s", buf);
 
   irc_privmsg(channel, "%s: %s", nick[nnick?0:1], buf);
}

int reply(info_t *in) {
   char *u1, *u2;
   int j, k;

   if (in->cmd != cmd_privmsg)
      return 0;

   if (!tail_cmd(&in->tail, "blinddate")) {
      if(!CHECKAUTH(in->sender, UL_OP)) {
         irc_notice(in->sender_nick, "Blinddate permission denied.");
         return 0;
   	  }
      u1 = tail_getp(&in->tail);
      u2 = tail_getp(&in->tail);

      if (u1 && !strcasecmp(u1, "off")) {
         irc_privmsg(to_sender(in), "Blinddate is over.");
         blinddate = 0;
         return 0;
      }

      if (!u1 || !u2) {
         irc_privmsg(to_sender(in), "Usage: %sblinddate <user1> <user2>", getprefix());
         blinddate = 0;
         return 0;
      }

      if (is_channel(u1) || is_channel(u2)) {
         irc_privmsg(to_sender(in), "One argument is a channel.");
         blinddate = 0;
         return 0;
      }

      j = config_getcnt("blinddate.so", "greet");
      if (j == 0) {
         irc_privmsg(to_sender(in), "No greetings found.");
         blinddate = 0;
         return 0;
      }
      strnzcpy(nick[0], u1, NICKLEN+1);
      strnzcpy(nick[1], u2, NICKLEN+1);
      strnzcpy(channel, to_sender(in), CHANLEN+1);

      irc_privmsg(to_sender(in), "Starting blinddate between %s and %s.",
         nick[0], nick[1]);
      blinddate = 1;

      talk(0, config_getn("blinddate.so", "greet", rrand(j)), in);
      talk(1, config_getn("blinddate.so", "greet", rrand(j)), in);
   }
   if(blinddate) {
      if (CHECKAUTH(in->sender, UL_OP)){
         if(!tail_cmd(&in->tail, "say1")) {
            talk(0, in->tail, in);
         } else if (!tail_cmd(&in->tail, "say2")) {
            talk(1, in->tail, in);
         } else if (!tail_cmd(&in->tail, "btourette1")) {
            char buf[100];
            if (!irc_query("tourette", buf, sizeof(buf)))
               talk(0, buf, in);
         } else if(!tail_cmd(&in->tail, "btourette2")) {
            char buf[100];
            if (!irc_query("tourette", buf, sizeof(buf)))
               talk(1, buf, in);
         }
      }
      
      if(!strcasecmp(in->argv[0], in->me)) {
         j = -1;
         for(k = 0; k < 2; k++)
            if(!strcasecmp(in->sender_nick, nick[k]))
               j = k?0:1;
         if(j > -1)
            talk(j, in->tail, in);
      }
   }
   return 0;
}

PLUGIN_DEF(NULL, NULL, reply, NULL, NULL, PLUGIN_DEP("auth.so", "nick.so"), PLUGIN_NOHELP)

