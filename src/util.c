/* 
 * util.c -- helpful functions
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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <regex.h>

#include "defs.h"
#include "util.h"

#if defined (__SVR4) && defined (__sun)   
char *strcasestr(char *haystack, char *needle) {
  char *n, *x, *p = haystack;
  char n1 = tolower(*(needle++));
search_loop: while(*p) {
    if (tolower(*(p++)) == n1) {
      x=p;
      n=needle;
      while(*n || *x)
        if (tolower(*(n++)) != tolower(*(x++)))
          goto search_loop;
      return p-1;
    }
  }
  return NULL;
} 
#endif

int printc(const char *format, ...) {
   va_list list;
   int retval;
   char buf[BUFSIZE];
   time_t now;

   va_start(list, format);
   time(&now);
#if defined (__SVR4) && defined (__sun)   
   ctime_r(&now, buf, BUFSIZE);
#else
   ctime_r(&now, buf);
#endif
   if (*format == '\001') {
      retval = vfprintf(stdout, format+1, list);
   } else {
      fprintf(stdout, "[%.24s] ", buf);
      retval = vfprintf(stdout, format, list);
   }
   va_end(list);
   fflush(stdout);
   return retval;
}


int warn(const char *format, ...) {
   va_list list;
   int retval;
   char buf[BUFSIZE];
   time_t now;

   va_start(list, format);
   time(&now);
#if defined (__SVR4) && defined (__sun)   
   ctime_r(&now, buf, BUFSIZE);
#else
   ctime_r(&now, buf);
#endif
   if (*format == '\001') {
      fprintf(stdout, "\033[1m");
      retval = vfprintf(stdout, format+1, list);
   } else {
      fprintf(stdout, "[%.24s] \033[1m", buf);
      retval = vfprintf(stdout, format, list);
   }
   fprintf(stdout, "\033[0m");
   va_end(list);
   fflush(stdout);
   return retval;
}

char *strnzcpy(char *dest, const char *src, unsigned int n)
{
   n = MIN(n-1, strlen(src));
   memcpy(dest, src, n);
   dest[n] = '\0';
   return dest;
}

void strip(char *str)
{
   int i;

   for (i = 0; str[i]; ++i) {
      if (str[i] == '\n' || str[i] == '\r'){
         str[i] = '\0';
         break;
      }
   }
}

int is_channel(const char *name)
{
   if (!name)
      return 0;

   if (name[0] == '#')
      return 1;
   if (name[0] == '&')
      return 1;
   if (name[0] == '+')
      return 1;
   if (name[0] == '!')
      return 1;
   return 0;
}

int timediff(struct timeval *t1, struct timeval *t2)
{
   int res;
   if (t1->tv_usec > t2->tv_usec) {
      res = t1->tv_usec - t2->tv_usec;
   } else {
      res = - (t2->tv_usec - t1->tv_usec);
   }

   if (t1->tv_sec > t2->tv_sec) {
      res += 1000000 * (t1->tv_sec - t2->tv_sec);
   } else {
      res += -1000000 * (t2->tv_sec - t1->tv_sec);
   }

   return res;
}

char *cmd_prefix = NULL;
int setprefix(const char *prefix)
{
  if (!prefix)
    return -1;

  if (cmd_prefix)
    free(cmd_prefix);

  cmd_prefix = strdup(prefix);
  return cmd_prefix?0:-1;
}

char *getprefix(void)
{
  return cmd_prefix?cmd_prefix:"";
}

int tail_cmd(char **tail, const char *c)
{
  int l1, l2;
  
  if (!tail || !*tail || !c || !cmd_prefix)
       return -1;
  
  l1 = strlen(cmd_prefix);
  if (strncasecmp(*tail, cmd_prefix, l1))
    return -1;

  l2 = strlen(c);
  if (strncasecmp(&(*tail)[l1], c, l2))
    return -1;

  if (!(*tail)[l1+l2]) {
      *tail = NULL;
      return 0;
   }

   if ((*tail)[l1+l2] == ' ') {
      *tail = *tail+l1+l2+1;
      return 0;
   }

   return -1;
}

int tail_cmp(char **tail, const char *c)
{
   int l;
   
   if (!tail || !*tail || !c)
      return -1;

   l = strlen(c);
   if (strncasecmp(*tail, c, l))
      return -1;

   if (!(*tail)[l]) {
      *tail = NULL;
      return 0;
   }

   if ((*tail)[l] == ' ') {
      *tail = *tail+l+1;
      return 0;
   }

   return -1;
}

char *tail_getp(char **tail) {
   char *p , *r;

   if (!tail || !*tail)
      return NULL;

   while (**tail == ' ') ++(*tail);

   if ((p = strchr(*tail, ' '))) {
      *(p++) = '\0';
      r = *tail;
      if (strlen(p))
         *tail = p;
      else
         *tail = NULL;
   } else {
      r = *tail;
      *tail = NULL;
      if (!strlen(r))
         r = NULL;
   }
   return r;
}

int regex_getflags(const char *str)
{
   int ret = REG_EXTENDED | REG_NOSUB | REG_NEWLINE;

   if (str) {
      while (*str) {
         switch(*str) {
            case 'i':
               ret |= REG_ICASE;
               break;
            case 'g':
               ret &= ~REG_NEWLINE;
               break;
         }
         ++str;
      }
   }
   return ret;
}

int regex(const char *str, const char *match)
{
   const char *m = match, *p;
   char *tmp;
   int cflags, ret;
   regex_t rg;

   while(isspace(*m))
      ++m;

   if (*m == '/') {
      ++m;
      p = strrchr(m, '/');
      if (!p)
         return -1;      
      cflags = regex_getflags(p+1);
      ret = p-m;
   } else {
      cflags = regex_getflags(NULL);
      ret = strlen(m);
   }
   tmp = malloc(ret+1);
   if (!tmp)
      return -1;
   memcpy(tmp, m, ret);
   tmp[ret] = '\0';
   if (regcomp(&rg, tmp, cflags) != 0) {
      free(tmp);
      return -1;
   }
   ret = regexec(&rg, str, 0, NULL, 0);
   regfree(&rg);
   free(tmp);
   return ret ? -1 : 0;
}

char *skip_nick(char *str, char *nick)
{
   int len = strlen(nick);

   if (!str || !nick)
      return str;

   if (!strncasecmp(str, nick, len)) {
      str += len;
      if (*str == ':')
         ++str;
      while (isspace(*str))
         ++str;
   }
   return str;
}

int banner(const char *message) {
   int l = strlen(message);
   char *buf = malloc(l + 1);
   if(buf == NULL) return 0;

   memset(buf, '#', l);
   printc("\n");
   printc(" ##%s##\n", buf);
   printc(" # %s #\n", message);
   printc(" ##%s##\n", buf);
   printc("\n");

   free(buf);
   return 1;
}

/\
* Returns 1 if the computer is on. If the computer isn't on, the value returned
  by this function is undefined. *\
/

int is_computer_on(void) {
   int ret;
   return 1?1:ret; 
}

/\
* Returns the temperature of the motherboard if the computer is currently
  on fire. If the computer isn't on fire, the function returns some other
  value. *\
/

double is_computer_on_fire(void) {
   double ret;
   return 1?ret:1;
}
