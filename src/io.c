/* 
 * I/O -- I/O handling for multiple fds
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

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/poll.h>
#include <time.h>

#include "io.h"

LIST_HEAD(iolist);
LIST_HEAD(timerlist);
unsigned long iocnt = 0;
int io_initialized = 0;

int io_init(void)
{
   if (io_initialized)
      return 0;
   if (atexit(io_clear))
      return -1;
   io_initialized = 1;
   return 0;
}

iod_t *io_getentry(int d)
{
   iod_t *iod;

   if (list_empty(&iolist))
      return NULL;

   list_for_each_entry(iod, &iolist, elm)
      if (iod->d == d && !iod->remove)
         return iod;
   return NULL;
}

int io_register(int d)
{
   iod_t *iod;
   int flags;
   
   if (io_getentry(d))
      return 0;

   if (io_init())
      return -1;

   iod = calloc(1, sizeof(iod_t));
   if (!iod)
      return -1;

   flags = fcntl(d, F_GETFL);
   if (flags == -1) {
      free(iod);
      return -1;
   }
   
   if (fcntl(d, F_SETFL, flags  | O_NONBLOCK) < 0) {
      free(iod);
      return -1;
   }
   iod->d = d;
   list_add(&iod->elm, &iolist);
   ++iocnt;
   return 0;
}

int io_unregister(int d)
{
   iod_t *iod;

   iod = io_getentry(d);
   if (!iod)
      return -1;
   iod->remove = 1;
   --iocnt;
   return 0;
}

int io_close(int *d)
{
	int rv = 0;

	if (*d != -1) {
		rv = io_unregister(*d);
		(void)close(*d);
		*d = -1;
	}
	return rv;
}

int io_wantread(int d, int (*func)(void *))
{
   iod_t *iod;

   iod = io_getentry(d);
   if (!iod)
      return -1;
   iod->can_read = func;
   return 0;
}

int io_wantwrite(int d, int (*func)(void *))
{
   iod_t *iod;

   iod = io_getentry(d);
   if (!iod)
      return -1;

   iod->can_write = func;
   return 0;
}

int io_setptr(int d, void *ptr)
{
   iod_t *iod;

   iod = io_getentry(d);
   if (!iod)
      return -1;
   iod->ptr = ptr;
   return 0;
}
int io_settimer(void (*func)(unsigned long))
{
   iot_t *iot;
   
   if (!func)
      return -1;

   if (io_init())
      return -1;

   iot = calloc(1, sizeof(iot_t));
   if (!iot)
      return -1;
   iot->func = func;
   list_add(&iot->elm, &timerlist);
   return 0;

}

int io_unsettimer(void (*func)(unsigned long))
{
   iot_t *iot;

   list_for_each_entry(iot, &timerlist, elm) {
      if (iot->func == func) {
         iot->remove = 1;
         return 0;
      }
   }
   return -1;
}


void io_clear(void)
{
   iot_t *iot;
   iod_t *iod;

   while(!list_empty(&timerlist)) {
      iot = list_entry(timerlist.next, iot_t, elm);
      list_del(&iot->elm);
      free(iot);
   }

   while(!list_empty(&iolist)) {
      iod = list_entry(iolist.next, iod_t, elm);
      list_del(&iod->elm);
      free(iod);
   }
}

int io_loop(int msec)
{
   struct pollfd *fds;
   iod_t *iod, *niod;
   unsigned long n, cnt;
   iot_t *iot, *niot;

   if (list_empty(&iolist)) {
      if (msec > 0)
         usleep(msec * 1000);
      return 0;
   }

   cnt = iocnt;
   fds = malloc(cnt * sizeof(struct pollfd));
   if (!fds) {
      if (msec > 0)
         usleep(msec * 1000);
      return -1;
   }

   n = 0;
   list_for_each_entry_safe(iod, niod, &iolist, elm) {
      if (iod->remove) {
         list_del(&iod->elm);
         free(iod);
      } else {
         fds[n].fd = iod->d;
         fds[n].events = 0;
         if (iod->can_read)
            fds[n].events |= POLLIN;
         if (iod->can_write)
            fds[n].events |= POLLOUT;
         if (++n == cnt)
		 break;
      }
   }
   list_for_each_entry_safe(iot, niot, &timerlist, elm) {
      if (iot->remove) {
         list_del(&iot->elm);
         free(iot);
      }
   }

   if (poll(fds, n, msec) == -1) {
      free(fds);
      if (msec > 0)
         usleep(msec * 1000);
      return -1;
   }

   n = 0;
   list_for_each_entry(iod, &iolist, elm) {
      if (fds[n].fd != iod->d)
         continue;
      if (!iod->remove && iod->can_write && fds[n].revents & POLLOUT)
         iod->can_write(iod->ptr);
      if (!iod->remove && iod->can_read && fds[n].revents & POLLIN)
         iod->can_read(iod->ptr);
      if(++n == cnt)
	      break;
   }
   free(fds);

   n = time(NULL);
   list_for_each_entry(iot, &timerlist, elm) {
      if (!iot->remove)
         iot->func(n);
   }
   return 0;
}

