/* 
 * rrand.c -- Generate random numbers uniformly distributed in a specified range
 * 
 * Copyright (C) 2010  Martin Wolters
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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
	unsigned int n;
	FILE *src;
} seed_t;

static seed_t s;

static int getrand(void) {
   unsigned int res;

   if (s.src && fread(&res, sizeof(res),1, s.src)) {
      res &= RAND_MAX;
      return (int)res;
   } 
   return rand_r(&s.n);
}

int rrand(int m) {
   int i, k;

   if (m <= 0)
      return 0;

   k = RAND_MAX % m;
   
   do {
      i = getrand();
   } while (i >= (RAND_MAX - k));

   return i % m;
}


void srrand(unsigned int seed, FILE *src) {
   s.src = src;
   s.n = seed;
}

