/* 
 * horde.c -- The horde.
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

#include "irc.h"
#include "join.h"
#include "util.h"
#include "conf.h"
#include "horde.h"
#include "rrand.h"

char hchannel[CHANLEN+1];
char hkey[PASSLEN+1];

int query(info_t * in, va_list ap)
{
   int cmd, n, i, c;
   char *p, **l;
   cmd = va_arg(ap, int);
   
   switch(cmd) {
      case HORDE_ALL:
         p = va_arg(ap, char*);
         irc_privmsg(hchannel, "!horde %s", p);
         irc_send("%s", p);
         return 1;
         break;
      case HORDE_ONE:
         n = GET_USERCNT(hchannel);
         if (GET_USER(hchannel, rrand(n), &p) > 0) {
            irc_privmsg(p, "!horde %s", va_arg(ap, char*));
            return 1;
         }
         break;
      case HORDE_BATCH:
         n = GET_USERCNT(hchannel);
         l = va_arg(ap, char**);
         c = rand();
         for (i  = 0; l[i]; ++i) {
            if (GET_USER(hchannel, (++c) % n, &p) > 0) {
               irc_privmsg(p, "!horde %s", l[i]);
            }
         }
         return i;
         break;
   }
   return -1;
}

int reply(info_t * in) {
   if (in->cmd == cmd_numeric && in->numeric == 1) {
         irc_send("JOIN %s %s", hchannel, hkey);
   } else if (in->cmd == cmd_numeric && in->numeric == 366 && !strcasecmp(in->argv[1], hchannel)) {
      if (GET_USERCNT(hchannel) <= 1)
         irc_send("MODE %s +sntk %s", hchannel, hkey);
   } else if (in->cmd == cmd_privmsg && !tail_cmp(&in->tail, "!horde")) {
      if (in->tail && GET_USERLVL(hchannel, in->sender_nick) >= 0)
         irc_send("%s", in->tail);
   }
   return 0;
}

int init(void)
{
   char *c, *k;
   c = config_get("horde.so", "channel");
   k = config_get("horde.so", "key");

   if (!c || !k || !c[0] || !k[0])
      return -1;

   strnzcpy(hchannel, c, CHANLEN+1);
   strnzcpy(hkey, k, PASSLEN+1);
   return 0;
}

PLUGIN_DEF(init, NULL, reply, NULL, query, PLUGIN_DEP("join.so", "nick.so"), PLUGIN_NOHELP)

