/* 
 * buffer.h -- header file for buffer.c
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

#ifndef BUFFER_H_
#define BUFFER_H_

#include <stdarg.h>

#ifndef MIN
#define MIN(x,y)  ((x<y)?(x):(y))
#endif

typedef struct {
   char *data;
   int siz, wp, rp;
   int full:1;
   int empty:1;
} buffer_t;

int buffer_init(buffer_t *buf, int size);
void buffer_free(buffer_t *buf);
int buffer_size(buffer_t *buf);
int buffer_length(buffer_t *buf);
void buffer_clear(buffer_t *buf);
void buffer_delete(buffer_t *buf, int len);
int buffer_get(buffer_t *buf, char *ptr, int len);
int buffer_getchr(buffer_t *buf);
int buffer_peek(buffer_t *buf, char *ptr, int len);
int buffer_peekchr(buffer_t *buf, int pos);
int buffer_put(buffer_t *buf, const char *ptr, int len);
int buffer_putchr(buffer_t *buf, char c);
int buffer_map(buffer_t *buf, char **pp);
int buffer_vprintf(buffer_t *buf, const char *fmt, va_list ap);
int buffer_printf(buffer_t *buf, const char *fmt, ...);
int buffer_findchr(buffer_t *buf, char c);
void buffer_arrange(buffer_t *buf);

#endif

