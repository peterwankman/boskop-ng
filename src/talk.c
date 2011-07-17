/* 
 * talk.c -- Talks to a user or channel
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

   if (in->cmd != cmd_privmsg)
      return 0;

   if (CHECKAUTH(in->sender, UL_OP)) {
      in->tail = skip_nick(in->tail, in->me);
      if (!tail_cmd(&in->tail,"talk")) {
         p = tail_getp(&in->tail);
         if (p) {
            irc_privmsg(p, "%s", nstr(in->tail));
	    irc_privmsg(to_sender(in), "Done.");
         } else {
            irc_privmsg(to_sender(in), "Usage: %stalk <username/channel> <message>", getprefix());
         }
      } else if(!tail_cmd(&in->tail, "notice")) {
         p = tail_getp(&in->tail);
         if (p) {
            irc_notice(p, "%s", nstr(in->tail));
            irc_privmsg(to_sender(in), "Done.");
         } else {
            irc_privmsg(to_sender(in), "Usage: %snotice <username/channel> <message>", getprefix());
         }
      } 
   }
   return 0;
}


PLUGIN_DEF(NULL, NULL, reply, NULL, NULL, PLUGIN_DEP("auth.so"), PLUGIN_NOHELP)


