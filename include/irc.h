/* 
 * irc.h -- irc.c header
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

#ifndef IRC_H_
#define IRC_H_

#include "defs.h"
#include "sock.h"

int irc_canwrite(void *ptr);
int irc_canread(void *ptr);
int irc_sendbuf_now(const char *buf, int len);
int irc_sendbuf(const char *buf, int len);
int irc_send(const char *fmt, ...);

/* added by sird 15.08.11 */
int irc_kick(const char *channel, const char *target, const char *msg, ...);
int irc_mode(const char *target, const char *mode);
/* end added by sird */

int irc_privmsg(const char *target, const char *msg, ...);
int irc_notice(const char *target, const char *msg, ...);
int irc_query(const char *module, ...);
int irc_reg(void);
int irc_makeinfo(char *in, info_t * out);
void irc_discon(void);
int irc_conn(void);
void irc_timer(unsigned long ts);
void irc_free(void);
void irc_cb_err(void *ptr);
void irc_cb_data(void *ptr);
int irc_init(void);
#endif

