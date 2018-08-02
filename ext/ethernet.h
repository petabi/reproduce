/* Some of these codes refer to the header file contents in the tcpdump library.
Reference file : ethertype.h, print-ether.c
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
#ifndef ETHERNET_H
#define ETHERNET_H

#define ETH_ALEN 6
struct ether_header {
  uint8_t ether_dhost[ETH_ALEN];
  uint8_t ether_shost[ETH_ALEN];
  uint16_t ether_type;
};

#define ETHERTYPE_GRE_ISO                                                      \
  0x00FE                        /* not really an ethertype only used in GRE    \
                                 */
#define ETHERTYPE_PUP 0x0200    /* PUP protocol */
#define ETHERTYPE_IP 0x0800     /* IP protocol */
#define ETHERTYPE_ARP 0x0806    /* Addr. resolution protocol */
#define ETHERTYPE_REVARP 0x8035 /* reverse Addr. resolution protocol */
#define ETHERTYPE_NS 0x0600
#define ETHERTYPE_SPRITE 0x0500
#define ETHERTYPE_TRAIL 0x1000
#define ETHERTYPE_MOPDL 0x6001
#define ETHERTYPE_MOPRC 0x6002
#define ETHERTYPE_DN 0x6003
#define ETHERTYPE_LAT 0x6004
#define ETHERTYPE_SCA 0x6007
#define ETHERTYPE_TEB 0x6558
#define ETHERTYPE_LANBRIDGE 0x8038
#define ETHERTYPE_DECDNS 0x803c
#define ETHERTYPE_DECDTS 0x803e
#define ETHERTYPE_VEXP 0x805b
#define ETHERTYPE_VPROD 0x805c
#define ETHERTYPE_ATALK 0x809b
#define ETHERTYPE_AARP 0x80f3
#define ETHERTYPE_TIPC 0x88ca
#define ETHERTYPE_8021Q 0x8100
#define ETHERTYPE_8021Q9100 0x9100
#define ETHERTYPE_8021Q9200 0x9200
#define ETHERTYPE_8021QinQ 0x88a8
#define ETHERTYPE_IPX 0x8137
#define ETHERTYPE_IPV6 0x86dd
#define ETHERTYPE_PPP 0x880b
#define ETHERTYPE_MPCP 0x8808
#define ETHERTYPE_SLOW 0x8809
#define ETHERTYPE_MPLS 0x8847
#define ETHERTYPE_MPLS_MULTI 0x8848
#define ETHERTYPE_PPPOED 0x8863
#define ETHERTYPE_PPPOES 0x8864
#define ETHERTYPE_NSH 0x894F
#define ETHERTYPE_PPPOED2 0x3c12
#define ETHERTYPE_PPPOES2 0x3c13
#define ETHERTYPE_MS_NLB_HB 0x886f /* MS Network Load Balancing Heartbeat */
#define ETHERTYPE_JUMBO 0x8870
#define ETHERTYPE_LLDP 0x88cc
#define ETHERTYPE_EAPOL 0x888e
#define ETHERTYPE_RRCP 0x8899
#define ETHERTYPE_AOE 0x88a2
#define ETHERTYPE_LOOPBACK 0x9000
#define ETHERTYPE_CFM_OLD 0xabcd    /* 802.1ag depreciated */
#define ETHERTYPE_CFM 0x8902        /* 802.1ag */
#define ETHERTYPE_IEEE1905_1 0x893a /* IEEE 1905.1 */
#define ETHERTYPE_ISO                                                          \
  0xfefe /* nonstandard - used in Cisco HDLC encapsulation */
#define ETHERTYPE_CALM_FAST 0x1111  /* ISO CALM FAST */
#define ETHERTYPE_GEONET_OLD 0x0707 /* ETSI GeoNetworking (before Jan 2013) */
#define ETHERTYPE_GEONET                                                       \
  0x8947 /* ETSI GeoNetworking (Official IEEE registration from Jan 2013) */
#define ETHERTYPE_MEDSA 0xdada /* Marvel Distributed Switch Architecture */

#endif
