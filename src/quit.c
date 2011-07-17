/* 
 * quit.c -- Let the bot quit gracefully
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

extern int running;

int reply(info_t * in) {
   if (in->cmd != cmd_privmsg)
      return 0;

   if (CHECKAUTH(in->sender, UL_MASTER)) {
      in->tail = skip_nick(in->tail, in->me);
      if (!tail_cmd(&in->tail, "quit")) {
         irc_privmsg(to_sender(in), "Bye.\n");
         irc_send("QUIT :%s", nstr(in->tail));
         runlevel = RL_OFFLINE;
      } else if (!tail_cmd(&in->tail, "restart")) {
         irc_privmsg(to_sender(in), "BRB\n");
         irc_send("QUIT :Restart...");
         runlevel = RL_RESTART;
      } else if (!tail_cmd(&in->tail, "reload")) {
         irc_privmsg(to_sender(in), "Reloading config...\n");
         runlevel = RL_RELOAD;
      }
   } 
   return 0;
}

PLUGIN_DEF(NULL, NULL, reply, NULL, NULL, PLUGIN_DEP("auth.so"), PLUGIN_NOHELP)

