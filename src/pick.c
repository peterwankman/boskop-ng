/* 
 * pick.c -- Pick an random answer from a list of choices.
 * 
 * Copyright (C) 2009-2018  Martin Wolters et al.
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
#include <stdlib.h>
#include <time.h>
#include <ctype.h>

#include "irc.h"
#include "conf.h"
#include "util.h"
#include "rrand.h"

static time_t last;

static int interval;
static int nrare = 0;
static int unlikelihood = 0;

static char *trigger = NULL;

int reply(info_t * in) {
   time_t now = time(NULL);
   int nreply, rarereply = 0;
   
   if (last + interval > now)
      return 0;

   if (in->cmd == cmd_privmsg) {
	   if (!tail_cmd(&in->tail, trigger)) {
	   	 nreply = config_getcnt("pick.so", "reply");

		 if((nrare > 0) && (unlikelihood > 0) && (rrand(unlikelihood) == 0))
			 rarereply = 1;

		 if(rarereply) {
			 irc_privmsg(to_sender(in), "%s",
			   config_getn("pick.so", "rarereply", rrand(nrare)));
		 } else {
			 irc_privmsg(to_sender(in), "%s",
			   config_getn("pick.so", "reply", rrand(nreply)));
		 }
	   }
   }
   return 0;
}

int init() {
   last = 0;
   char *buf;

   trigger = config_get("pick.so", "trigger");
   nrare = config_getcnt("pick.so", "rarereply");
   buf = config_get("pick.so", "unlikelihood");
   if(buf) unlikelihood = atoi(buf);

   return trigger != NULL ? 0 : -1;
}


PLUGIN_DEF(init, NULL, reply, NULL, NULL, PLUGIN_NODEP, PLUGIN_HELP("I make the hard choices for you"))

