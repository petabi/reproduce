/* Some of these codes refer to the header file contents in the tcpdump library.
Reference file : tcp.h
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

/*
 * TCP header.
 * Per RFC 793, September, 1981.
 */
struct tcphdr {
  uint16_t th_sport; /* source port */
  uint16_t th_dport; /* destination port */
  uint32_t th_seq;   /* sequence number */
  uint32_t th_ack;   /* acknowledgement number */
  uint8_t th_offx2;  /* data offset, rsvd */
  uint8_t th_flags;
  uint16_t th_win; /* window */
  uint16_t th_sum; /* checksum */
  uint16_t th_urp; /* urgent pointer */
};

#define TCP_MINLEN 20
#define TCP_HLEN(th_offx2) (((th_offx2)&0xf0) >> 4)

/* TCP flags */
#define TH_FIN 0x01
#define TH_SYN 0x02
#define TH_RST 0x04
#define TH_PUSH 0x08
#define TH_ACK 0x10
#define TH_URG 0x20
#define TH_ECNECHO 0x40 /* ECN Echo */
#define TH_CWR 0x80     /* ECN Cwnd Reduced */

#define TCPOPT_EOL 0
#define TCPOPT_NOP 1
#define TCPOPT_MAXSEG 2
#define TCPOLEN_MAXSEG 4
#define TCPOPT_WSCALE 3    /* window scale factor (rfc1323) */
#define TCPOPT_SACKOK 4    /* selective ack ok (rfc2018) */
#define TCPOPT_SACK 5      /* selective ack (rfc2018) */
#define TCPOPT_ECHO 6      /* echo (rfc1072) */
#define TCPOPT_ECHOREPLY 7 /* echo (rfc1072) */
#define TCPOPT_TIMESTAMP 8 /* timestamp (rfc1323) */
#define TCPOLEN_TIMESTAMP 10
#define TCPOLEN_TSTAMP_APPA (TCPOLEN_TIMESTAMP + 2) /* appendix A */
#define TCPOPT_CC 11        /* T/TCP CC options (rfc1644) */
#define TCPOPT_CCNEW 12     /* T/TCP CC options (rfc1644) */
#define TCPOPT_CCECHO 13    /* T/TCP CC options (rfc1644) */
#define TCPOPT_SIGNATURE 19 /* Keyed MD5 (rfc2385) */
#define TCPOLEN_SIGNATURE 18
#define TCP_SIGLEN 16  /* length of an option 19 digest */
#define TCPOPT_SCPS 20 /* SCPS-TP (CCSDS 714.0-B-2) */
#define TCPOPT_UTO 28  /* tcp user timeout (rfc5482) */
#define TCPOLEN_UTO 4
#define TCPOPT_TCPAO 29        /* TCP authentication option (rfc5925) */
#define TCPOPT_MPTCP 30        /* MPTCP options */
#define TCPOPT_FASTOPEN 34     /* TCP Fast Open (rfc7413) */
#define TCPOPT_EXPERIMENT2 254 /* experimental headers (rfc4727) */

#define TCPOPT_TSTAMP_HDR                                                      \
  (TCPOPT_NOP << 24 | TCPOPT_NOP << 16 | TCPOPT_TIMESTAMP << 8 |               \
   TCPOLEN_TIMESTAMP)

#ifndef FTP_PORT
#define FTP_PORT 21
#endif
#ifndef SSH_PORT
#define SSH_PORT 22
#endif
#ifndef TELNET_PORT
#define TELNET_PORT 23
#endif
#ifndef SMTP_PORT
#define SMTP_PORT 25
#endif
#ifndef WHOIS_PORT
#define WHOIS_PORT 43
#endif
#ifndef NAMESERVER_PORT
#define NAMESERVER_PORT 53
#endif
#ifndef HTTP_PORT
#define HTTP_PORT 80
#endif
#ifndef NETBIOS_NS_PORT
#define NETBIOS_NS_PORT 137 /* RFC 1001, RFC 1002 */
#endif
#ifndef NETBIOS_SSN_PORT
#define NETBIOS_SSN_PORT 139 /* RFC 1001, RFC 1002 */
#endif
#ifndef BGP_PORT
#define BGP_PORT 179
#endif
#ifndef RPKI_RTR_PORT
#define RPKI_RTR_PORT 323
#endif
#ifndef SMB_PORT
#define SMB_PORT 445
#endif
#ifndef RTSP_PORT
#define RTSP_PORT 554
#endif
#ifndef MSDP_PORT
#define MSDP_PORT 639
#endif
#ifndef LDP_PORT
#define LDP_PORT 646
#endif
#ifndef PPTP_PORT
#define PPTP_PORT 1723
#endif
#ifndef NFS_PORT
#define NFS_PORT 2049
#endif
#ifndef OPENFLOW_PORT_OLD
#define OPENFLOW_PORT_OLD 6633
#endif
#ifndef OPENFLOW_PORT_IANA
#define OPENFLOW_PORT_IANA 6653
#endif
#ifndef HTTP_PORT_ALT
#define HTTP_PORT_ALT 8080
#endif
#ifndef RTSP_PORT_ALT
#define RTSP_PORT_ALT 8554
#endif
#ifndef BEEP_PORT
#define BEEP_PORT 10288
#endif
#ifndef REDIS_PORT
#define REDIS_PORT 6379
#endif
