/* 
 * pluin.h -- header for auth
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

#ifndef AUTH_H_
#define AUTH_H_

#include "defs.h"

#define UL_MASTER 3
#define UL_OP     2
#define UL_NOBODY 1
#define UL_NOAUTH 0

#define AUTHCMD_GET   1
#define CHECKAUTH(sender, level)   (irc_query("auth", AUTHCMD_GET, sender) >= level)

typedef struct userinfo userinfo_t;
struct userinfo {
   char name[NICKLEN+1], hash[HASHLEN+1];
   int level;
   char ident[SENDERLEN+1];
   struct userinfo *next;
};

#endif

