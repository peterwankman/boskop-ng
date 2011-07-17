/* 
 * join.h -- join.c header
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

#ifndef JOIN_H_
#define JOIN_H_

#define UFL_OP        0x04
#define UFL_HOP       0x02
#define UFL_VOICE     0x01
#define UFL_NONE      0x00

#define CFL_MOD       0x20
#define CFL_NOEXMSG   0x10
#define CFL_TOPICOP   0x08
#define CFL_INVITE    0x04
#define CFL_SECRET    0x02
#define CFL_PRIVATE   0x01

#define JCMD_CFL     1
#define JCMD_UFL     2
#define JCMD_LIMIT   3
#define JCMD_KEY     4
#define JCMD_CCNT    5
#define JCMD_UCNT    6
#define JCMD_CNAME   7
#define JCMD_UNAME   8

#define GET_CHANLVL(c)      irc_query("join", JCMD_CFL, c)
#define GET_USERLVL(c, u)   irc_query("join", JCMD_UFL, c, u)
#define GET_USERCNT(c)      irc_query("join", JCMD_UCNT, c)
#define GET_USER(c, n, pp)  irc_query("join", JCMD_UNAME, c, n, pp)

#endif

