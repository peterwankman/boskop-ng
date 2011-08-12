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


enum yx_state cur_state;
unsigned long change_state;

int isyxchannel(const char *chan)
{
	int i, cnt;

	cnt = config_getcnt("yx.so", "channel");

	for (i = 0; i < cnt; ++i)
		if (!strcasecmp(config_getn("yx.so", "channel", i), chan))
			return 1;
	return 0;
}

void yx_amsg(const char *msg)
{
	int i, cnt;

	cnt = config_getcnt("yx.so", "channel");
	for (i = 0; i < cnt; ++i)
		irc_privmsg(config_getn("yx.so", "channel", i),
			"%s", msg);
}

void timer(unsigned long ts) {
  struct tm now;
  int n, t, l, ccnt;

  if (ts > change_state) {
    (void)localtime_r((time_t*)&ts, &now);
    switch(cur_state) {
      case YX_IRC:
        if (now.tm_hour < 14) {
          cur_state = YX_S3;
          yx_amsg("sor gn8bier und s3");
          now.tm_min += rrand(23);
          now.tm_hour = 18 + now.tm_min / 60;
          now.tm_min %= 60;
        } else {
          if (rrand(7) < 5) {
            cur_state = YX_KDF;
            yx_amsg("ersma was kraft durch freude");
          } else {
            cur_state = YX_AMF;
            yx_amsg("bald arbeit macht frei modus :<");
          }
          now.tm_min = rrand(20);
          now.tm_hour = 6;
        } 
        break;
      case YX_S3:
        cur_state = YX_IRC;
        yx_amsg("hulla");
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

  } else {
    ccnt = config_getcnt("yx.so", "channel");
    if ((cur_state == YX_IRC || cur_state == YX_KDF)
	&& ccnt > 0 && rrand(5000/ccnt) == 1)
      switch(rrand(3)) {
        case 0:
          n = config_getcnt("yx.so", "talk");
          if (n > 0)
            irc_privmsg(config_getn("yx.so", "channel", rrand(ccnt)), "%s",
		config_getn("yx.so", "talk", rrand(n)));
          break;
        case 1:
          n = config_getcnt("yx.so", "recommend");
          if (n > 0)
            irc_privmsg(config_getn("yx.so", "channel", rrand(ccnt)),
            	"Dr. Keke recommends %s", 
		config_getn("yx.so", "recommend", rrand(n)));
          break;
        case 2:
          n = config_getcnt("yx.so", "pizza");
          t = config_getcnt("yx.so", "topping");
          l = config_getcnt("yx.so", "like");
          if (n > 0 && t > 0 && l > 0)
	    irc_privmsg(config_getn("yx.so", "channel", rrand(ccnt)), 
          	"%s mit %s, %s und %s %s",
		config_getn("yx.so", "pizza", rrand(n)),
		config_getn("yx.so", "topping", rrand(t)),
		config_getn("yx.so", "topping", rrand(t)),
		config_getn("yx.so", "topping", rrand(t)),
		config_getn("yx.so", "like", rrand(l)));
          break;
      }
  }
}

int reply(info_t * in) {
  int n;
  int ccnt;
  if (in->cmd == cmd_numeric && in->numeric == 1) {
    ccnt = config_getcnt("yx.so", "channel");
    for (n = 0; n < ccnt; ++n)
    	irc_send("JOIN %s", config_getn("yx.so", "channel", n));
  } else if (in->cmd == cmd_join
    && isyxchannel(in->argv[0]?in->argv[0]:in->tail)) {
    if ((cur_state == YX_IRC || cur_state == YX_KDF) 
        && strcasecmp(in->me, in->sender_nick)) {
      irc_privmsg(in->argv[0]?in->argv[0]:in->tail, "%s", in->sender_nick);
      irc_privmsg(in->argv[0]?in->argv[0]:in->tail, "hallu");
    }
  } else if (in->cmd == cmd_privmsg) {
    in->tail = skip_nick(in->tail, in->me);
    if (!tail_cmd(&in->tail, "yx")) {
      irc_privmsg(to_sender(in), "State %s will change in %d minutes",
        yx_state_str[cur_state], (change_state - time(NULL)) / 60);
    } else if ((cur_state == YX_IRC || cur_state == YX_KDF)
        && is_channel(in->argv[0])
        && isyxchannel(in->argv[0])) {
      n = config_getcnt("yx.so", "reply");
      if (n > 0 && rrand(80) == 23) {
        irc_privmsg(in->argv[0], "%s: %s", in->sender_nick, 
			config_getn("yx.so", "reply", rrand(n)));
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
    cur_state = YX_KDF;
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

