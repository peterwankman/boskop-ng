/* 
 * Buffer -- Implementation for stream buffering
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
#include <stdlib.h>
#include <string.h>

#include "buffer.h"

int buffer_init(buffer_t *buf, int size)
{
   buf->data = malloc(size);
   if (!buf->data)
      return -1;
   buf->siz = size;
   buffer_clear(buf);
   return 0;
}

void buffer_free(buffer_t *buf)
{ 
   free(buf->data);
   buf->data = NULL;
   buf->siz = 0;
   buffer_clear(buf);
}

int buffer_size(buffer_t *buf)
{
   return buf->siz;
}

int buffer_length(buffer_t *buf)
{
   if (buf->empty)
      return 0;
   if (buf->full)
      return buf->siz;
   return (buf->wp + buf->siz - buf->rp) % buf->siz;
}

void buffer_clear(buffer_t *buf)
{
   buf->wp = 0;
   buf->rp = 0;
   buf->empty = 1;
   buf->full = 0;
}

void buffer_delete(buffer_t *buf, int len)
{
   if (len <= 0)
      return;
   if (len >= (buf->wp + buf->siz - buf->rp) % buf->siz) {
      buffer_clear(buf);
      return;
   }
   buf->rp = (buf->rp + len) % buf->siz;
   buf->full = 0;
}

int buffer_get(buffer_t *buf, char *ptr, int len)
{
   len = buffer_peek(buf, ptr, len);
   buffer_delete(buf, len);
   return len;
}

int buffer_getchr(buffer_t *buf)
{
   int ret;
   
   ret = buffer_peekchr(buf, 0);
   if (ret != -1)
      buffer_delete(buf, 1);
   return ret;
}

int buffer_peek(buffer_t *buf, char *ptr, int len)
{
   int ahead, have;

   if (buf->empty || len <= 0)
      return 0;
   if (buf->full)
      have = buf->siz;
   else
      have = (buf->wp + buf->siz - buf->rp) % buf->siz;
   len = MIN(len, have);
   ahead = MIN(len, buf->siz - buf->rp);
   memcpy(ptr, &buf->data[buf->rp], ahead);
   if (ahead < len)
      memcpy(&ptr[ahead], buf->data, len - ahead);
   return len;
}

int buffer_peekchr(buffer_t *buf, int pos)
{
   int have;

   if (buf->empty || pos < 0)
      return -1;
   if (buf->full)
      have = buf->siz;
   else
      have = (buf->wp + buf->siz - buf->rp) % buf->siz;

   if (pos >= have)
      return -1;
  return (unsigned char)buf->data[(buf->rp + pos) % buf->siz];
}

int buffer_put(buffer_t *buf, const char *ptr, int len)
{
   int ahead, have;
   if (buf->full || len < 0)
      return -1;
   if (len == 0)
      return 0;
   if (buf->empty)
      have = buf->siz;
   else
      have = (buf->rp + buf->siz - buf->wp) % buf->siz;
   if (len > have)
      return -1;
   ahead = MIN(len, buf->siz - buf->wp);
   memcpy(&buf->data[buf->wp], ptr, ahead);
   if (ahead < len)
      memcpy(buf->data, &ptr[ahead], len - ahead);
   buf->wp = (buf->wp + len) % buf->siz;
   if (buf->wp == buf->rp)
      buf->full = 1;
   buf->empty = 0;
   return 0;
}

int buffer_putchr(buffer_t *buf, char c)
{   
   if (buf->full)
      return -1;

   buf->data[buf->wp++] = c;
   buf->wp %= buf->siz;  
   if (buf->wp == buf->rp)
      buf->full = 1;
   buf->empty = 0;
   return 0; 
}

int buffer_map(buffer_t *buf, char **pp)
{
   if (buf->empty)
      return 0;
   *pp = &buf->data[buf->rp];
   return (buf->wp + buf->siz - buf->rp) % buf->siz;
}

int buffer_vprintf(buffer_t *buf, const char *fmt, va_list ap)
{
   char tmp[4096];
   int len;

   len = vsnprintf(tmp, sizeof(tmp), fmt, ap);
   if (len > sizeof(tmp))
      return -1;
   return buffer_put(buf, tmp, len);
}

int buffer_printf(buffer_t *buf, const char *fmt, ...)
{
   int ret;
   va_list ap;

   va_start(ap, fmt);
   ret = buffer_vprintf(buf, fmt, ap);
   va_end(ap);
   return ret;
}

int buffer_findchr(buffer_t *buf, char c)
{
   int i, n;
   if (buf->empty)
      return -1;

   for (i = buf->rp, n = 1; n==1 || (i != buf->wp); i = (i+1) % buf->siz, ++n)
      if (buf->data[i] == c)
         return n;
   return -1;
}

