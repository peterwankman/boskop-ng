%{
/* 
 * conf.l -- Configurationfile lexer
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
#include <string.h>

#include "confpar.h"

#define YY_NO_INPUT
/* %option noyywrap */
%}
%option yylineno
%%
[ \t\r\n]+      	;
^\#[^\n]*\n       ;
\{			return BOPEN;
\}			return BCLOSE;
:			  return ASSIGN;
;			  return DONE;
include return INCLUDE;
[A-Za-z][A-Za-z0-9\.]*	yylval.str = strdup(yytext); return IDENT;
[A-Za-z0-9\.\,\-_\&\!\+\#\/\\]* yylval.str = strdup(yytext); return ASTR;
\".*?\"  yylval.str = malloc(strlen(yytext)-1); if(yylval.str) { memcpy(yylval.str, &yytext[1], strlen(yytext)-2); yylval.str[strlen(yytext)-2] ='\0'; } return DATA;
%%

