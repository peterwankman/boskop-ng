/* 
 * invite.c -- Autojoin on invite.
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

#include <string.h>

#include "irc.h"
#include "auth.h"
#include "util.h"

int reply(info_t * in) {
   if (in->cmd == cmd_invite)
      irc_send("JOIN %s", in->argv[1]);
   return 0;
}

PLUGIN_DEF( NULL, NULL, reply, NULL, NULL, PLUGIN_NODEP, PLUGIN_NOHELP);

