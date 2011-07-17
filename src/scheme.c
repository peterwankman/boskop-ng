/* 
 * scheme.c -- Announce date and location of #scheme meeting
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
#include <stdlib.h>
#include <time.h>
#include <ctype.h>

#include "irc.h"
#include "conf.h"
#include "util.h"
#include "rrand.h"

time_t last;
int interval;

int reply(info_t * in) {
   time_t now = time(NULL);
   int day, month, ncity;
   
   if (last + interval > now)
      return 0;

   if (in->cmd == cmd_privmsg) {
	   if (!tail_cmd(&in->tail, "scheme")) {
	   	 ncity = config_getcnt("scheme.so", "city");
		   month = rrand(12) + 1;
       if (month == 2) {
         day = 28;
       } else if (month % 2) {
         day = 31;
       } else {
        day = 30;
       }
       day = rrand(day) + 1;
		   irc_privmsg(to_sender(in), "#scheme-treffen ist am %d.%d. in %s.",
			   day, month, config_getn("scheme.so", "city", rrand(ncity)));
	   }
   }
   return 0;
}

int init() {
   char *buf;
   last = 0;

   buf = config_getn("scheme.so", "interval", 0);
   interval = buf? atoi(buf): 10;
   return 0;
}

PLUGIN_DEF(init, NULL, reply, NULL, NULL, PLUGIN_NODEP, PLUGIN_NOHELP)

