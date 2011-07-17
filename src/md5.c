/*
 * md5.c -- An implementation of the MD5 hash algorithm
 *
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "md5.h"

#ifndef MIN
#define MIN(x,y)        ((x<y)?(x):(y))
#endif

#define ROTL(x, n) (((x) << (n)) | ((x) >> (32 - (n))))

#define F(x, y, z) (z ^ (x & (y ^ z)))
#define G(x, y, z) (y ^ (z & (x ^ y)))
#define H(x, y, z) ((x) ^ (y) ^ (z))
#define I(x, y, z) ((y) ^ ((x) | (~z)))

#define FF(a, b, c, d, x, s, ac) { \
(a) += F ((b), (c), (d)) + (x) + (ac); \
(a) = ROTL ((a), (s)); \
(a) += (b); \
}

#define GG(a, b, c, d, x, s, ac) { \
(a) += G ((b), (c), (d)) + (x) + (ac); \
(a) = ROTL ((a), (s)); \
(a) += (b); \
}

#define HH(a, b, c, d, x, s, ac) { \
(a) += H ((b), (c), (d)) + (x) + (ac); \
(a) = ROTL ((a), (s)); \
(a) += (b); \
}

#define II(a, b, c, d, x, s, ac) { \
(a) += I ((b), (c), (d)) + (x) + (ac); \
(a) = ROTL ((a), (s)); \
(a) += (b); \
}

void initcontext(context_t *context) {
	context->a = 0x67452301;
	context->b = 0xefcdab89;
	context->c = 0x98badcfe;
	context->d = 0x10325476;
	context->len = 0;
	context->buf = (uchar*)malloc(128);
}

void processmd5(context_t *context) {
	unsigned int i, a, b, c, d, w[16];
	
	for(i = 0; i < 16; i++) {
		w[i]  = context->buf[i*4];
		w[i] += context->buf[i*4+1] << 8;
		w[i] += context->buf[i*4+2] << 16;
		w[i] += context->buf[i*4+3] << 24;
	}

	a = context->a;
	b = context->b;
	c = context->c;
	d = context->d;
	
	FF (a, b, c, d, w[ 0],  7, 0xd76aa478);
	FF (d, a, b, c, w[ 1], 12, 0xe8c7b756);
	FF (c, d, a, b, w[ 2], 17, 0x242070db);
	FF (b, c, d, a, w[ 3], 22, 0xc1bdceee); 
	FF (a, b, c, d, w[ 4],  7, 0xf57c0faf);
	FF (d, a, b, c, w[ 5], 12, 0x4787c62a);
	FF (c, d, a, b, w[ 6], 17, 0xa8304613);
	FF (b, c, d, a, w[ 7], 22, 0xfd469501);
	FF (a, b, c, d, w[ 8],  7, 0x698098d8);
	FF (d, a, b, c, w[ 9], 12, 0x8b44f7af);
	FF (c, d, a, b, w[10], 17, 0xffff5bb1);        
	FF (b, c, d, a, w[11], 22, 0x895cd7be);        
	FF (a, b, c, d, w[12],  7, 0x6b901122);        
	FF (d, a, b, c, w[13], 12, 0xfd987193);        
	FF (c, d, a, b, w[14], 17, 0xa679438e);        
	FF (b, c, d, a, w[15], 22, 0x49b40821);        

	GG (a, b, c, d, w[ 1],  5, 0xf61e2562); 
	GG (d, a, b, c, w[ 6],  9, 0xc040b340); 
	GG (c, d, a, b, w[11], 14, 0x265e5a51);        
	GG (b, c, d, a, w[ 0], 20, 0xe9b6c7aa); 
	GG (a, b, c, d, w[ 5],  5, 0xd62f105d); 
	GG (d, a, b, c, w[10],  9, 0x02441453); 
	GG (c, d, a, b, w[15], 14, 0xd8a1e681);        
	GG (b, c, d, a, w[ 4], 20, 0xe7d3fbc8); 
	GG (a, b, c, d, w[ 9],  5, 0x21e1cde6); 
	GG (d, a, b, c, w[14],  9, 0xc33707d6);        
	GG (c, d, a, b, w[ 3], 14, 0xf4d50d87); 
	GG (b, c, d, a, w[ 8], 20, 0x455a14ed); 
	GG (a, b, c, d, w[13],  5, 0xa9e3e905);        
	GG (d, a, b, c, w[ 2],  9, 0xfcefa3f8); 
	GG (c, d, a, b, w[ 7], 14, 0x676f02d9); 
	GG (b, c, d, a, w[12], 20, 0x8d2a4c8a);        

	HH (a, b, c, d, w[ 5],  4, 0xfffa3942); 
	HH (d, a, b, c, w[ 8], 11, 0x8771f681); 
	HH (c, d, a, b, w[11], 16, 0x6d9d6122);        
	HH (b, c, d, a, w[14], 23, 0xfde5380c);        
	HH (a, b, c, d, w[ 1],  4, 0xa4beea44); 
	HH (d, a, b, c, w[ 4], 11, 0x4bdecfa9); 
	HH (c, d, a, b, w[ 7], 16, 0xf6bb4b60); 
	HH (b, c, d, a, w[10], 23, 0xbebfbc70);        
	HH (a, b, c, d, w[13],  4, 0x289b7ec6);        
	HH (d, a, b, c, w[ 0], 11, 0xeaa127fa); 
	HH (c, d, a, b, w[ 3], 16, 0xd4ef3085); 
	HH (b, c, d, a, w[ 6], 23, 0x04881d05);  
	HH (a, b, c, d, w[ 9],  4, 0xd9d4d039); 
	HH (d, a, b, c, w[12], 11, 0xe6db99e5);        
	HH (c, d, a, b, w[15], 16, 0x1fa27cf8);        
	HH (b, c, d, a, w[ 2], 23, 0xc4ac5665); 

	II (a, b, c, d, w[ 0],  6, 0xf4292244); 
	II (d, a, b, c, w[ 7], 10, 0x432aff97); 
	II (c, d, a, b, w[14], 15, 0xab9423a7);        
	II (b, c, d, a, w[ 5], 21, 0xfc93a039); 
	II (a, b, c, d, w[12],  6, 0x655b59c3);        
	II (d, a, b, c, w[ 3], 10, 0x8f0ccc92); 
	II (c, d, a, b, w[10], 15, 0xffeff47d);        
	II (b, c, d, a, w[ 1], 21, 0x85845dd1); 
	II (a, b, c, d, w[ 8],  6, 0x6fa87e4f); 
	II (d, a, b, c, w[15], 10, 0xfe2ce6e0);        
	II (c, d, a, b, w[ 6], 15, 0xa3014314); 
	II (b, c, d, a, w[13], 21, 0x4e0811a1);        
	II (a, b, c, d, w[ 4],  6, 0xf7537e82); 
	II (d, a, b, c, w[11], 10, 0xbd3af235);        
	II (c, d, a, b, w[ 2], 15, 0x2ad7d2bb); 
	II (b, c, d, a, w[ 9], 21, 0xeb86d391); 

	context->a += a;
	context->b += b;
	context->c += c;
	context->d += d;
}

void addtocontext(context_t *context, uchar *add, int len) {
	int buflen, newlen, rep = 1;
	
	while(rep) {
		buflen = context->len & 63;
		newlen = buflen + len;
	
		memcpy(context->buf + buflen, add, MIN(64, len));
		if(newlen > 63) {
			processmd5(context);
			memcpy(context->buf, context->buf + 64, 64);
		} 
        
		if(len > 64) {
			context->len +=64;
			len -=64;
			add +=64;
		} else {
			context->len += len ;
			rep = 0;
		}
	}
}

void finishcontext(context_t *context, uchar *out) {
	uchar pad[65] = {
		0x80,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	};
	int i;
	int len = context->len;
	int padlen = ((len&63)<56)?(56-(len&63)):(120-(len&63));
	len <<=3;

	pad[padlen] = len;
	pad[padlen+1] = len >> 8;
	pad[padlen+2] = len >> 16;
	pad[padlen+3] = len >> 24;
	pad[padlen+4] = pad[padlen+5] = pad[padlen+6] = pad[padlen+7] = 0;

	addtocontext(context, pad, padlen+8);

	free(context->buf);

	for(i = 0; i < 4; i++) {
		out[i]    = context->a;
		out[i+4]  = context->b;
		out[i+8]  = context->c;
		out[i+12] = context->d;
		context->a >>= 8;
		context->b >>= 8;
		context->c >>= 8;
		context->d >>= 8;
	}
}

