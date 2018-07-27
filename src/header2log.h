#ifndef HEADER2LOG_H
#define HEADER2LOG_H

#include <cstdint>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#include "rdkafka_producer.h"

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
  Pcap(const Pcap&&) = delete;
  Pcap& operator=(const Pcap&&) = delete;
  ~Pcap() { fclose(pcapfile); };
  bool skip_bytes(size_t size);
  const std::string get_next_stream();

private:
  FILE* pcapfile;
  std::ostringstream log_stream;
  unsigned int linktype;
  size_t (Pcap::*get_datalink_process())();
  size_t pcap_header_process();
  size_t ethernet_process();
  size_t ipv4_process();
  size_t icmp_process();
  size_t udp_process();
  size_t tcp_process();
  bool payload_process(size_t remain_len);
};

#endif
