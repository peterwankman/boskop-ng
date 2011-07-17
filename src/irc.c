/* 
 * irc.c -- IRC functions
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>

#include "defs.h"
#include "plugins.h"
#include "util.h"
#include "conf.h"
#include "io.h"
#include "irc.h"

info_t info;
irc_t irc;
int nextserv = 0;

int irc_sendbuf_now(const char *buf, int len)
{
   if (!irc.bs || !(len > 0))
      return -1;
   (void)gettimeofday(&irc.lastline, NULL);
   return bufsock_put(irc.bs, buf, len);
}

int irc_sendbuf(const char *buf, int len)
{
   struct timeval now;
   ircline_t *il;

   if (!irc.bs || !(len > 0))
      return -1;

#ifndef NDEBUG
   printc("-> ");
   fwrite(buf, 1, len, stdout);
#endif
   (void)gettimeofday(&now, NULL);
   if (list_empty(&irc.linequeue)) {
      (void)gettimeofday(&now, NULL);
      if (timediff(&now, &irc.lastline) > LINEDELAY)
         return irc_sendbuf_now(buf, len);
   }
   il = malloc(sizeof(ircline_t) + len);
   if (!il)
      return -1;
   memcpy(il->buf, buf, len);
   il->len = len;
   list_add_tail(&il->elm, &irc.linequeue); 
   return len;
}

int irc_send(const char *fmt, ...) {
   char buf[BUFLEN+1];
   va_list ap;
   int len;

   va_start(ap, fmt);
   len = vsnprintf(buf, sizeof(buf), fmt, ap);
   va_end(ap);
   if (len > sizeof(buf))
      return -1;
   len += snprintf(&buf[len], sizeof(buf)-len, "\r\n");
   if (len > sizeof(buf))
      return -1;
   return irc_sendbuf(buf, len);
}

int irc_privmsg(const char *target, const char *msg, ...)
{
   char buf[BUFLEN+1];
   va_list ap;
   int len;

   len = snprintf(buf, sizeof(buf), "PRIVMSG %s :", target);
   if (len > sizeof(buf))
      return -1;
   va_start(ap, msg);
   len += vsnprintf(&buf[len], sizeof(buf)-len, msg, ap);
   if (len > sizeof(buf))
      return -1;
   len += snprintf(&buf[len], sizeof(buf)-len, "\r\n");
   if (len > sizeof(buf))
      return -1;
   return irc_sendbuf(buf, len);
}

int irc_notice(const char *target, const char *msg, ...)
{
   char buf[BUFLEN+1];
   va_list ap;
   int len;

   len = snprintf(buf, sizeof(buf), "NOTICE %s :", target);
   if (len > sizeof(buf))
      return -1;
   va_start(ap, msg);
   len += vsnprintf(&buf[len], sizeof(buf)-len, msg, ap);
   if (len > sizeof(buf))
      return -1;
   len += snprintf(&buf[len], sizeof(buf)-len, "\r\n");
   if (len > sizeof(buf))
      return -1;
   return irc_sendbuf(buf, len);
}

int irc_query(const char *module, ...)
{
   va_list ap;
   plugin_t *pl;

   va_start(ap, module);

   if (list_empty(&plugins))
      return -1;

   list_for_each_entry(pl, &plugins, elm) {
      if (!strcmp(module, pl->name) && pl->query)
         return pl->query(&info, ap);
   }
   va_end(ap);
   return -1;
}

int irc_reg(void)
{
	if (irc_send("NICK %s", config_get("core", "nick")) == -1)
		return -1;
	if (irc_send("USER %s 0 0 :%s", config_get("core", "user"), config_get("core", "realname")) == -1)
		return -1;
	return 0;
}

int irc_makeinfo(char *in, info_t * out)
{
	int i;
	char *p, *cmd;

	p = in;
	out->me = irc.me;
	out->argc = 0;
	out->cmd = cmd_unknown;
	out->numeric = 0;
	if (p[0] == ':') {
		out->sender_nick = in + 1;
      if ((p = strchr(p, ' ')))
         *(p++) = '\0';
      strnzcpy(out->sender, out->sender_nick, SENDERLEN+1); 
      if ((out->sender_user = strchr(out->sender_nick, '!'))
         && (out->sender_host = strchr(out->sender_user+1, '@'))) {
         /* User */
         *(out->sender_user++) = '\0';
         *(out->sender_host++) = '\0';
      } else {
         /* Server */
         out->sender_host = out->sender_nick;
         out->sender_nick = NULL;
         out->sender_user = NULL;
      }
	} else {
		out->sender_nick = NULL;
      out->sender_host = NULL;
      out->sender_user = NULL;
	}

	if (!(cmd = p))
		return -1;

	if ((p = strchr(cmd, ' ')))
		(*p++) = '\0';

	if (!strcmp(cmd, "PING")) {
		out->cmd = cmd_ping;
	} else if (!strcmp(cmd, "JOIN")) {
		out->cmd = cmd_join;
	} else if (!strcmp(cmd, "PART")) {
		out->cmd = cmd_part;
	} else if (!strcmp(cmd, "PRIVMSG")) {
		out->cmd = cmd_privmsg;
	} else if (!strcmp(cmd, "NOTICE")) {
		out->cmd = cmd_notice;
	} else if (!strcmp(cmd, "KICK")) {
		out->cmd = cmd_kick;
	} else if (!strcmp(cmd, "MODE")) {
		out->cmd = cmd_mode;
	} else if (!strcmp(cmd, "INVITE")) {
		out->cmd = cmd_invite;
   } else if (!strcmp(cmd, "NICK")) {
      out->cmd = cmd_nick;
   } else if (!strcmp(cmd, "QUIT")) {
      out->cmd = cmd_quit;
	} else if ((i = atoi(cmd))) {
		out->cmd = cmd_numeric;
		out->numeric = i;
	}

   memset(out->argv, 0, INFO_MAXARG*sizeof(char*));
	while (p && out->argc < INFO_MAXARG) {
		if (p[0] == ':') {
			out->tail = p + 1;
			break;
		}

		out->argv[out->argc++] = p;
		if ((p = strchr(p, ' ')))
			*(p++) = '\0';
	}
	return 0;
}

