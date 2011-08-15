/* 
* qauth.c -- Authenticate with quakenet's channel service
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

#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include "conf.h"
#include "util.h"
#include "irc.h"
#include "sha1.h"
#include "hmac.h"

#define QAUTH_SIMPLE	1
#define QAUTH_CHALLENGE	2

typedef struct {
	char *user;
	char *pass;
	char *challenge;
	int mode;
	int hide;
	int authed;
} qauth_t;

qauth_t qauth;

int query(info_t * in, va_list ap) {
	int cmd;

	cmd = va_arg(ap, int);
	switch (cmd) {
		case 1:
			if (qauth.authed)
				return 1;
			else
				return 0;
			break;
		default:
			break;
	}
	return -1;
}

int reply(info_t * in) {
	const char qto[] = "Q@CServe.quakenet.org";
	const char qfrom[] = "Q!TheQBot@CServe.quakenet.org";
	char buf[1024];
	hash_t hash;

	if(in->cmd == cmd_numeric && in->numeric == 1) {
		qauth.authed = 0;
		if(qauth.mode == QAUTH_SIMPLE)
			irc_privmsg(qto, "AUTH %s %s", qauth.user, qauth.pass);
		else
			irc_privmsg(qto, "CHALLENGE");
	} else if(in->cmd == cmd_notice && !strcasecmp(in->sender, qfrom)) {
		if (!strncmp(in->tail, "You are now logged in", 21)) {
			printc("Q-Authentication successful\n");
			qauth.authed = 1;
			if (qauth.hide)
				irc_send("MODE %s +x", in->argv[0]);
		} else if (!strcmp(in->tail, "Username or password incorrect.")) {
			warn("Q-Authentication failed\n");
		} else if (!strncasecmp(tail_getp(&in->tail), "challenge", 9)) {
			qauth.challenge = tail_getp(&in->tail);
			printc("Received challenge: %s\n", qauth.challenge);
			if(strlen(qauth.pass) > 10)
				qauth.pass[10] = 0;
			hash = sha1(qauth.pass, strlen(qauth.pass));
			free(hash.string);
			sprintf(buf, "%s:%08x%08x%08x%08x%08x", qauth.user,
				hash.h0, hash.h1, hash.h2, hash.h3, hash.h4);
			printc("password_hash: %s\n", buf);
			hash = sha1(buf, strlen(buf));
			free(hash.string);
			sprintf(buf, "%08x%08x%08x%08x%08x\n", 
				hash.h0, hash.h1, hash.h2, hash.h3, hash.h4);
			printc("key: %s\n", buf);
			hash = hmac_sha1(buf, 40, qauth.challenge, 32);
			sprintf(buf, "%08x%08x%08x%08x%08x",
                hash.h0, hash.h1, hash.h2, hash.h3, hash.h4);
			free(hash.string);
			irc_privmsg(qto, "CHALLENGEAUTH %s %s HMAC-SHA-1", 
				qauth.user, buf);
		}
		
	}
	return 0;
}

inline void ctolower(char *in) { while((*(in++) = tolower(*in))); }

int init() {
	char *mode;

	qauth.user = config_get("qauth.so", "user");
	qauth.pass = config_get("qauth.so", "pass");
	mode = config_get("qauth.so", "mode");
	
	qauth.mode = strncasecmp(mode, "challenge", 9)? 
		QAUTH_SIMPLE: QAUTH_CHALLENGE;
	free(mode);

	if(!qauth.user) {
		warn("qauth.so: \"user\" not set.\n");
		if(qauth.pass)
			free(qauth.pass);
		return 0;		
	}

	if(!qauth.pass) {
		warn("qauth.so: \"pass\" not set.\n");
		if(qauth.user)
			free(qauth.user);
		return 0;		
	}

	ctolower(qauth.user);
	
	qauth.hide = strncasecmp(config_get("qauth.so", "hide"), "y", 1)?0:1;

	return 0;
}

PLUGIN_DEF(init, NULL, reply, NULL, query, PLUGIN_NODEP, PLUGIN_NOHELP)

