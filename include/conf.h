/* 
 * conf.h -- confpar.y header
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

#ifndef CONF_H_
#define CONF_H_

#include "defs.h"


struct confnode {
   char *name;
   union {
      void *data;
      char *value;
      struct confnode *child;
   };
   struct confnode *next;
};

typedef struct confnode config_t;

int config_parse(const char *in);
void config_free(void);
char *config_get(const char *sec, const char *name);
char *config_getn(const char *sec, const char *name, int n);
int config_getcnt(const char *sec, const char *name);

#endif

