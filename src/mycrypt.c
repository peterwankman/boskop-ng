/* 
 * Copyright (C) 2006  Martin Wolters
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
 */

#include <string.h>
#include "md5.h"

#ifndef MIN
#define MIN(x,y)  ((x<y)?(x):(y))
#endif

static const uchar b64t[64] =
"./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

int mycrypt(uchar *key, uchar *salt, uchar *buffer, int buflen) {
	context_t context, context1, context2;
	uchar hash[16];
	int cnt, keylen, saltlen;

	if(buflen < 23)
		return -1;
	if (!strncmp((char*)salt, "$1$", 3))
		salt += 3;

	keylen = strlen((char*)key);
	saltlen = MIN(strcspn((char*)salt, "$"), 8);

	initcontext(&context1);
	addtocontext(&context1, key, keylen);
	addtocontext(&context1, (uchar*)"$1$", 3);
	addtocontext(&context1, salt, saltlen);

	initcontext(&context2);
	addtocontext(&context2, key, keylen);
	addtocontext(&context2, salt, saltlen);
	addtocontext(&context2, key, keylen);
	finishcontext(&context2, hash);
        
	for(cnt = keylen; cnt > 16; cnt -=16)
		addtocontext(&context1, hash, 16);
	addtocontext(&context1, hash, cnt);

	*hash = 0;
	for(cnt = keylen; cnt > 0; cnt>>=1)
		addtocontext(&context1, (cnt&1)!=0?hash:key, 1);
	finishcontext(&context1, hash); 
	for(cnt = 0; cnt <1000; ++cnt) {
		initcontext(&context);
		if((cnt & 1) != 0)
			addtocontext(&context, key, keylen);
		else    
			addtocontext(&context, hash, 16);
          
		if(cnt % 3 != 0)
			addtocontext(&context, salt, saltlen);
                
		if(cnt % 7 != 0)
			addtocontext(&context, key, keylen);

		if((cnt & 1) != 0)
			addtocontext(&context, hash, 16);
		else    
			addtocontext(&context, key, keylen);
                
		finishcontext(&context, hash);
	}

#define b64_from_24bit(B2, B1, B0, N)                                         \
  do {                                                                        \
    unsigned int w = ((B2) << 16) | ((B1) << 8) | (B0);                       \
    int n = (N);                                                              \
    while (n-- > 0)                                             \
      {                                                                       \
        *buffer++ = b64t[w & 0x3f];                                           \
        w >>= 6;                                                              \
      }                                                                       \
  } while (0)
  
  b64_from_24bit (hash[0], hash[6], hash[12], 4);
  b64_from_24bit (hash[1], hash[7], hash[13], 4);
  b64_from_24bit (hash[2], hash[8], hash[14], 4);
  b64_from_24bit (hash[3], hash[9], hash[15], 4);
  b64_from_24bit (hash[4], hash[10], hash[5], 4);
  b64_from_24bit (0, 0, hash[11], 2);
  *buffer = 0;
   return 0;
}
