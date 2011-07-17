/* 
 * util.h -- util.c header
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

#ifndef HELP_H_
#define HELP_H_

#include "config.h"

#define RL_OFFLINE	0
#define RL_RUNNING	1
#define RL_RESTART	2
#define RL_RELOAD	3

extern int runlevel;

#if defined (__SVR4) && defined (__sun)
char *strcasestr(char *haystack, char *needle);
#endif

int printc(const char *format, ...);
int warn(const char *format, ...);
char *strnzcpy(char *dest, const char *src, unsigned int n);
void strip(char *str);
int is_channel(const char *name);
int timediff(struct timeval *t1, struct timeval *t2);
int setprefix(const char *prefix);
char *getprefix(void);
int tail_cmd(char **tail, const char *c);
int tail_cmp(char **tail, const char *c);
char *tail_getp(char **tail);
int regex(const char *str, const char *match);
char *skip_nick(char *str, char *nick);
int banner(const char *message);
int is_computer_on(void);
double is_computer_on_fire(void);

#endif
