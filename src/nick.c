/* 
 * nick.c -- Nick function
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
#include "auth.h"
#include "util.h"

int reply(info_t * in) {
   char *p;

   if (in->cmd == cmd_numeric && in->numeric == 1) {
      strnzcpy(in->me, in->argv[0], NICKLEN+1);
   } else if (in->cmd == cmd_nick && !strcasecmp(in->sender_nick, in->me)) {
      if (strlen(in->tail))
        strnzcpy(in->me, in->tail, NICKLEN+1);
      else if (strlen(in->argv[0]))
	strnzcpy(in->me, in->argv[0], NICKLEN+1);
   } else if (in->cmd == cmd_numeric && in->numeric == 433 && !in->me[0]) {
      irc_send("NICK %s_", in->argv[1]);
   } else if (in->cmd == cmd_privmsg && CHECKAUTH(in->sender, UL_OP)) {
      in->tail = skip_nick(in->tail, in->me);
      if(!tail_cmd(&in->tail, "nick") && (p = tail_getp(&in->tail)))
         irc_send("NICK %s", p);
      if(!tail_cmd(&in->tail, "whoareyou"))
	irc_privmsg(to_sender(in), "I am %s", in->me);
   }
   return 0;
}

PLUGIN_DEF(NULL, NULL, reply, NULL, NULL, PLUGIN_NODEP, PLUGIN_NOHELP)

