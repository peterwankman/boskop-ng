/* 
* nickserv.c -- Authenticate with nickserv
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

#include "defs.h"
#include "conf.h"
#include "util.h"
#include "irc.h"

int reply(info_t * in) {
   int i, cnt, nlen;
   char *cred, *hostmask;
   
   if((in->cmd == cmd_notice || in->cmd == cmd_privmsg) && in->sender_nick) {
      hostmask = config_get("nickserv.so", "hostmask");
      if (hostmask && !strcasecmp(in->sender, hostmask)
         && (strstr(in->tail, "registered") || strstr(in->tail, "protected"))){
         nlen = strlen(in->me);
         cnt = config_getcnt("nickserv.so", "credentials");
         for (i = 0; i < cnt; ++i) {
            cred = config_getn("nickserv.so", "credentials", i);
            if (!strncasecmp(cred, in->me, nlen) && cred[nlen] == '/') {
               irc_privmsg(in->sender_nick, "IDENTIFY %s", cred+nlen+1);
               break;
            } 
         }
      }      
   }
   return 0;
}

PLUGIN_DEF(NULL, NULL, reply, NULL, NULL, PLUGIN_NODEP, PLUGIN_NOHELP)

