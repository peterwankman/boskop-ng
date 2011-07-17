/* 
 * help.c -- Provide help
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

#include "config.h"

#include <stdlib.h>
#include <string.h>

#include "auth.h"
#include "irc.h"
#include "conf.h"
#include "util.h"
#include "plugins.h"

int reply(info_t * in) {
  char *mname;
  plugin_t *pl;
  char buf[4096];
  int pos, len, i;

  if (in->cmd == cmd_privmsg) {
    in->tail = skip_nick(in->tail, in->me);
    if(!tail_cmd(&in->tail, "help")) {
      mname = tail_getp(&in->tail);
      if (mname) {
        pl = plugins_find(mname);
        if (pl && pl->help && pl->help[0]) {
          for (i = 0; pl->help[i]; ++i) {
            irc_privmsg(to_sender(in), "%s: %s", pl->name, pl->help[i]);
          }
        } else {
          irc_privmsg(to_sender(in), "No such plugin. Try !help to get a plugin list");
        }
      } else {
        pos = 0;
        buf[0] = '\0';
        list_for_each_entry(pl, &plugins, elm) {
          if (!pl->help || !pl->help[0])
            continue;
          len = strlen(pl->name);
          if (len <= sizeof(buf) - 2 - pos) {
            buf[pos] = ' ';
            strcpy(&buf[pos+1], pl->name);
            pos += len+1;
          } else {
            break;
          }
        }
        irc_privmsg(to_sender(in), "Use %shelp <module> to view help information. The following modules are available:%s", getprefix(), buf);
      }
    }
  }
  return 0;
}

PLUGIN_DEF(NULL, NULL, reply, NULL, NULL, PLUGIN_NODEP, PLUGIN_HELP("!help <module> - Display help for a certain module. Omit <module> to get a list of all modules"))

