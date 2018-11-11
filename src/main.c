/* 
 * Boskopmann -- Plugin capable IRC bot
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

#include "config.h"

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <arpa/inet.h>
#ifdef TLS
#include <gnutls/gnutls.h>
#endif

#include "irc.h"
#include "conf.h"
#include "util.h"
#include "io.h"
#include "plugins.h"
#include "dns.h"
#include "rrand.h"

#define RECONNECT_DELAY 60

typedef struct {
   char *config;
   int daemonize;
   char *chroot;
   char *chuser;
} settings_t;

static settings_t settings = { DEFAULT_CONFIG, 0, NULL, NULL };
int runlevel;

int checkconfig(void)
{
   char *p;
   
   if (!(p = config_get("core", "nick")) || !strlen(p)) {
      fprintf(stderr, "Missing core/nick\n");
      return -1;
   }

   if (!(p = config_get("core", "user")) || !strlen(p)) {
      fprintf(stderr, "Missing core/user\n");
      return -1;
   }

   if (!(p = config_get("core", "realname")) || !strlen(p)) {
      fprintf(stderr, "Missing core/realname\n");
      return -1;
   }

   if (!(p = config_get("core", "port")) || !(atoi(p)>0)) {
      fprintf(stderr, "Missing core/port\n");
      return -1;
   }

   if (!config_getcnt("core", "server")) {
      fprintf(stderr, "No core/server given\n");
      return -1;   
   }

   if (!(p = config_get("core", "prefix"))) {
      fprintf(stderr, "No core/prefix given\n");
      return -1;
   } else {
      setprefix(p);
   }
   return 0;
}

void print_version(void)
{
   fprintf(stderr,
PACKAGE_STRING 
#ifdef SVN_REV
"." SVN_REV
#endif
", Copyright (C) 2009-2010  Martin Wolters et al.\n"
"\n"
PACKAGE_NAME " comes with ABSOLUTELY NO WARRANTY.\n"
"This is free software, and you are welcome to redistribute it\n"
"under the conditions GPL version 2 or later.\n");
   exit(-1);
}

void print_usage(const char *cmd)
{
   fprintf(stderr,
"Usage: %s (options)\n"
"\n"
"Where options are:\n"
" -c (configfile)   Specify a configuration file (default is " DEFAULT_CONFIG 
")\n"
" -d                Daemonize (fork to background)\n"
" -j (path)         Chroot to the given path\n"
" -u (user)         Change user id\n"
" -v                Show version\n"
" -h                Show this\n"
      , cmd);
   exit(-1);
}

int secure_it(char *path, char *usr)
{
   struct passwd *pw = NULL;

   if (usr) {
      pw = getpwnam(usr);
      if (!pw)
         return -1;
   }
   
   if (path) {
      if (chroot(path) == -1)
         return -1;
   }

   if (pw) {
      if (setuid(pw->pw_uid) == -1)
         return -1;
   }

   return 0;
}

int main(int argc, char **argv) {
   int ret, opt, plugins_loaded = 0, banner_displayed = 0;
   unsigned long now, lastconn = time(NULL);
   FILE *urandom;

   runlevel = RL_OFFLINE;   

   while ((opt = getopt(argc, argv, "c:dj:u:vh")) != -1) {
      switch(opt) {
         case 'c':
            settings.config = optarg;
            break;
         case 'd':
            settings.daemonize = 1;
            break;
         case 'j':
            settings.chroot = optarg;
            break;
         case 'u':
            settings.chuser = optarg;
            break;
         case 'v':
            print_version();
            break;
         case 'h':
         default:
            print_usage(argv[0]);
            break;
      }
   }

   srand(time(NULL));
   
   urandom = fopen("/dev/urandom", "r");
   srrand(time(NULL) ^ getpid(), urandom);

   if (config_parse(settings.config)) {
      fprintf(stderr, "Unable to load configuration file '%s'.\n",
	settings.config);
      return -1;
   }

   if (checkconfig())
      return -1;

   if (settings.daemonize) {
      ret = fork();
      switch(ret) {
         case -1:
            fprintf(stderr, "Unable to fork to background\n");
            return -1;
        default:
            return 0;
      }
   }

   if (secure_it(settings.chroot, settings.chuser)) {
      fprintf(stderr, "Failed to chroot/setuid\n");
      return -1;
   }

#ifdef TLS
  if (gnutls_global_init() == GNUTLS_E_SUCCESS)
    atexit(gnutls_global_deinit);
  else
    fprintf(stderr, "Unable to initialize TLS library\n");
#endif

  if (dns_init() == -1)
      warn("Unable to initialize dns resolver\n");
  
   for(runlevel = RL_RUNNING; runlevel;) {
      if (irc_init() == -1) {
         warn("Unable to init irc data structure");
         return -1;
      }

      if(!plugins_loaded) {
         plugins_load();
         plugins_loaded = 1;
      }

      if(!banner_displayed) {
         banner_displayed = banner("Welcome to " PACKAGE_STRING
#ifdef SVN_REV
            "." SVN_REV
#endif
         );
      }

      while ((runlevel == RL_RUNNING) && (irc_conn() == -1)) {
         warn("Unable to establish irc connection\n");
         sleep(RECONNECT_DELAY);
      }

      lastconn = time(NULL);
      while(runlevel == RL_RUNNING)
         io_loop(100);
      
      irc_free();

      if((runlevel != RL_RUNNING) && plugins_loaded) { 
         plugins_unload();
         plugins_loaded = 0;
      }
      
      if(runlevel == RL_RELOAD) {
         printc("Reloading config file '%s'...\n", settings.config);
         if(config_parse(settings.config)) {
            warn("Error reloading config file.\n");
            runlevel = RL_OFFLINE;
         } else if(checkconfig()) {
            runlevel = RL_OFFLINE;
         }
         runlevel = RL_RUNNING;
      }

      now = time(NULL);
      if(runlevel != RL_OFFLINE) {
        runlevel = RL_RUNNING;
        if (now < lastconn + RECONNECT_DELAY)
          sleep(lastconn + RECONNECT_DELAY - now);
      }
   }

   if(urandom)
      fclose(urandom);
   return 0;
}


