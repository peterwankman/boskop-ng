/* 
 * plugin.c -- Administrate plugins
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

#include "irc.h"
#include "auth.h"
#include "util.h"
#include "plugins.h"

int reply(info_t * in) {
   plugin_t *pl;
   char *p;

   if (in->cmd == cmd_privmsg && CHECKAUTH(in->sender, UL_MASTER)) {
      in->tail = skip_nick(in->tail, in->me);
      if (!tail_cmd(&in->tail, "pload")) {
         p = tail_getp(&in->tail);
         if (!p) {
            irc_notice(in->sender_nick, "Usage: %spload <module>", getprefix());
         } else {
            if(plugins_open(p) < 0) {
               irc_notice(in->sender_nick, "Error loading %s, see log for details", p);
            } else {
               irc_notice(in->sender_nick, "Success");
            }
         }
      } else if (!tail_cmd(&in->tail, "punload")) {
         p = tail_getp(&in->tail);
         if (!p) {
            irc_notice(in->sender_nick, "Usage: %spunload <module>", getprefix());
         } else {
            if(plugins_close(p) < 0) {
               irc_notice(in->sender_nick, "Error unloading %s, see log for details", p);
            } else {
               irc_notice(in->sender_nick, "Success");
            }
         } 
      } else if (!tail_cmd(&in->tail, "preload")) {
         p = tail_getp(&in->tail);
         if (!p) {
            irc_notice(in->sender_nick, "Usage: %spreload <module>", getprefix());
         } else {
            if(plugins_reload(p) < 0) {
               irc_notice(in->sender_nick, "Error reloading %s, see log for details", p);
            } else {
               irc_notice(in->sender_nick, "Success");
            }
         }
      } else if (!tail_cmd(&in->tail, "plist")) {
         list_for_each_entry(pl, &plugins, elm) {
            irc_notice(in->sender_nick, "%s", pl->name);
         }
         irc_notice(in->sender_nick, "End of plugin list");
      }
   } 
   return 0;
}

PLUGIN_DEF(NULL, NULL, reply, NULL, NULL, PLUGIN_DEP("auth.so"), PLUGIN_NOHELP)

