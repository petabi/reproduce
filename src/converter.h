#ifndef CONVERTER_H
#define CONVERTER_H

#include <cstdint>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "forward_proto.h"
#include "matcher.h"
#include "sessions.h"

namespace Conv {
enum class Status { Fail = -2, Pass = -1, Success = 0 };
}

/**
 * Converter
 */

class Converter {
public:
  virtual ~Converter() = default;
  virtual Conv::Status convert(uint64_t event_id, char* in, size_t in_len,
                               PackMsg& pm) = 0;

  uint64_t get_id() const;
  Matcher* get_matcher() { return matc.get(); }
  virtual bool remaining_data() const { return false; }
  virtual void set_allowed_entropy_ratio(float e) {}
  void set_id(const uint64_t _id);
  void set_matcher(const std::string& filename, const Mode& mode);
  virtual void update_pack_message(uint64_t event_id, PackMsg& pm,
                                   const char* in = nullptr, size_t in_len = 0)
  {
    return;
  }

protected:
  uint64_t id = 1;
  std::unique_ptr<Matcher> matc{nullptr};
};

/**
 * PacketConverter
 */

using bpf_int32 = int32_t;
using bpf_u_int32 = uint32_t;
using u_short = unsigned short;

struct pcap_timeval {
  bpf_int32 tv_sec;  /* seconds */
  bpf_int32 tv_usec; /* microseconds */
};

struct pcap_sf_pkthdr {
  struct pcap_timeval ts; /* time stamp */
  bpf_u_int32 caplen;     /* length of portion present */
  bpf_u_int32 len;        /* length this packet (off wire) */
};

class PacketConverter : public Converter {
public:
  PacketConverter() = delete;
  PacketConverter(const uint32_t _l2_type);
  PacketConverter(const uint32_t _l2_type, time_t launch_time);
  PacketConverter(const PacketConverter&) = delete;
  PacketConverter& operator=(const PacketConverter&) = delete;
  PacketConverter(PacketConverter&&) = delete;
  PacketConverter& operator=(const PacketConverter&&) = delete;
  ~PacketConverter() override = default;
  Conv::Status convert(uint64_t event_id, char* in, size_t in_len,
                       PackMsg& pm) override;
  bool remaining_data() const override
  {
    return sessions.get_number_bytes_in_sessions() > 0;
  }
  void set_allowed_entropy_ratio(float e) override
  {
    sessions.set_allowed_entropy_ratio(e);
  }
  void update_pack_message(uint64_t event_id, PackMsg& pm,
                           const char* in = nullptr,
                           size_t in_len = 0) override;

private:
  bool match;
  uint32_t dst = 0;
  uint32_t ip_hl = 0;
  uint32_t l4_hl = 0;
  uint32_t l2_type;
  uint32_t src = 0;
  uint16_t dport = 0;
  uint16_t l3_type;
  uint16_t sport = 0;
  uint8_t l4_type;
  uint8_t proto = 0;
  uint8_t vlan = 0;
  Sessions sessions;
  std::ofstream session_file;

  void save_session(uint64_t event_id, uint32_t src, uint32_t dst,
                    uint8_t proto, uint16_t sport, uint16_t dport);
  bool (PacketConverter::*get_l2_process())(unsigned char* offset,
                                            size_t length);
  bool (PacketConverter::*get_l3_process())(unsigned char* offset,
                                            size_t length);
  bool (PacketConverter::*get_l4_process())(unsigned char* offset,
                                            size_t length);
  bool l2_ethernet_process(unsigned char* offset, size_t length);
  bool l2_null_process(unsigned char* offset, size_t length);
  bool l3_ipv4_process(unsigned char* offset, size_t length);
  bool l3_arp_process(unsigned char* offset, size_t length);
  bool l3_null_process(unsigned char* offset, size_t length);
  bool l4_icmp_process(unsigned char* offset, size_t length);
  bool l4_udp_process(unsigned char* offset, size_t length);
  bool l4_tcp_process(unsigned char* offset, size_t length);
  bool l4_null_process(unsigned char* offset, size_t length);
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
  ~LogConverter() override = default;
  Conv::Status convert(uint64_t event_id, char* in, size_t in_len,
                       PackMsg& pm) override;
};

#endif

// vim: et:ts=2:sw=2
