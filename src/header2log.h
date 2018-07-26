#ifndef HEADER2LOG_H
#define HEADER2LOG_H
#include "rdkafka_producer.h"
#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>

typedef int32_t bpf_int32;
typedef uint32_t bpf_u_int32;
typedef unsigned short u_short;

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
  Pcap(){};
  ~Pcap() { fclose(pcapfile); };
  bool open_pcap(const std::string& filename);
  bool skip_bytes(size_t size);
  bool get_next_stream();
  bool conf_rdkafka(const std::string& brokers, const std::string& topic);
  bool produce_to_rdkafka();
  void print_log_stream();

private:
  FILE* pcapfile;
  std::ostringstream log_stream;
  unsigned int linktype;
  Rdkafka_producer rp;
  size_t (Pcap::*get_datalink_process())();
  bool global_header_process();
  size_t pcap_header_process();
  size_t ethernet_process();
  size_t ipv4_process();
  size_t icmp_process();
  size_t udp_process();
  size_t tcp_process();
  bool payload_process(size_t remain_len);
};

#endif
