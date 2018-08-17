#ifndef HEADER2LOG_H
#define HEADER2LOG_H

#include <cstdint>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#include "rdkafka_producer.h"

constexpr int PACKET_END = 0;
constexpr int PACKET_FAIL = -1;

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

class Pcap {

public:
  Pcap() = delete;
  Pcap(const std::string& filename);
  Pcap(const Pcap&) = delete;
  Pcap& operator=(const Pcap&) = delete;
  Pcap(Pcap&& other) noexcept;
  Pcap& operator=(const Pcap&&) = delete;
  ~Pcap();
  bool skip_packets(size_t size);
  int get_next_stream(char* message, size_t size);

private:
  FILE* pcapfile;
  unsigned int linktype;
  char packet_buf[512];
  char* ptr;
  int stream_length = 0;
  bool (Pcap::*get_datalink_process())(char* offset);
  bool (Pcap::*get_internet_process(uint16_t ether_type))(char* offset);
  bool (Pcap::*get_transport_process(uint8_t ip_p))(char* offset);
  size_t pcap_header_process();
  bool ethernet_process(char* offset);
  bool ipv4_process(char* offset);
  bool arp_process(char* offset);
  bool icmp_process(char* offset);
  bool udp_process(char* offset);
  bool tcp_process(char* offset);
  bool null_process(char* offset);
  bool payload_process(size_t remain_len);
  void add_token_to_stream(const char* fmt, ...);
};

#endif

// vim: et:ts=2:sw=2
