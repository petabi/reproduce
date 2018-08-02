/* Some of these codes refer to the header file contents in the tcpdump library.
Reference file : netdissect-stdinc.h
Last Modified : 2018-07-20
 */
/*
 * Copyright (c) 1993, 1994, 1996
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that: (1) source code distributions
 * retain the above copyright notice and this paragraph in its entirety, (2)
 * distributions including binary code include the above copyright notice and
 * this paragraph in its entirety in the documentation or other materials
 * provided with the distribution, and (3) all advertising materials mentioning
 * features or use of this software display the following acknowledgement:
 * ``This product includes software developed by the University of California,
 * Lawrence Berkeley Laboratory and its contributors.'' Neither the name of
 * the University nor the names of its contributors may be used to endorse
 * or promote products derived from this software without specific prior
 * written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */
#ifndef UTIL_H
#define UTIL_H

// TODO : Platform-independent implementation of the following functions
#if __BYTE_ORDER == __BIG_ENDIAN
/* The host byte order is the same as network byte order,
   so these functions are all just identity.  */
#define ntohl(x) __uint32_identity(x)
#define ntohs(x) __uint16_identity(x)
#define htonl(x) __uint32_identity(x)
#define htons(x) __uint16_identity(x)
#else
#if __BYTE_ORDER == __LITTLE_ENDIAN
#define ntohl(x) __bswap_32(x)
#define ntohs(x) __bswap_16(x)
#define htonl(x) __bswap_32(x)
#define htons(x) __bswap_16(x)
#endif
#endif

#if defined(__GNUC__) && defined(__i386__) && !defined(__APPLE__) &&           \
    !defined(__ntohl)
#undef ntohl
#undef ntohs
#undef htonl
#undef htons

static __inline__ unsigned long __ntohl(unsigned long x);
static __inline__ unsigned short __ntohs(unsigned short x);

#define ntohl(x) __ntohl(x)
#define ntohs(x) __ntohs(x)
#define htonl(x) __ntohl(x)
#define htons(x) __ntohs(x)

static __inline__ unsigned long __ntohl(unsigned long x)
{
  __asm__("xchgb %b0, %h0\n\t" /* swap lower bytes  */
          "rorl  $16, %0\n\t"  /* swap words        */
          "xchgb %b0, %h0"     /* swap higher bytes */
          : "=q"(x)
          : "0"(x));
  return (x);
}

static __inline__ unsigned short __ntohs(unsigned short x)
{
  __asm__("xchgb %b0, %h0" /* swap bytes */
          : "=q"(x)
          : "0"(x));
  return (x);
}
#endif

#endif
