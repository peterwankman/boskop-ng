/* 
 * auth.c -- Plugin for user authentication
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
#include <stdlib.h>
#include <unistd.h>

#include "irc.h"
#include "auth.h"
#include "util.h"
#include "conf.h"
#include "mycrypt.h"

static userinfo_t *users_head = NULL, *users_tail = NULL;
static char *gsalt = NULL;
static char dbn[BUFSIZE] = "";
static char lvl2str[4][20] = {
   "Not authed",
   "Nobody",
   "Operator",
   "Grandmaster"
};

static userinfo_t *findauth(const char *ident)
{
   userinfo_t *ui;
   
   for (ui = users_head; ui; ui = ui->next) {
         if (!strcasecmp(ident, ui->ident))
            return ui;
  }
   return NULL;
}

static userinfo_t *finduser(const char *name)
{
   userinfo_t *ui;

   for (ui = users_head; ui; ui = ui->next){
      if (!strcasecmp(name, ui->name))
         return ui;
   }
   return NULL;
}

static userinfo_t *adduser(const char *name, const char *hash, int level)
{
   if (!users_tail) {
      users_head = malloc(sizeof(userinfo_t));
      if (!users_head)
         return NULL;
      users_tail = users_head;
   } else {
      users_tail->next = malloc(sizeof(userinfo_t));
      if (!users_tail->next)
         return NULL;
      users_tail = users_tail->next;
   }
   strnzcpy(users_tail->name, name, NICKLEN+1);
   strnzcpy(users_tail->hash, hash, HASHLEN+1);
   users_tail->level = level;
   users_tail->next = NULL;
   return users_tail;
}

static int storedb(void)
{
   char tmp[sizeof(dbn) + 4], buf[NICKLEN+1+HASHLEN+1+sizeof(int)], *hash = &buf[NICKLEN+1];
   FILE *f;
   userinfo_t *ui;
   int ret, *lvl = (int*)&buf[NICKLEN+1+HASHLEN+1];
   if (!strlen(dbn)) {
      printc("No database file given.\n");
      return -1;
   }

   snprintf(tmp, sizeof(tmp), "%s.tmp", dbn);
   if (!(f = fopen(tmp, "w"))) {
      printc("Unable to open temporary file '%s'.\n", tmp);
      return -1;
   }

   for (ui = users_head; ui; ui = ui->next)
   {
      memcpy(buf, ui->name, NICKLEN+1);
      memcpy(hash, ui->hash, HASHLEN+1);
      *lvl = ui->level;
      ret = fwrite(buf, NICKLEN+1+HASHLEN+1+sizeof(int), 1, f);
      if (ret != 1) {
         printc("Write error on temporary file.\n");
         fclose(f);
         unlink(tmp);
         return -1;
      }
   }
   fclose(f);
   unlink(dbn);
   rename(tmp, dbn);
   return 0;
}


int query(info_t * in, va_list ap)
{
   char *sender;
   userinfo_t *ui;
   int cmd = va_arg(ap, int);
   switch(cmd) {
      case AUTHCMD_GET:
         sender = va_arg(ap, char*);
         ui = findauth(sender);
         return ui?ui->level:UL_NOAUTH;
      default:
         break;
   }
   return -1;
}

int reply(info_t * in) {
   char tmp[23];
   char *p, *u;
   userinfo_t *ui;

   if (in->cmd == cmd_privmsg && !is_channel(in->argv[0])) {
      if (!tail_cmd(&in->tail, "register")) {
         u = tail_getp(&in->tail);
         p = tail_getp(&in->tail);
         if (!p) {
            irc_notice(in->sender_nick, "Usage: %sregister <username> <password>", getprefix());
            return 0;
         }

         if (strlen(u) > NICKLEN) {
            irc_notice(in->sender_nick, "Username too long");
            return 0;
         }
         if (finduser(u)) {
            irc_notice(in->sender_nick, "Nickname already registered. Try %sauth <username> <password>", getprefix());
            return 0;
         }
         if (strlen(p) < 6) {
            irc_notice(in->sender_nick, "Password too short");
            return 0;
         }
         if (in->tail) {
            irc_notice(in->sender_nick, "Password must not contain whitespaces");
            return 0;
         }
         (void)mycrypt(p, gsalt, tmp, sizeof(tmp));

         ui = adduser(u, tmp, UL_NOBODY);
         if (ui) {
            strnzcpy(ui->ident, in->sender, SENDERLEN+1);
            irc_notice(in->sender_nick, "You have been registered");
         } else {
            irc_notice(in->sender_nick, "Internal error");
         }
      } else if (!tail_cmd(&in->tail, "auth")) {
         u = tail_getp(&in->tail);
         p = tail_getp(&in->tail);
         if (!p) {
            irc_notice(in->sender_nick, "Usage: %sauth <username> <password>", getprefix());
            return 0;
         }
         ui = finduser(u);
         mycrypt(p, gsalt, tmp, sizeof(tmp));
	      if (!ui || strcmp(ui->hash, tmp)) {
            irc_notice(in->sender_nick, "Authentication error");
            return 0;
         }
         strnzcpy(ui->ident, in->sender, SENDERLEN+1);
         if (ui->level == UL_MASTER)
            irc_notice(in->sender_nick, "Welcome grandmaster %s.", in->sender_nick);
         else if(ui->level == UL_OP)
            irc_notice(in->sender_nick, "You have been authed as operator.");
         else
            irc_notice(in->sender_nick, "You have been authed.");
      } else if (!tail_cmd(&in->tail, "saveauth")) {
          if (CHECKAUTH(in->sender, UL_MASTER))  {
            irc_notice(in->sender_nick, "Storing user database... %s.", storedb()?"Failed":"Done");
         }
      } else if (!tail_cmd(&in->tail, "logout")) {
         ui = findauth(in->sender);
         if (ui) {
            ui->ident[0] = '\0';
            irc_notice(in->sender_nick, "Logout done");
         }
      } else if (!tail_cmd(&in->tail, "list")) {
          if (CHECKAUTH(in->sender, UL_MASTER))  {
            for (ui = users_head; ui; ui = ui->next)
            irc_notice(in->sender_nick, "%s (level %s, ident %s)", ui->name, lvl2str[ui->level], strlen(ui->ident)?ui->ident:"N/A");
            irc_notice(in->sender_nick, "End of userlist");
         }
      } else if(!tail_cmd(&in->tail, "promote")) {
         if (CHECKAUTH(in->sender, UL_MASTER))  {
            ui = finduser(tail_getp(&in->tail));
            if (ui) {
               if (ui->level < UL_MASTER) {
                  ++ui->level;
                  irc_notice(in->sender_nick, "User has been promoted.");
               } else {
                  irc_notice(in->sender_nick, "User is already a grandmaster.");
               }
            } else {
               irc_notice(in->sender_nick, "User not found.");
            }
         }
      } else if(!tail_cmd(&in->tail, "demote")) {
         if (CHECKAUTH(in->sender, UL_MASTER))  {
            ui = finduser(tail_getp(&in->tail));
            if (ui) {
               if (ui->level > UL_NOBODY) {
                  --ui->level;
                  irc_notice(in->sender_nick, "User has been demoted.");
               } else {
                  irc_notice(in->sender_nick, "User is already a nobody.");
               }
            } else {
               irc_notice(in->sender_nick, "User not found.");
            }
         }
      } 
   }
   return 0;
}

int init(void) {
   FILE *f;
   char buf[NICKLEN+1+HASHLEN+1+sizeof(int)], *hash = &buf[NICKLEN+1], *arg;
   char pass[PASSLEN+1];
   int havemaster = 0, *lvl = (int*)&buf[NICKLEN+1+HASHLEN+1];
   userinfo_t *ui;
   
   gsalt = config_get("auth.so", "salt");
   if(!gsalt || !strlen(gsalt)) {
	   printc("No salt provided. Disable auth.\n"); 
      return -1;
   }

   arg = config_get("auth.so", "dbfile");
   if (!arg || !strlen(arg)) {
      printc("No user db given.\n");
      return -1;
   }

   printc("Loading user db '%s'... ",arg);
   if (!(f = fopen(arg, "r"))) {
      printc("\001Failed.\n");
   } else {
      for(;;) {
         if (fread(buf, NICKLEN+1+HASHLEN+1+sizeof(int), 1, f) < 1)
            break;
         ui = adduser(buf, hash, *lvl);
         if (!ui)
            continue;
         if (ui->level >= UL_MASTER)
            havemaster = 1;
      }
      fclose(f);
      printc("\001Done.\n");
   }
   strnzcpy(dbn,arg,sizeof(dbn));
   if (!havemaster) {
      printc("No master was configured. We will do it now.\n");
      (void)fputs("Username: ", stdout);
      (void)fflush(stdout);
      if(!fgets(buf, NICKLEN, stdin))
         goto no_master;
      strip(buf);
      (void)fputs("Password: ", stdout);
      (void)fflush(stdout);
      if(!fgets(pass, PASSLEN, stdin))
         goto no_master;
      strip(pass);
      mycrypt(pass, gsalt, hash, HASHLEN+1);
      *lvl = UL_MASTER;
      if (!adduser(buf, hash, *lvl))
         goto no_master;
      storedb();
   }
	return 0;

no_master:
   warn("Failed to create master\n");
   return -1;
}

void destroy()
{
   userinfo_t *ui;
   
   (void)storedb();
   while ((ui = users_head)) {
      users_head = ui->next;
      free(ui);
   }
   users_tail = NULL;
}

PLUGIN_DEF(init, destroy, reply, NULL, query, PLUGIN_NODEP, 
   PLUGIN_HELP("To register yourself, use 'register <username> <pass>'. To authenticate, use 'auth <username> <pass>'."))