void irc_discon(void)
{
   ircline_t *il;

   while (!list_empty(&irc.linequeue)) {
      il = list_entry(irc.linequeue.next, ircline_t, elm);
      list_del(&il->elm);
      irc_sendbuf_now(il->buf, il->len);
      free(il);
   }
   bufsock_discon(irc.bs);
}

int irc_init(void)
{
   irc.bs = bufsock_new(BUFSIZE, BUFSIZE, irc_cb_err, irc_cb_data, NULL, NULL);
   if (!irc.bs)
      return -1;
   if (io_settimer(irc_timer) == -1) {
      bufsock_free(irc.bs);
      return -1;
   }

   INIT_LIST_HEAD(&irc.linequeue);
   (void)gettimeofday(&irc.lastline, NULL);
 
  return 0;
}

void irc_free(void)
{
   irc_discon();
   bufsock_free(irc.bs);
}

int irc_conn(void)
{
   int p, oldnext, ret;
   char *s;

   irc.me[0] = '\0';
   oldnext = nextserv;

   for(;;) {
      s = config_getn("core", "server", nextserv);
      p = atoi(config_get("core", "port"));
      ret = bufsock_tcp_connect(irc.bs, s, p);
      nextserv = (nextserv + 1) % config_getcnt("core", "server");
      if (ret == -1) {
         printc("Unable to connect to %s:%i.\n", s, p);
	 if (nextserv == oldnext)
	 	return -1;
         continue;
      }
      
      if (irc_reg() != -1)
      	break;
      if (nextserv == oldnext)
      	return -1;
   }
   printc("Connected to %s:%i.\n", s, p);
   (void)gettimeofday(&irc.lastdata, NULL);
   return 0;
}

void irc_timer(unsigned long ts)
{
   struct timeval now;
   ircline_t *il;

   (void)gettimeofday(&now, NULL);
   if (timediff(&now, &irc.lastdata) > CONNTIMEOUT) {
     irc_conn();
     return;
   }

   if (!list_empty(&irc.linequeue)) {
      if (timediff(&now, &irc.lastline) > LINEDELAY) {
         memcpy(&irc.lastline, &now, sizeof(irc.lastline));
         il = list_entry(irc.linequeue.next, ircline_t, elm);
         irc_sendbuf_now(il->buf, il->len);
         list_del(&il->elm);
         free(il);
      }
   }
}
void irc_cb_err(void *ptr)
{
   irc_conn();   
}

void irc_cb_data(void *ptr)
{
   int pos, len;
   plugin_t *pl;
   char buf[BUFLEN+1], tmp[BUFLEN+1];
  
   (void)gettimeofday(&irc.lastdata, NULL); 
   while ((pos = bufsock_findchr(irc.bs, '\n')) != -1) {
      len = MIN(pos-1, sizeof(buf));
      bufsock_get(irc.bs, buf, len);
      bufsock_delete(irc.bs, pos-len);
      buf[len] = '\0';
      if (len > 0 && buf[len-1] == '\r')
         buf[len-1] = '\0';
#ifndef NDEBUG
      printc("<- %s\n", buf);
#endif
      if (list_empty(&plugins))
         continue;
      list_for_each_entry(pl, &plugins, elm) {
         /* restore info as some modules destroy it :( */
         memcpy(tmp, buf, sizeof(tmp));
         irc_makeinfo(tmp, &info);
         if (pl->reply && pl->reply(&info))
            break;
      }
   }
}

