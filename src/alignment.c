/* 
 * alignment.c -- Tells your alignment.
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
#include "rrand.h"

char *ethic[] = { "lawful", "neutral", "chaotic", "true" };
char *moral[] = { "good", "neutral", "evil" };

int reply(info_t * in) {
	int e, m;

	if (in->cmd == cmd_privmsg && !tail_cmd(&in->tail,"alignment")) {
		e = rrand(3);
		m = rrand(3);
		if ((e == 1) && (m == 1))
			e = 3;
		irc_privmsg(to_sender(in), "Your alignment is %s %s.", 
			ethic[e], moral[m]);
	}
	return 0;
}

PLUGIN_DEF(NULL, NULL, reply, NULL, NULL, PLUGIN_NODEP, PLUGIN_NOHELP)

