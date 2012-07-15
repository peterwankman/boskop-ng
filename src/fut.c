/* 
 * xyzzy.c -- Magic.
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

#include "irc.h"
#include "util.h"

int reply(info_t * in) {
	char *p;
	int i;

	if (in->cmd == cmd_privmsg && !tail_cmd(&in->tail,"fut")) {
		i = config_getcnt("fut.so", "insult");
		p = tail_getp(&in->tail);
		if(p) {
			irc_privmsg(to_sender(in), "%s: %s", p, 
				i?config_getn("fut.so", "insult", rrand(i)):"Fut");
		} else {
			irc_privmsg(to_sender(in), "%s",
				i?config_getn("fut.so", "insult", rrand(i)):"Fut");
		}
	}

	return 0;
}

PLUGIN_DEF(NULL, NULL, reply, NULL, NULL, PLUGIN_NODEP, PLUGIN_HELP("Tell the people what you think of them."))

