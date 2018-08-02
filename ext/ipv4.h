/* Some of these codes refer to the header file contents in the tcpdump library.
Reference file : ip.h, ipproto.h, print-ip.c
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
#ifndef IPV4_H
#define IPV4_H

typedef unsigned char ipv4[4];

struct ip {
  uint8_t ip_vhl; /* header length, version */
#define IP_V(ip_vhl) ((ip_vhl & 0xf0) >> 4)
#define IP_HL(ip_vhl) (ip_vhl & 0x0f)
  uint8_t ip_tos;         /* type of service */
  uint16_t ip_len;        /* total length */
  uint16_t ip_id;         /* identification */
  uint16_t ip_off;        /* fragment offset field */
#define IP_DF 0x4000      /* dont fragment flag */
#define IP_MF 0x2000      /* more fragments flag */
#define IP_OFFMASK 0x1fff /* mask for fragmenting bits */
  uint8_t ip_ttl;         /* time to live */
  uint8_t ip_p;           /* protocol */
  uint16_t ip_sum;        /* checksum */
  ipv4 ip_src, ip_dst;    /* source and dest address */
};

#define IP_MINLEN 20

#define IPPROTO_IP 0      /* dummy for IP */
#define IPPROTO_HOPOPTS 0 /* IPv6 hop-by-hop options */
#define IPPROTO_ICMP 1    /* control message protocol */
#define IPPROTO_IGMP 2    /* group mgmt protocol */
#define IPPROTO_IPV4 4
#define IPPROTO_TCP 6 /* tcp */
#define IPPROTO_EGP 8 /* exterior gateway protocol */
#define IPPROTO_PIGP 9
#define IPPROTO_UDP 17  /* user datagram protocol */
#define IPPROTO_DCCP 33 /* datagram congestion control protocol */
#define IPPROTO_IPV6 41
#define IPPROTO_ROUTING 43  /* IPv6 routing header */
#define IPPROTO_FRAGMENT 44 /* IPv6 fragmentation header */
#define IPPROTO_RSVP 46     /* resource reservation */
#define IPPROTO_GRE 47      /* General Routing Encap. */
#define IPPROTO_ESP 50      /* SIPP Encap Sec. Payload */
#define IPPROTO_AH 51       /* SIPP Auth Header */
#define IPPROTO_MOBILE 55
#define IPPROTO_ICMPV6 58  /* ICMPv6 */
#define IPPROTO_NONE 59    /* IPv6 no next header */
#define IPPROTO_DSTOPTS 60 /* IPv6 destination options */
#define IPPROTO_MOBILITY_OLD 62
#define IPPROTO_ND 77    /* Sun net disk proto (temp.) */
#define IPPROTO_EIGRP 88 /* Cisco/GXS IGRP */
#define IPPROTO_OSPF 89
#define IPPROTO_PIM 103
#define IPPROTO_IPCOMP 108
#define IPPROTO_VRRP 112 /* See also CARP. */
#define IPPROTO_PGM 113
#define IPPROTO_SCTP 132
#define IPPROTO_MOBILITY 135

#endif
