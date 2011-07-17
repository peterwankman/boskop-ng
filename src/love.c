/* 
 * love.c -- Spread the love
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
#include <string.h>

#include "irc.h"
#include "horde.h"
#include "auth.h"
#include "conf.h"
#include "util.h"
#include "rrand.h"

#define N   10

int reply(info_t * in) {
   int i, n;
   char buf[BUFSIZE], *p;

   if (!(in->cmd == cmd_privmsg))
      return 0;

   if (CHECKAUTH(in->sender, UL_OP)) {
      in->tail = skip_nick(in->tail, in->me);
      if (!tail_cmd(&in->tail, "love")) {
         
         p = tail_getp(&in->tail);
         if (!p) {
            irc_privmsg(to_sender(in), "No victim.");
            return 0;
         }
         n = config_getcnt("love.so", "messages");
         if (!n) {
            irc_privmsg(to_sender(in), "No messages.");
            return 0;
         }
	      irc_privmsg(to_sender(in), "Loving %s.", p);
         for (i = 0; i < N; ++i) {
            snprintf(buf, BUFSIZE, "PRIVMSG %s :%s", p,
               config_getn("love.so", "messages", rrand(n)));
            HORDE_SENDA(buf);
         }
      }
   } 
   return 0;
}

PLUGIN_DEF(NULL, NULL, reply, NULL, NULL, PLUGIN_DEP("auth.so", "horde.so"), PLUGIN_NOHELP)
