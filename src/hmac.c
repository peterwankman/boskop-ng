/* 
 * hmac.c -- hmac-sha1
 * 
 * Copyright (C) 2010  Martin Wolters et al.
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

#include "sha1.h"

/* One of the ugliest functions i've ever written. 
   Have a barf bag handy */

hash_t hmac_sha1(char *key, int keylen, uchar *msg, int msglen) {
	uchar *mykey, *buf, *ipad, *opad;
	hash_t hash;
	int i, L, B;
	
	hash = sha1init();
	L = hash.bytesize;
	B = hash.blocksize;

	hash.h0 = hash.h1 = hash.h2 = hash.h3 = hash.h4 = 0;
	hash.string = NULL;
	
	buf = malloc(B + L + msglen);
	if(buf == NULL)
		return hash;

	opad = malloc(B);
	ipad = malloc(B);
	mykey = malloc(B);
	if(mykey == NULL) {
		return hash;
	}

	if(keylen > B) {
		hash = sha1(key, keylen);
		memcpy(key, hash.string, L);
		free(hash.string);
	} else if(keylen < B) {
		memcpy(mykey, key, keylen);
		for(i = keylen; i < B; i++)
			mykey[i] = 0;
	} else {
		memcpy(mykey, key, keylen);
	}
	
	for(i = 0; i < B; i++) {
		opad[i] = mykey[i] ^ 0x5c;
		ipad[i] = mykey[i] ^ 0x36;
	}
	
	memcpy(buf, ipad, B);
	memcpy(buf + B, msg, msglen);

	hash = sha1(buf, B + msglen);
	
	memcpy(buf, opad, B);
	memcpy(buf + B, hash.string, L);
	free(hash.string);

	hash = sha1(buf, 84);

	free(mykey);	
	free(ipad);
	free(opad);
	free(buf);

	return hash;
}
