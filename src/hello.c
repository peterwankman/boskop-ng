/* 
 * hello.c -- A kind greeting
 * 
 * Copyright (C) 2009  Martin Wolters et al.
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

typedef struct {
   time_t last;
   int ntrigger;
   char **trigger;
   int nreply;
   char **reply;
   int interval;
   int odds;
} hello_t;

static hello_t helloinfo;

int reply(info_t * in) {
   time_t now = time(NULL);
   int i, j;
   
   if (helloinfo.last + helloinfo.interval > now)
      return 0;

   if (in->cmd == cmd_privmsg) {
      in->tail = skip_nick(in->tail, in->me);
      for (i = 0; i < helloinfo.ntrigger; i++) {
         if (!regex(in->tail, helloinfo.trigger[i])) {
            if(rrand(helloinfo.odds) > 0)
               return 1;
            irc_privmsg(to_sender(in), "%s",
               helloinfo.reply[rrand(helloinfo.nreply)]);
            helloinfo.last = now;
         }
      }
   }
   return 0;
}

int init() {
   int i;
   char *buf;
   
   helloinfo.last = 0;
   helloinfo.ntrigger = config_getcnt("hello.so", "trigger");
   helloinfo.nreply = config_getcnt("hello.so", "reply");

   if (!helloinfo.ntrigger || !helloinfo.ntrigger)
      return -1;
   
   if((helloinfo.trigger = malloc(helloinfo.ntrigger * sizeof(char*))) == NULL)
      return -1;
   
   if((helloinfo.reply = malloc(helloinfo.nreply * sizeof(char*))) == NULL) {
      free(helloinfo.trigger);
      return -1;
   }
   

   for(i = 0; i < helloinfo.ntrigger; i++) {
      helloinfo.trigger[i] = config_getn("hello.so", "trigger", i);
   }


   for(i = 0; i < helloinfo.nreply; i++) {
      helloinfo.reply[i] = config_getn("hello.so", "reply", i);
   }

   buf = config_getn("hello.so", "interval", 0);
   helloinfo.interval = buf? atoi(buf): 30;

   buf = config_getn("hello.so", "odds", 0);
   helloinfo.odds = buf? atoi(buf): 20;
   return 0;
}

void destroy()
{
   free(helloinfo.trigger);
   free(helloinfo.reply);
}

PLUGIN_DEF(init, destroy, reply, NULL, NULL, PLUGIN_NODEP, PLUGIN_NOHELP);

