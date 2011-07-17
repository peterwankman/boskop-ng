/* 
 * sercz.c -- Emulate sercz
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

#define WORD_CNT  7

typedef struct {
   time_t last;
   int interval;
   int max;
   int nword[WORD_CNT];
   char **word[WORD_CNT];
} talk_t;

static talk_t talkinfo;
static char *word[] = { "init", "pain", "time", "what", "sport", "organ", "accid" };

void destroy();

int reply(info_t * in) {
   time_t now = time(NULL);
   int i, nword[WORD_CNT];
   
   if (talkinfo.last + talkinfo.interval > now)
      return 0;

   if (in->cmd == cmd_privmsg) {
	   if (!tail_cmd(&in->tail, "sercz")) {
		   for(i = 0; i < WORD_CNT; i++) {
			   nword[i] = rrand(talkinfo.nword[i]);
		   }
		   irc_privmsg(to_sender(in), "%s %s",
   			   talkinfo.word[0][nword[0]], talkinfo.word[1][nword[1]]);
		   irc_privmsg(to_sender(in), "%s %d %s %s",
			   talkinfo.word[2][nword[2]], rrand(talkinfo.max - 2) + 2,
			   talkinfo.word[3][nword[3]], talkinfo.word[4][nword[4]]);
		   irc_privmsg(to_sender(in), "und dabei mein%s %s",
			   talkinfo.word[5][nword[5]], talkinfo.word[6][nword[6]]);				
	   }
   }
   return 0;
}

int init() {
   int i, j;
   char *buf;

   talkinfo.last = time(NULL);

   for(i = 0; i < WORD_CNT; i++) {
	   talkinfo.nword[i] = config_getcnt("sercz.so", word[i]);
	   if (!talkinfo.nword[i]) {
		   talkinfo.word[i] = NULL;
         return -1;
      }

	   
	   if(!(talkinfo.word[i] = malloc(talkinfo.nword[i] * sizeof(char*)))) {
         destroy();  
         return -1;
      }
      
	   for(j = 0; j < talkinfo.nword[i]; j++) {
		   talkinfo.word[i][j] = config_getn("sercz.so", word[i], j);
	   }
   }

   buf = config_getn("sercz.so", "interval", 0);
   talkinfo.interval = buf? atoi(buf): 10;

   buf = config_getn("sercz.so", "max", 0);
   talkinfo.max = buf? atoi(buf): 50;

   if(talkinfo.max < 3)
      talkinfo.max = 3;
   return 0;
}

void destroy()
{
	int i;

	for(i = 0; i < WORD_CNT && talkinfo.word[i]; i++)
      free(talkinfo.word[i]);
}

PLUGIN_DEF(init, destroy, reply, NULL, NULL, PLUGIN_NODEP, PLUGIN_NOHELP)

