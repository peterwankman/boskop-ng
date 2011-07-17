/* 
 * I/O -- Header file
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

#ifndef IO_H_
#define IO_H_

#include "list.h"

typedef struct {
   struct list_head elm; 
   int d;
   int (*can_read)(void *);
   int (*can_write)(void *);
   void *ptr;
   int remove:1;
} iod_t;

typedef struct {
   struct list_head elm;
   void (*func)(unsigned long);
   int remove:1;
} iot_t;

int io_register(int d);
int io_unregister(int d);
int io_close(int *d);
int io_wantread(int d, int (*func)(void *));
int io_wantwrite(int d, int (*func)(void *));
int io_setptr(int d, void *ptr);
int io_settimer(void (*func)(unsigned long));
int io_unsettimer(void (*func)(unsigned long));
void io_clear(void);
int io_loop(int msec);

#endif

