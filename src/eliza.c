/* 
 * eliza.c -- Chatbot.
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

#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#include "irc.h"
#include "util.h"
#include "conf.h"
#include "auth.h"
#include "rrand.h"

#define CONV_TIMEOUT    30

typedef struct {
   char *str;
   struct list_head lst;
} liststr_t;

typedef struct {
   struct list_head words;
   int respcnt;
   struct list_head resps;
   struct list_head lst;
} variant_t;

typedef struct {
   char *key;
   char *gto;
   int pri;
   struct list_head vars;
   struct list_head lst;
} knowledge_t;

typedef struct {
   char nick[NICKLEN+1];
   char channel[CHANLEN+1];
   unsigned long timeout;
   struct list_head lst;
} conversion_t;

typedef struct {
   char channel[CHANLEN+1];
   char msg[BUFLEN+1];
   unsigned long timeout;
   struct list_head lst;
} answer_t;

typedef struct {
   char *channel;
   char *nick;
   char *me;
} msginfo_t;

static char *kfile = NULL;
static LIST_HEAD(conversions);
static LIST_HEAD(answers);
static LIST_HEAD(knows);
static LIST_HEAD(syns);
static int syncnt = 0;

static int isword(char c)
{
   return (strchr(" \t.,!?:;-'\"&()[]{}/\\", c) == NULL)?1:0;
}

static liststr_t *liststr_add(struct list_head *lsth, const char *str,
      const char *ende)
{
   liststr_t *ls;

   ls = malloc(sizeof(liststr_t));
   if (!ls)
      return NULL;

   if (!ende) {
      ls->str = strdup(str);
   } else {
      int len = ende - str;
      ls->str = malloc(len + 1);
      if (ls->str) {
         memcpy(ls->str, str, len);
         ls->str[len] = '\0';
      }
   }

   if (!ls->str) {
      free(ls);
      return NULL;
   }
   list_add_tail(&ls->lst, lsth);
   
   return ls;
}

static void liststr_free(liststr_t *ls)
{
   free(ls->str);
   list_del(&ls->lst);
   free(ls);
}

static int wordlist_create(struct list_head *lhead, const char *words)
{
   const char *np, *p = words;
   int cnt = 0;

   while(*p) {
      while (!isword(*p))
         ++p;
      np = p;
      while (isword(*np))
         ++np;
      if (np > p) {
         if (!liststr_add(lhead, p, np))
            return -1;
         ++cnt;
      }
      p = np;
   }
   return cnt;
}

static void wordlist_free(struct list_head *lhead)
{
   while(!list_empty(lhead))
      liststr_free(list_entry(lhead->next, liststr_t, lst));
}

static int wordlist_in(struct list_head *lhead, const char *p)
{
   liststr_t *lst;
   int cnt = 0;

   list_for_each_entry(lst, lhead, lst) {
      if (!strcasecmp(lst->str, p))
         return cnt;
      ++cnt;
   }
   return -1;
}
static int wordlist_replace(struct list_head *lhead, const char *str,
      const char *rpl)
{
   liststr_t *lst;
   char *tmp;

   list_for_each_entry(lst, lhead, lst) {
      if (!strcasecmp(str, lst->str)) {
         tmp = lst->str;
         lst->str = strdup(rpl);
         if (!lst->str) {
            lst->str = tmp;
            return -1;
         }
         free(tmp);
         return 1;
      }
   }
   return 0;
}

static int wordlist_match(struct list_head *words, struct list_head *pattern)
{
   int cnt = 0;
   liststr_t *pstr, *wstr, *rpstr;
   
   wstr = list_entry(words->next, liststr_t, lst);
   rpstr = list_entry(pattern->next, liststr_t, lst);
   while (&wstr->lst != words) {
      pstr = rpstr;
      while (&pstr->lst != pattern) {
         if (!strcasecmp(wstr->str, pstr->str)) {
            ++cnt;
            rpstr = list_entry(pstr->lst.next, liststr_t, lst);
            break;
         }
         pstr = list_entry(pstr->lst.next, liststr_t, lst);
      }
      wstr = list_entry(wstr->lst.next, liststr_t, lst);
   }
   
   return cnt;
}

static conversion_t *conv_find(const char *channel, const char *nick)
{
   conversion_t *conv;

   list_for_each_entry(conv, &conversions, lst) {
      if (!strcasecmp(conv->channel, channel) && !strcasecmp(conv->nick, nick))
         return conv;
   }
   return NULL;
}

static void conv_update(conversion_t *conv, const char *channel,
      const char *nick)
{
   if (!conv) {
      conv = (conversion_t*)malloc(sizeof(conversion_t));
      if (!conv)
         return;
      strnzcpy(conv->channel, channel, CHANLEN+1);
      strnzcpy(conv->nick, nick, NICKLEN+1);
      list_add(&conv->lst, &conversions);
   }
   conv->timeout = time(NULL)+CONV_TIMEOUT;
}

static void kfile_free(void)
{
   knowledge_t *kn;
   variant_t *var;

   wordlist_free(&syns);
   syncnt = 0;

   while(!list_empty(&knows)) {
      kn = list_entry((&knows)->next, knowledge_t, lst);
      while(!list_empty(&kn->vars)) {
         var = list_entry((&kn->vars)->next, variant_t, lst);
         wordlist_free(&var->words);
         wordlist_free(&var->resps);
         list_del(&var->lst);
         free(var);
      }
      if (kn->gto)
         free(kn->gto);
      free(kn->key);
      list_del(&kn->lst);
      free(kn);
   } 
}

static int kfile_load(void)
{
   FILE *f;
   char buf[BUFSIZE];
   char *s, *p;
   knowledge_t *kn = NULL;
   variant_t *var = NULL;

   if (!kfile)
      return -1;
   
   f = fopen(kfile, "r");
   if (!f)
      return -1;

   kfile_free();
   while((s = fgets(buf, BUFSIZE, f))) {
      if ((p = strchr(buf, '\n')))
         *p = '\0';
      if ((p = strchr(buf, '\r')))
         *p = '\0';
      if ((p = strchr(buf, '#')))
         *p = '\0';
      while(isspace(*s))
         ++s;
      if (!(p = strchr(s, ':')))
         continue;
      *(p++) = '\0';
      while (isspace(*p))
         ++p;
      if (!*s || !*p)
         continue;
      if (!strcasecmp(s, "syn")) {
         if (!liststr_add(&syns, p, NULL))
            goto err;
         ++syncnt;
      } else if (!strcasecmp(s, "key")) {
         kn = malloc(sizeof(knowledge_t));
         if (!kn)
            goto err;
         kn->key = strdup(p);
         if (!kn->key) {
            free(kn);
            goto err;
         }
         kn->pri = 0;
         kn->gto = NULL;
         INIT_LIST_HEAD(&kn->vars);
         list_add(&kn->lst, &knows);
         var = NULL;
         //printf("key: %s\n", kn->key);
      } else if(!kn) {
         continue;
      } else {
         if (!strcasecmp(s, "pri")) { 
            kn->pri = atoi(p);
            //printf("\tpri: %i\n", kn->pri);
         } else if(!strcasecmp(s, "goto")) {
            if (!kn->gto) {
               kn->gto = strdup(p);
               if (!kn->gto)
                  goto err;
               //printf("\tgoto: %s\n", kn->gto);
            }
         } else if(!strcasecmp(s, "var")) {
            var = malloc(sizeof(variant_t));
            if (!var)
               goto err;
            INIT_LIST_HEAD(&var->words);  
            if (wordlist_create(&var->words, p) == -1) {
               free(var);
               goto err;
            }
            INIT_LIST_HEAD(&var->resps);
            var->respcnt = 0;
            list_add(&var->lst, &kn->vars);
            //printf("\tvar: %s\n",p);
         } else if (var && !strcasecmp(s, "resp")) {
            if (!liststr_add(&var->resps, p, NULL))
               goto err; 
            //printf("\t\tresp: %s\n", p);
            ++var->respcnt;
         }
      }
 
   }
   fclose(f);
   return 0;
err:
   kfile_free();
   fclose(f);
   return -1;
}

void timer(unsigned long ts) {
   answer_t *ans, *nans;
   conversion_t *conv, *nconv;

   list_for_each_entry_safe(ans, nans, &answers, lst) {
      if (ans->timeout < ts) {
         irc_privmsg(ans->channel, "%s", ans->msg);
         list_del(&ans->lst);
         free(ans);
      }
   }

   list_for_each_entry_safe(conv, nconv, &conversions, lst) {
      if (conv->timeout < ts) {
         list_del(&conv->lst);
         free(conv);
      }
   }
}

int mkstr(char *buf, int len, const char *str, msginfo_t *in)
{
   int i;
   char *fs, *p = buf;

   while (*str && len-1 > 0) {
      if (*str == '%' && *(str+1)) {
         switch(*(str+1)) {
            case '%':
               fs = "%";
               break;
            case 'n':
               fs =  in->nick;
               break;
            case 'm':
               fs = in->me;
               break;
            case 'c':
               fs = in->channel;
               break;
            default:
               fs = NULL;
               break;
         }
         if (fs) {
            i = MIN(len-1, strlen(fs));
            memcpy(p, fs, i);
            p += i;
            len -= i;
         }
         str += 2;
      } else {
         *(p++) = *(str++);
         --len;
      }
   }
   *p = '\0';
   --len;
   return (p-buf);
}

int reply(info_t * in) {
   char *p, *sp, *c, *u;
   int tome = 0, s, i;
   conversion_t *conv, *nconv;
   knowledge_t *kn, *bkn;
   variant_t *var, *bvar;
   liststr_t *ls;
   answer_t *ans;
   msginfo_t minfo;
   struct list_head words;

   if (in->cmd == cmd_privmsg) {
      p = strchr(in->tail, ':');
      sp = strchr(in->tail, ' ');
      if (p) {
         *p = '\0';
         if(!sp || sp > p) {
            if (p && *(++p) == ' ')
               ++p;
         }
         if (!strcasecmp(in->tail, in->me)) {
            //printf("talking to me\n");
            tome = 2;
         } else {
            //printf("NOT talking to me\n");
            return 0; /* Not talking to me */
         }
      } else {
         p = in->tail;
         if (strcasestr(p, in->me)) {
            //printf("maybe talking to me\n");
            tome = 1;
         }
      }
      if(!tail_cmd(&p, "eliza") && CHECKAUTH(in->sender, UL_OP)) {
         c = tail_getp(&p);
         u = tail_getp(&p);
         if (!u || !is_channel(c)) {
            irc_notice(in->sender_nick, "Usage: %seliza <channel> <user>", getprefix());
         } else {
            s = rrand(syncnt);
            list_for_each_entry(ls, &syns, lst) {
               if (!s--) {
                  ans = malloc(sizeof(answer_t));
                  if (ans) {
                     strnzcpy(ans->channel, c, CHANLEN+1);
                     minfo.channel = c;
                     minfo.nick =  u;
                     minfo.me = in->me;
                     mkstr(ans->msg, BUFLEN+1, ls->str, &minfo); 
                     ans->timeout = time(NULL) + rand() % 2;
                     list_add(&ans->lst, &answers); 
 
                  }
                  break;
               }
            }
            conv_update(conv_find(c, u), c, u);   
         }
      } else if(!tail_cmd(&p, "uneliza") && CHECKAUTH(in->sender, UL_OP)) {
         c = tail_getp(&p);
         u = tail_getp(&p);
         if (!u || !is_channel(c)) {
            irc_notice(in->sender_nick, "Usage: %suneliza <channel> <user>", getprefix());
         } else {
            list_for_each_entry(conv, &conversions, lst) {
               if (!strcasecmp(c, conv->channel)
                     && !strcasecmp(u, conv->nick)) {
                  list_del(&conv->lst);
                  free(conv);
                  break;
               }
            }
         }
      } else if(is_channel(in->argv[0])) {
         conv = conv_find(in->argv[0], in->sender_nick);
         if (conv || tome) {
            INIT_LIST_HEAD(&words);
            //printf("conversation found: %s\n", p);
            if(wordlist_create(&words, p) != -1) {
               bkn = NULL;
               list_for_each_entry(kn, &knows, lst) {
                  if ((!bkn || bkn->pri < kn->pri)
                     && wordlist_in(&words, kn->key) != -1)
                        bkn = kn;
               }
               if (bkn) {
                  //printf("best known found %p (%s)\n", bkn, bkn->key);
                  if (bkn->gto) {
                     list_for_each_entry(kn, &knows, lst) {
                        if (!strcasecmp(bkn->gto, kn->key)) {
                           wordlist_replace(&words, bkn->key, kn->key);
                           bkn = kn;
                           //printf("\tgoto %p (%s)\n", bkn, bkn->key);
                           break;
                        }  
                     }
                  }
                  bvar = NULL;
                  s = 1;
                  list_for_each_entry(var, &bkn->vars, lst) {
                     if ((i = wordlist_match(&words, &var->words)) >= s) {
                        bvar = var;
                        s = i;
                        //printf("current best variant %p (%i)\n", bvar, s);
                     }
                  }
                  //printf("best variant %p (%i)\n", bvar, s);
                  if (bvar) {
                     s = rand() % bvar->respcnt;
                     list_for_each_entry(ls, &bvar->resps, lst) {
                        if (!s--) {
                           ans = malloc(sizeof(answer_t));
                           if (ans) {
                              strnzcpy(ans->channel, in->argv[0], CHANLEN+1);
                              minfo.channel =  in->argv[0];
                              minfo.nick =  in->sender_nick;
                              minfo.me =  in->me;
                              mkstr(ans->msg, BUFLEN+1, ls->str, &minfo); 
                              ans->timeout = time(NULL) + rand() % 4;
                              list_add(&ans->lst, &answers); 
                           }    
                           conv_update(conv, in->argv[0], in->sender_nick);
                           break;
                        }
                  
                     }
                  }
               } else if (tome > 1) {
                  /* TODO: Random response */
               }
            }
            wordlist_free(&words);
         }
      }
   } else if(in->cmd == cmd_nick) {
      list_for_each_entry(conv, &conversions, lst) {
         if (!strcasecmp(in->sender_nick, conv->nick))
            strnzcpy(conv->nick, in->tail, NICKLEN+1);
      }
   } else if(in->cmd == cmd_quit) {
      list_for_each_entry_safe(conv, nconv, &conversions, lst) {
         if (!strcasecmp(in->sender_nick, conv->nick)) {
            list_del(&conv->lst);
            free(conv);
         }
      }
   } else if(in->cmd == cmd_part) {
      list_for_each_entry_safe(conv, nconv, &conversions, lst) {
         if (!strcasecmp(in->sender_nick, conv->nick)
               && !strcasecmp(in->argv[0], conv->channel)) {
            list_del(&conv->lst);
            free(conv);
         }
      }
   }
   return 0;
}

int init(void) {
   
   kfile = config_get("eliza.so", "knowledgefile");
   if (!kfile || !strlen(kfile)) {
      warn("No knowledge file given.\n");
      return -1;
   }
   
   if (kfile_load() == -1) {
      warn("Failed to load knowledge file.\n");
   } 
   return 0;
}

void destroy(void)
{
   conversion_t *conv;
   answer_t *ans;
   
   while(!list_empty(&answers)) {
      ans = list_entry((&answers)->next, answer_t, lst);
      list_del(&ans->lst);
      free(ans);
   }

   while(!list_empty(&conversions)) {
      conv = list_entry((&conversions)->next, conversion_t, lst);
      list_del(&conv->lst);
      free(conv);
   }

   kfile_free();
}

PLUGIN_DEF(init, destroy, reply, timer, NULL, PLUGIN_NODEP, PLUGIN_NOHELP)

