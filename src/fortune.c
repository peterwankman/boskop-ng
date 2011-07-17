/* 
 * fortune.c -- Send random fortune cookies
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
#include <limits.h>
#include <errno.h>

#include "irc.h"
#include "util.h"
#include "conf.h"
#include "rrand.h"

#define MIN_DELAY_DEF   (60 * 5)
#define MAX_DELAY_DEF   (60 * 15)
#define REQ_DELAY_DEF	60

unsigned long nexttalk, min_delay, max_delay, req_delay, lasttime;

char *randomline(char *buf, int maxlen) {
  FILE *f;
  int lines, i;

  f = fopen("fortune.txt", "r");
  if (!f)
    return NULL;
        
  for (lines = 0;;) {
    if (!fgets(buf, maxlen, f))
      break;
    ++lines;
  }

  if (!lines)
    return NULL;

  fseek(f, 0, SEEK_SET);
  lines = rrand(lines);
  for (i = 0; i < lines; i++) {
    if (!fgets(buf, maxlen, f))
      break;
  }
  return buf;
}

void timer(unsigned long ts) {
  int n;
  char *ch, buf[512];

  if (nexttalk <= ts) {
    n = config_getcnt("fortune.so", "channels");
    if (n > 0 && randomline(buf, sizeof(buf)) != NULL) {
      ch = config_getn("fortune.so", "channels", rrand(n));
      irc_privmsg(ch, "%s", buf);
    }
    nexttalk = ts + min_delay + rrand(max_delay - min_delay);
  }
}

int reply(info_t * in) {
  int remain;
  char buf[512];

  if (in->cmd == cmd_privmsg) {
    in->tail = skip_nick(in->tail, in->me);
    if(!tail_cmd(&in->tail, "fortune")
        && randomline(buf, sizeof(buf)) != NULL) {
      if(time(NULL) - lasttime >= req_delay) {
        irc_privmsg(to_sender(in), "%s", buf); 
        lasttime = time(NULL);
      } else {
        remain = req_delay - (time(NULL) - lasttime);
        if(remain > 3600)
          irc_notice(in->sender_nick, "Please wait %d hours.", remain / 3600);
        else if(remain > 60)
          irc_notice(in->sender_nick, "Please wait %d minutes.", remain / 60);
        else
          irc_notice(in->sender_nick, "Please wait %d seconds.", remain);
      }
    }
  }
  return 0;
}

int init(void) {
  char *p;

  p = config_get("fortune.so", "mindelay");
  if (p) {
    min_delay = strtoul(p, NULL, 10);
    if (min_delay == ULONG_MAX && errno == ERANGE) {
      min_delay = MIN_DELAY_DEF;
      printc("Illegal minimum delay. Use default %lu\n",
        MIN_DELAY_DEF);
    }
  } else {
    min_delay = MIN_DELAY_DEF;
    printc("No minimum delay. Use default %lu\n", MIN_DELAY_DEF);
  }

  p = config_get("fortune.so", "maxdelay");
  if (p) {
    max_delay = strtoul(p, NULL, 10);
    if (max_delay == ULONG_MAX && errno == ERANGE) {
      max_delay = MAX_DELAY_DEF;
      printc("Illegal maximum delay. Use default %lu\n",
        MAX_DELAY_DEF);
    }
  } else {
    max_delay = MAX_DELAY_DEF;
    printc("No maximum delay. Use default %lu\n", MAX_DELAY_DEF);
  }
 
  if (max_delay < min_delay)
    max_delay = min_delay;

  nexttalk = time(NULL) + min_delay + rrand(max_delay - min_delay);

  p = config_get("fortune.so", "reqdelay");
  if (p) {
    req_delay = strtoul (p, NULL, 10);
    if (req_delay == ULONG_MAX && errno == ERANGE) {
      req_delay = REQ_DELAY_DEF;
      printc("Illegal request delay. Use default %lu\n",
        REQ_DELAY_DEF);
    }
  } else {
    req_delay = REQ_DELAY_DEF;
    printc("No request delay. Use default %lu\n", REQ_DELAY_DEF);
  }
  lasttime = 0;

  return 0;
}

PLUGIN_DEF(init, NULL, reply, timer, NULL, PLUGIN_NODEP, PLUGIN_HELP("!fortune - behold the eternal wisdom"))

