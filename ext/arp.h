/* Some of these codes refer to the header file contents in the tcpdump library.
Reference file : print-arp.c, print-ether.c
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

#ifndef ARP_H
#define ARP_H
#include <map>

using nd_uint16_t = uint16_t;
using nd_uint8_t = uint8_t;

struct arp_pkthdr {
  nd_uint16_t ar_hrd;      /* format of hardware address */
#define ARPHRD_ETHER 1     /* ethernet hardware format */
#define ARPHRD_IEEE802 6   /* token-ring hardware format */
#define ARPHRD_ARCNET 7    /* arcnet hardware format */
#define ARPHRD_FRELAY 15   /* frame relay hardware format */
#define ARPHRD_ATM2225 19  /* ATM (RFC 2225) */
#define ARPHRD_STRIP 23    /* Ricochet Starmode Radio hardware format */
#define ARPHRD_IEEE1394 24 /* IEEE 1394 (FireWire) hardware format */
  nd_uint16_t ar_pro;      /* format of protocol address */
  nd_uint8_t ar_hln;       /* length of hardware address */
  nd_uint8_t ar_pln;       /* length of protocol address */
  nd_uint16_t ar_op;       /* one of: */
#define ARPOP_REQUEST 1    /* request to resolve address */
#define ARPOP_REPLY 2      /* response to previous request */
#define ARPOP_REVREQUEST 3 /* request protocol address given hardware */
#define ARPOP_REVREPLY 4   /* response giving protocol address */
#define ARPOP_INVREQUEST 8 /* request to identify peer */
#define ARPOP_INVREPLY 9   /* response identifying peer */
#define ARPOP_NAK 10       /* NAK - only valif for ATM ARP */

/*
 * The remaining fields are variable in size,
 * according to the sizes above.
 */
#ifdef COMMENT_ONLY
  nd_byte ar_sha[]; /* sender hardware address */
  nd_byte ar_spa[]; /* sender protocol address */
  nd_byte ar_tha[]; /* target hardware address */
  nd_byte ar_tpa[]; /* target protocol address */
#endif
};

#define ARP_HDRLEN 8

const std::map<nd_uint16_t, const char* const> arpop_values = {
    {ARPOP_REQUEST, "Request"},
    {ARPOP_REPLY, "Reply"},
    {ARPOP_REVREQUEST, "Reverse Request"},
    {ARPOP_REVREPLY, "Reverse Reply"},
    {ARPOP_INVREQUEST, "Inverse Request"},
    {ARPOP_INVREPLY, "Inverse Reply"},
    {ARPOP_NAK, "NACK Reply"},
    {0, NULL}};

const std::map<nd_uint16_t, const char* const> arphrd_values = {
    {ARPHRD_ETHER, "Ethernet"}, {ARPHRD_IEEE802, "TokenRing"},
    {ARPHRD_ARCNET, "ArcNet"},  {ARPHRD_FRELAY, "FrameRelay"},
    {ARPHRD_STRIP, "Strip"},    {ARPHRD_IEEE1394, "IEEE 1394"},
    {ARPHRD_ATM2225, "ATM"},    {0, NULL}};

#endif
