/* 
* decide.c -- Solves critical uncertainties.
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "irc.h"
#include "auth.h"
#include "util.h"
#include "conf.h"
#include "rrand.h"

#define DECIDE_MAX 49
 
int reply(info_t * in) {
   char line[BUFLEN], *alt[DECIDE_MAX], *p;
   int m, n, i, j, k, len;
   
   if (in->cmd == cmd_privmsg && 
         (!tail_cmd(&in->tail,"choose") || !tail_cmd(&in->tail,"decide"))) {
      p = config_get("decide.so", "maximum");
      if(!p) {
         m = 5;
      } else {
	 m = MIN(atoi(p), DECIDE_MAX);
      }

      p = tail_getp(&in->tail);
      if(!p || (n = atoi(p)) <= 0) {
         irc_privmsg(to_sender(in), "Usage: %schoose <num> <...>", getprefix());
	 return 0;
      }
		
      if (n > m) {
         irc_privmsg(to_sender(in), "Maximum number exceeded.");
	 return 0;
      }
      
      for (i = 0; i < DECIDE_MAX; ++i) {
         if (!(alt[i] = tail_getp(&in->tail)))
            break;
      }
      
      if(i < n) {
         irc_privmsg(to_sender(in), "Can't choose %i out of %i.", n, i);
	 return 0;
      }

      line[0] = '\0';
      len = sizeof(line)-1;
      p = line;
      for(j = 0; j < n; j++) {
         k = rrand(i);
         m = strlen(alt[k]);
         if (m >= len) {
            irc_privmsg(to_sender(in), "Arguments too long");
            return 0;
         }
         strcpy(p, alt[k]);
         p += m;
         len -= m+1;
         *(p++) = ' ';
         --i;
         for (m = k; m < i; ++m)
            alt[m] = alt[m+1];
      }
      if (p > line) {
         *(p-1) = '\0';
	 irc_privmsg(to_sender(in), "%s: %s", in->sender_nick, line);
      }
   }
   return 0;
}


PLUGIN_DEF(NULL, NULL, reply, NULL, NULL, PLUGIN_NODEP, PLUGIN_HELP("!decide <num> <alternative 1> ..."))

