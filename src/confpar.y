%{
/* 
 * confpar.y -- Configurationfile parser
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
#include <stdio.h>
#include <string.h>

#include "confsym.h"
#include "conf.h"
#include "util.h"

struct config_include {
    FILE *file;
    int lineno;
    const char *filename;
    struct config_include *prev;
};


static config_t *config = NULL;
static struct config_include *config_inc = NULL;
static const char *config_filename = NULL;
static int config_init  = 0;
static int config_inccount = 0;

int yyerror(char *);
int yywrap();
void config_include(const char *file);
config_t *config_mktag(char *name, void *data, config_t *next);

%}

%union {
   char *str;
   struct confnode *cfg;
}
%start entry
%token ASSIGN DONE BOPEN BCLOSE
%token <str> IDENT DATA ASTR INCLUDE
%type <str> content
%type <cfg> stmt
%type <cfg> block

%%
entry:      block                           { config = $1; }
block:                                      { $$ = NULL; }
          | IDENT BOPEN stmt BCLOSE block   { $$ = config_mktag($1, $3, $5); }
          | include block                   { $$ = $2; }
          ;
stmt:                                       { $$ = NULL; }
          | IDENT ASSIGN content DONE stmt  { $$ = config_mktag($1, $3, $5); }
          | include stmt                    { $$ = $2; }
          ;
include:    INCLUDE ASSIGN content DONE     { config_include($3); }
          ;
content:    DATA            { $$ = $1; }
          | IDENT           { $$ = $1; }
          | ASTR            { $$ = $1; }
          | INCLUDE         { $$ = $1; }
          ;
%%

int yywrap() {
  struct config_include *inc;

  if ((inc = config_inc)) {
    fclose(yyin);
    yyin = inc->file;
    yylineno = inc->lineno;
    config_filename = inc->filename;
    config_inc = inc->prev;
    free(inc);
    --config_inccount;
    yypop_buffer_state();
    return 0;
  }
  return 1;
}

int yyerror(char *s) {
   warn("Config parsing error: %s (file %s, line %i)\n", s, config_filename, yylineno);
   return 0;
}

void config_include(const char *file)
{
  struct config_include *inc;
 
  if (config_inccount > 1000) {
    (void)yyerror("Too many includes (infinite loop?)");
    return;
  }

  inc = malloc(sizeof(struct config_include));
  if (!inc) {
    (void)yyerror("Out of memory");
    return;
  }
  inc->file = (void*)yyin;
  yyin = fopen(file, "r");
  if (!yyin) {
    char errmsg[512];

    yyin = inc->file;
    free(inc);
    snprintf(errmsg, sizeof(errmsg), "Cannot open include file '%s'", file);
    (void)yyerror(errmsg);
    return;
  }

  inc->lineno = yylineno;
  inc->filename = config_filename;
  inc->prev = config_inc;
  config_inc = inc;
  yylineno = 1;
  config_filename = file;
  ++config_inccount;
#ifndef YY_BUF_SIZE
#define YY_BUF_SIZE 16384
#endif
  yypush_buffer_state(yy_create_buffer( yyin, YY_BUF_SIZE ));
}


config_t *config_mktag(char *name, void *data, config_t *next)
{
  config_t *p;
   
  p = malloc(sizeof(config_t));
  if (!p) {
    yyerror("Out of memory");
    return next;
  }
  p->name = name;
  p->data = data;
  p->next = next;
  return p;
}

int config_parse(const char *in)
{
   int ret;
   
   if (!config_init) {
      if (atexit(config_free))
         return -1;
      config_init = 1;
   }

   yyin = fopen(in, "r");
   if (!yyin)
      return -1;
   config_filename = in;
   config_free();
   ret = yyparse();
  fclose(yyin);
#if defined(LINUX) || defined(linux) || defined(_LINUX_) || defined(_linux_)
   yylex_destroy();
#endif
   return ret;
}

void config_free()
{
   config_t *s, *n;
   while ((s = config)) {
      while((n = s->child)) {
         free(n->name);
         free(n->value);
         s->child = n->next;
         free(n);
      }
      free(s->name);
      config = s->next;
      free(s);
   }
}


config_t *config_findnode(config_t *p, const char *name)
{
  if (!name)
    return NULL;

   for (; p; p = p->next)
      if (p->name && !strcmp(name, p->name))
         return p;
   return NULL;
}

int config_cntchild(config_t *p, const char *name)
{
   int i = 0;
   for (; p; p = p->next) {
      if (p->name && !strcmp(name, p->name))
         ++i;
   }
   return i;
}

char *config_get(const char *sec, const char *name)
{
   config_t *p;
   p = config_findnode(config, sec);
   if (p) {
      p = config_findnode(p->child, name);
      if (p) {
         return p->value;
      }
   }
   return NULL;
}

char *config_getn(const char *sec, const char *name, int n)
{
   config_t *p;
   int i = 0;

   p = config_findnode(config, sec);
   if (p && p->child) {
      for (p = p->child; p; p = p->next) {
         if (p->name && !strcmp(p->name, name)) {
            if (i == n)
               return p->value;
            else
               ++i;
         }
      }
   }
   return NULL;
}

int config_getcnt(const char *sec, const char *name)
{
   config_t *p;
   p = config_findnode(config, sec);
   if (p)
      return config_cntchild(p->child, name);
   return 0;
}

