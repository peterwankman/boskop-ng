/* 
 * achtball.c -- Answer questions
 * 
 * Copyright (C) 2010  Martin Wolters et al.
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
#include "conf.h"
#include "rrand.h"

int reply(info_t * in) {
   int cnt;

   if (in->cmd == cmd_privmsg) {
      in->tail = skip_nick(in->tail, in->me);
      if (!regex(in->tail, "/(^|(\\.|\\?)\\s*)((oder|und)\\s+|)((bin|soll|sollte|darf|werde)\\s+ich|(bist|willst|wirst)\\s+du|(ist|wird)\\s+(das|dies|er|sie|es)|(sind|sollen|duerfen|dürfen|werden)\\s+wir|(seid|werdet)\\s+ihr)/i")) {
         cnt = config_getcnt("achtball.so", "answer");
         irc_privmsg(to_sender(in), "%s",
            config_getn("achtball.so", "answer", rrand(cnt)));      
      }
   }
   return 0;
}

PLUGIN_DEF(NULL, NULL, reply, NULL, NULL, PLUGIN_NODEP, PLUGIN_NOHELP)


