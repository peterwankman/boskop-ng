/* 
 * quote.c -- Maintain a database of quotations.
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "auth.h"
#include "irc.h"
#include "util.h"
#include "conf.h"
#include "rrand.h"

static char *dbfile;
static int interval;
static time_t last;

int reply(info_t * in) {
   char buf[BUFLEN+1];
   FILE *quote;
   int i, lines, found = 0;
   char *p;

   if (in->cmd == cmd_privmsg) {
      if (!tail_cmd(&in->tail, "addquote")) {
         if (!CHECKAUTH(in->sender, UL_OP)) {
            irc_notice(in->sender_nick, "Permission denied.");
         } else if (dbfile && (quote = fopen(dbfile, "a"))) {
            fprintf(quote, "%s\n", in->tail);
            fclose(quote); 
            irc_privmsg(to_sender(in), "Added.");
         } else {
            irc_notice(in->sender_nick, "Error.");
         }
      } else if (!tail_cmd(&in->tail, "quote")) {
         if (!CHECKAUTH(in->sender, UL_OP) 
               && last + interval > time(NULL)) {
            irc_notice(in->sender_nick, "Please wait a moment.");
         } else if (dbfile && (quote = fopen(dbfile, "r"))) {
            last = time(NULL);
            lines = 0;
            for (;;) {
               if (!fgets(buf, sizeof(buf), quote))
                  break;
               ++lines;
            }

            fseek(quote, 0, SEEK_SET);
            p = tail_getp(&in->tail);
            if(!p)
               lines = rrand(lines);
               
            for (i = 0; i < lines; i++) {
               if (!fgets(buf, sizeof(buf), quote)) 
                  break;
               if(p && !regex(buf, p)) {
                     found = 1;
                     break;
                  }
            }
               
            if (lines && (found || (!p && (lines == i)))) {
               buf[strlen(buf) - 1] = '\0';
               irc_privmsg(to_sender(in), "Quote %d: %s", i + 1, buf);
            } else {
               irc_notice(in->sender_nick, "Not found.");   
            }
            fclose(quote);
         } 
      }
   }
   return 0;
}

int init(void) {
   char *buf;
   last = 0;
   buf = config_getn("quote.so", "interval", 0);
   interval = buf? atoi(buf): 10;
   
   dbfile = config_get("quote.so", "dbfile");
   if (!dbfile || !strlen(dbfile)) {
      warn("No quote db given.\n");
      return -1;
   }
   printc("Quote database file is '%s'\n", dbfile);

   return 0;
}

PLUGIN_DEF(init, NULL, reply, NULL, NULL, PLUGIN_NODEP, PLUGIN_NOHELP)

