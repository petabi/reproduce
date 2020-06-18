#ifndef CONVERTER_H
#define CONVERTER_H

#include <cstdint>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>

#include "matcher.h"
#include "sessions.h"

struct ForwardMode;

namespace Conv {
enum class Status { Fail = -2, Pass = -1, Success = 0 };
}

/**
 * Converter
 */

class Converter {
public:
  virtual ~Converter() = default;
  virtual auto convert(uint64_t event_id, char* in, size_t in_len,
                       ForwardMode* pm) -> Conv::Status = 0;

  auto get_matcher() -> Matcher* { return matc.get(); }
  [[nodiscard]] virtual auto remaining_data() const -> bool { return false; }
  virtual void set_allowed_entropy_ratio(float e) {}
  void set_matcher(const std::string& filename, const Mode& mode);
  virtual void update_pack_message(uint64_t event_id, ForwardMode* pm,
                                   size_t max_bytes, const char* in = nullptr,
                                   size_t in_len = 0)
  {
    return;
  }

protected:
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
  auto operator=(const PacketConverter&) -> PacketConverter& = delete;
  PacketConverter(PacketConverter&&) = delete;
  auto operator=(const PacketConverter &&) -> PacketConverter& = delete;
  ~PacketConverter() override = default;
  auto convert(uint64_t event_id, char* in, size_t in_len, ForwardMode* msg)
      -> Conv::Status override;
  auto remaining_data() const -> bool override
  {
    return sessions.get_number_bytes_in_sessions() > 0;
  }
  void set_allowed_entropy_ratio(float e) override
  {
    sessions.set_allowed_entropy_ratio(e);
  }
  void update_pack_message(uint64_t event_id, ForwardMode* msg,
                           size_t max_bytes, const char* in = nullptr,
                           size_t in_len = 0) override;

  auto payload_only_message(uint64_t event_id, ForwardMode* pm, const char* in,
                            size_t in_len) -> Conv::Status;

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

  auto get_l2_process()
      -> bool (PacketConverter::*)(unsigned char* offset, size_t length);
  auto get_l3_process()
      -> bool (PacketConverter::*)(unsigned char* offset, size_t length);
  auto get_l4_process()
      -> bool (PacketConverter::*)(unsigned char* offset, size_t length);

  auto l2_ethernet_process(unsigned char* offset, size_t length) -> bool;
  auto l2_null_process(unsigned char* offset, size_t length) -> bool;
  auto l3_ipv4_process(unsigned char* offset, size_t length) -> bool;
  auto l3_arp_process(unsigned char* offset, size_t length) -> bool;
  auto l3_null_process(unsigned char* offset, size_t length) -> bool;
  auto l4_icmp_process(unsigned char* offset, size_t length) -> bool;
  auto l4_udp_process(unsigned char* offset, size_t length) -> bool;
  auto l4_tcp_process(unsigned char* offset, size_t length) -> bool;
  auto l4_null_process(unsigned char* offset, size_t length) -> bool;
};

/**
 * LogConverter
 */

class LogConverter : public Converter {
public:
  LogConverter() = default;
  LogConverter(const LogConverter&) = delete;
  auto operator=(const LogConverter&) -> LogConverter& = delete;
  LogConverter(LogConverter&&) = delete;
  auto operator=(const LogConverter &&) -> LogConverter& = delete;
  ~LogConverter() override = default;
  auto convert(uint64_t event_id, char* in, size_t in_len, ForwardMode* msg)
      -> Conv::Status override;
};

#endif

// vim: et:ts=2:sw=2
