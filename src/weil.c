/* 
 * weil.c -- Gives clever answers.
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
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "irc.h"
#include "util.h"
#include "conf.h"
#include "rrand.h"

typedef struct {
   int delay;
   char msg[512];
   char *dbfile;
   int interval;
   int odds;
   time_t lasttime;
} weilinfo_t;

static weilinfo_t info;

static int check_config(const char *str, const char *conf)
{
   int i, n;
   char *p;

   if (!str)
      return 1;
   n = config_getcnt("weil.so", conf);

   for (i = 0; i < n; ++i) {
      p = config_getn("weil.so", conf, i);
      if (!p)
         continue;
      if (!strcasecmp(p, str))
         return 0;
   }
   return 1;
}

void timer(unsigned long ts) {
   if (info.delay > 0) {
      --info.delay;
   } else {
      irc_send("%s", info.msg);
      PLUGIN_TIMER(NULL);
   }
}

int reply(info_t * in) {
   char buf[512], *p;
   FILE *weil;
   int i, lines;

   if (in->cmd == cmd_privmsg) {
      p = strchr(in->tail, ':');
      if (p && *(++p) == ' ')
         ++p;
      else
         p = NULL;

      if ((p && !strncasecmp(p, "weil ", 5) && (in->tail = p))
            || !strncasecmp(in->tail, "weil ", 5)) {
         if (check_config(in->argv[0], "blacklist")) {
            if (info.dbfile && (weil = fopen(info.dbfile, "a"))) {
               fprintf(weil, "%s\n", in->tail);
               fclose(weil);
            }
         }
      } else {
         in->tail = skip_nick(in->tail, in->me);
         if ((!strncasecmp(in->tail, "warum", 5) || 
               !strncasecmp(in->tail, "wieso", 5)) &&
               check_config(in->argv[0],"shutup")) {
            if (info.lasttime + info.interval > time(NULL)) {
               return 1;
            } 
            if (rrand(info.odds) > 0)
               return 1;

            if (info.dbfile && (weil = fopen(info.dbfile, "r"))) {
               info.lasttime = time(NULL);
               lines = 0;
               for (;;) {
                  if (!fgets(buf, sizeof(buf), weil))
                     break;
                  ++lines;
               }
               fseek(weil, 0, SEEK_SET);
               lines = rrand(lines);
               for (i = 0; i < lines; i++)
                  if (!fgets(buf, sizeof(buf), weil))
                     break;
               if (i == lines) {
                  buf[strlen(buf) - 1] = '\0';
                  (void)snprintf(info.msg, sizeof(info.msg), "PRIVMSG %s :%s",
                     to_sender(in), buf);
//                info.delay = rrand(5) + 1;
//                PLUGIN_TIMER(timer);
                  irc_send("%s", info.msg);
               }
               fclose(weil);
               return 1;
            }
         } 
      }
   }
   return 0;
}

int init(void) {
   char *buf;
   
   info.dbfile = config_get("weil.so", "dbfile");
   if (!info.dbfile || !strlen(info.dbfile)) {
      warn("No weil db given.\n");
      return -1;
   }
   printc("Weil database file is '%s'\n", info.dbfile);

   info.lasttime = 0;
   info.delay = 0;

   buf = config_getn("weil.so", "interval", 0);
   info.interval = buf? atoi(buf):90;

   buf = config_getn("weil.so", "odds", 0);
   info.odds = buf? atoi(buf): 20;

   return 0;
}

PLUGIN_DEF(init, NULL, reply, NULL, NULL, PLUGIN_NODEP, PLUGIN_NOHELP)

