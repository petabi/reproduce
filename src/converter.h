#ifndef CONVERTER_H
#define CONVERTER_H

#include <cstdint>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

/**
 * Converter
 */

class Converter {
public:
  virtual size_t convert(char* in, size_t in_len, char* out,
                         size_t out_len) = 0;
};

/**
 * PacketConverter
 */

using bpf_int32 = int32_t;
using bpf_u_int32 = uint32_t;
using u_short = unsigned short;

struct pcap_file_header {
  bpf_u_int32 magic;
  u_short version_major;
  u_short version_minor;
  bpf_int32 thiszone;   /* gmt to local correction */
  bpf_u_int32 sigfigs;  /* accuracy of timestamps */
  bpf_u_int32 snaplen;  /* max length saved portion of each pkt */
  bpf_u_int32 linktype; /* data link type (LINKTYPE_*) */
};

struct pcap_timeval {
  bpf_int32 tv_sec;  /* seconds */
  bpf_int32 tv_usec; /* microseconds */
};

struct pcap_pkthdr {
  struct pcap_timeval ts; /* time stamp */
  bpf_u_int32 caplen;     /* length of portion present */
  bpf_u_int32 len;        /* length this packet (off wire) */
};

class PacketConverter : public Converter {
public:
  PacketConverter() = delete;
  PacketConverter(const uint32_t _l2_type);
  PacketConverter(const PacketConverter&) = delete;
  PacketConverter& operator=(const PacketConverter&) = delete;
  PacketConverter(PacketConverter&&) = delete;
  PacketConverter& operator=(const PacketConverter&&) = delete;
  ~PacketConverter() = default;
  size_t convert(char* in, size_t in_len, char* out, size_t out_len) override;

private:
  int conv_len = 0;
  char* ptr;
  int length;
  uint32_t l2_type;
  uint16_t l3_type;
  uint8_t l4_type;
  bool (PacketConverter::*get_l2_process())(unsigned char* offset);
  bool (PacketConverter::*get_l3_process())(unsigned char* offset);
  bool (PacketConverter::*get_l4_process())(unsigned char* offset);
  bool l2_ethernet_process(unsigned char* offset);
  bool l2_null_process(unsigned char* offset);
  bool l3_ipv4_process(unsigned char* offset);
  bool l3_arp_process(unsigned char* offset);
  bool l3_null_process(unsigned char* offset);
  bool l4_icmp_process(unsigned char* offset);
  bool l4_udp_process(unsigned char* offset);
  bool l4_tcp_process(unsigned char* offset);
  bool l4_null_process(unsigned char* offset);
  bool add_conv_len();
};

/**
 * LogConverter
 */

class LogConverter : public Converter {
public:
  LogConverter() = default;
  LogConverter(const LogConverter&) = delete;
  LogConverter& operator=(const LogConverter&) = delete;
  LogConverter(LogConverter&&) = delete;
  LogConverter& operator=(const LogConverter&&) = delete;
  ~LogConverter() = default;
  size_t convert(char* in, size_t in_len, char* out, size_t out_len) override;
};

/**
 * NullConverter
 */

class NullConverter : public Converter {
public:
  NullConverter() = default;
  NullConverter(const NullConverter&) = delete;
  NullConverter& operator=(const NullConverter&) = delete;
  NullConverter(NullConverter&&) = delete;
  NullConverter& operator=(const NullConverter&&) = delete;
  ~NullConverter() = default;
  size_t convert(char* in, size_t in_len, char* out, size_t out_len) override;
};

#endif

// vim: et:ts=2:sw=2
