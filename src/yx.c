/* 
 * yx.c -- Emulate yx.
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "irc.h"
#include "util.h"
#include "conf.h"
#include "rrand.h"

#ifndef length
#define length(x) (sizeof(x)/sizeof(x[0]))
#endif

enum yx_state {
  YX_IRC = 0,
  YX_S3,
  YX_AMF,
  YX_KDF
};

char *yx_state_str[] = {
  "IRC",
  "S3",
  "AMF",
  "KDF"
};

char *chan = "#scheme";

enum yx_state cur_state;
unsigned long change_state;

void timer(unsigned long ts) {
  struct tm now;
  char *msg[] = {
    "ati ist toll",
    "kb mehr auf nv giev ati 5770",
    "卐 ATI 卐",
    "ATI ATI ATI",
    "wann gehen wir hodi besuchen? :D",
    ":E",
    "aber ich werd mir cata anschaun",
    "cataclysm is so die verarsche einfach",
    "oh yeah ati catacylst 10.3 PREVIEW EDITION",
    "bierzeit",
    "ich brauch noch en kasten bier",
    "gestern mal ausprobiert ;F",
    "smknight: du judenkönig",
    ":Γ",
    "ihr $tourettes",
    "texmex pizza mit thunfisch und gyros... und joghurt",
    "bk enabled, brb",
    "train hard, go pro",
    ".choose 1 zwickel spezial",
  };

  if (ts > change_state) {
    (void)localtime_r((time_t*)&ts, &now);
    switch(cur_state) {
      case YX_IRC:
        if (now.tm_hour < 14) {
          cur_state = YX_S3;
          irc_privmsg(chan, "sor gn8bier und s3");
          now.tm_min += rrand(23);
          now.tm_hour = 18 + now.tm_min / 60;
          now.tm_min %= 60;
        } else {
          if (rrand(5) == 1) {
            cur_state = YX_KDF;
            irc_privmsg(chan, "ersma was kraft durch freude");
          } else {
            cur_state = YX_AMF;
            irc_privmsg(chan, "bald arbeit macht frei modus :<");
          }
          now.tm_min = rrand(20);
          now.tm_hour = 6;
        } 
        break;
      case YX_S3:
        cur_state = YX_IRC;
        irc_privmsg(chan, "hulla");
        now.tm_min = rrand(10);
        now.tm_hour = 20;
        break;
      case YX_AMF:
      case YX_KDF:
        cur_state = YX_IRC;
        now.tm_min += rrand(60);
        now.tm_hour = 11 + now.tm_min / 60;
        now.tm_min %= 60;
        break;
    }

    change_state = mktime(&now);
    if (change_state <= ts)
      change_state += 24*60*60;

    fprintf(stderr, "state changed: %d\n",cur_state);
  } else {
    if ((cur_state == YX_IRC || cur_state == YX_KDF)
	&& rrand(50000) == 1337)
      irc_privmsg(chan, "%s", msg[rrand(length(msg))]);
  }
}

int reply(info_t * in) {
  char *msg[] = {
    "kerker",
    "was? :<",
    "stell mal bier kalt",
    "bierli reingefüllt?",
    "dank UBANTO lunix",
    ";S",
    "ofenfrische hawaii mit 100g bonus mozzarella kommt urgut",
    "danke",
  };

  if (in->cmd == cmd_numeric && in->numeric == 1) {
    irc_send("JOIN %s", chan);
  } else if (in->cmd == cmd_join
    && !strcasecmp(chan, (in->argv[0])?in->argv[0]:in->tail)) {
    if ((cur_state == YX_IRC || cur_state == YX_KDF) 
        && strcasecmp(in->me, in->sender_nick)) {
      irc_privmsg(chan, "%s", in->sender_nick);
      irc_privmsg(chan, "hallu");
    }
  } else if (in->cmd == cmd_privmsg) {
    in->tail = skip_nick(in->tail, in->me);
    if (!tail_cmd(&in->tail, "yx")) {
      irc_privmsg(to_sender(in), "State %s will change in %d minutes",
        yx_state_str[cur_state], (change_state - time(NULL)) / 60);
    } else if ((cur_state == YX_IRC || cur_state == YX_KDF)
        && is_channel(in->argv[0])
        && !strcasecmp(in->argv[0], chan)) {
      if (rrand(80) == 23) {
        irc_privmsg(chan, "%s: %s", in->sender_nick, msg[rrand(length(msg))]);
      }
    }
  }
  return 0;
}

int init(void) {
  unsigned long t;
  struct tm now;

  time((time_t*)&t);
  localtime_r((time_t*)&t, &now);
  
  if (now.tm_hour >= 20 || now.tm_hour < 6) {
    cur_state = YX_AMF;
    now.tm_hour = 6;
    now.tm_min = rrand(20);
  } else if (now.tm_hour >= 11 && now.tm_hour < 18) {
    cur_state = YX_S3;
    now.tm_min += rrand(23);
    now.tm_hour = 18 + now.tm_min / 60;
    now.tm_min %= 60;
  } else {
    cur_state = YX_IRC;
    if (now.tm_hour < 14) {
      now.tm_hour = 11;
    } else {
      now.tm_hour = 20;
    }
    now.tm_min = rrand(10);
  }

  change_state = mktime(&now);
  if (change_state <= t)
    change_state += 24*60*60;

 return 0;
}

PLUGIN_DEF(init, NULL, reply, timer, NULL, PLUGIN_NODEP, PLUGIN_NOHELP)

