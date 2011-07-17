/* 
 * jukebox.c -- Never gona give you up...
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

#include "irc.h"
#include "horde.h"
#include "auth.h"
#include "conf.h"
#include "util.h"

#define N   10

int reply(info_t * in) {
   int i, n;

   if (in->cmd == cmd_privmsg) {
      in->tail = skip_nick(in->tail, in->me);
      if (!tail_cmd(&in->tail, "jukebox")) {
         char buf[N][BUFSIZE];
         char *lst[N+1];
         char *p;

         if (!CHECKAUTH(in->sender, UL_OP)) {
            irc_notice(in->sender_nick, "Singing permission denied.");
            return 0;
         }
         p = tail_getp(&in->tail);
         if (!p) {
            irc_privmsg(to_sender(in), "No victim.");
            return 0;
         }
         n = MIN(N, config_getcnt("jukebox.so", "text"));
         if (!n) {
            irc_privmsg(to_sender(in), "No text.");
            return 0;
         }
         irc_privmsg(to_sender(in), "Singing to %s.", p);
         for (i = 0; i < n; ++i) {
            snprintf(buf[i], BUFSIZE, "NOTICE %s :%s", p,
               config_getn("jukebox.so", "text", i));
            lst[i] = buf[i];
         }
         lst[i] = NULL;
         HORDE_SENDB(lst);
      }
   }
   return 0;
}

PLUGIN_DEF(NULL, NULL, reply, NULL, NULL, PLUGIN_DEP("auth.so", "horde.so"), PLUGIN_NOHELP)

