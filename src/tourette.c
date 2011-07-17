/* 
 * tourette.c -- HITLERCUNT!
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

#include <stdlib.h>
#include <string.h>

#include "irc.h"
#include "conf.h"
#include "util.h"
#include "rrand.h"

int query(info_t * in, va_list ap)
{
   char *buf = va_arg(ap, char*);
   int nwords, ncomb, i, bz = va_arg(ap, int), len = 0;

   nwords = config_getcnt("tourette.so", "word");
   if (!nwords)
      return -1;
   ncomb = rrand(3) + 2;
   for(i = 0; i < ncomb; i++) {
      if (snprintf(buf+len, bz-len, "%s", config_getn("tourette.so", "word", rrand(nwords))) >= bz-len)
         return -1;
      len = strlen(buf);
   }
   return 0;
}


#define TPAT	"$tourette"

int reply(info_t * in) {
   char buf[512], tmp[512], *rp, *p, *wp = buf, *end = buf + sizeof(buf) -1;
   int found = 0, len;


   if (in->cmd == cmd_privmsg) {
      rp = skip_nick(in->tail, in->me);
      while ((p = strstr(rp, TPAT))) {
         found = 1;
         len = MIN(p-rp, end-wp);
         memcpy(wp, rp, len);
         wp += len;
         rp = p+strlen(TPAT);
         if(!irc_query("tourette", tmp, sizeof(tmp))) {
            len = MIN(strlen(tmp), end-wp);
            memcpy(wp, tmp, len);
            wp += len;
         }
      }
      
      if (found) {
         len = MIN(strlen(rp), end-wp);
         memcpy(wp, rp, len);
         wp += len;
         *wp = '\0';
         irc_privmsg(to_sender(in), "%s meint: %s", in->sender_nick, buf);
      }
   }
   return 0;
}

PLUGIN_DEF(NULL, NULL, reply, NULL, query, PLUGIN_NODEP, PLUGIN_HELP("Use '$tourette' anywhere in a line for instant hilarity."))

