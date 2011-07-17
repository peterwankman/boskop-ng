/* 
 * plugins.h -- plugins.c header
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

#ifndef PLUGINS_H_
#define PLUGINS_H_

#include "defs.h"

struct list_head plugins;

const char *plugins_nonly(const char *name);
plugin_t *plugins_find(const char *name);
int plugins_depends(const char *name);
int plugins_open(const char *name);
int plugins_require(const char *name);
int plugins_close(const char *name);
void plugins_unload(void);
void plugins_load(void);
int plugins_reload(const char *name);

#endif

