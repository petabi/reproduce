#include <arpa/inet.h>
#include <array>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <functional>
#include <iomanip>
#include <memory>
#include <stdexcept>
#include <vector>

#include "arp.h"
#include "ethernet.h"
#include "icmp.h"
#include "ipv4.h"
#include "service.h"
#include "tcp.h"
#include "udp.h"

#include "converter.h"
#include "event.h"
#include "util.h"

struct Traffic;

extern "C" {

uint64_t traffic_make_next_message(Traffic*, uint64_t, ForwardMode*, size_t);
Traffic* traffic_new();
size_t traffic_session_count(const Traffic*);
size_t traffic_update_session(Traffic*, uint32_t, uint32_t, uint16_t, uint16_t,
                              uint8_t, const char*, size_t, uint64_t);

} // extern "C"

constexpr std::array<char, 4> src_key{"src"};
constexpr std::array<char, 4> dst_key{"dst"};
constexpr std::array<char, 6> sport_key{"sport"};
constexpr std::array<char, 6> dport_key{"dport"};
constexpr std::array<char, 6> proto_key{"proto"};
#define SESSION_EXTRA_BYTES                                                    \
  (src_key.size() + dst_key.size() + sport_key.size() + dport_key.size() +     \
   proto_key.size() + 2 * 5 + (2 * sizeof(uint32_t)) +                         \
   (2 * sizeof(uint16_t)) + sizeof(uint8_t))

constexpr auto message_label = "message";

// size of message_label includes null terminator.
constexpr size_t message_n_label_bytes =
    (sizeof(message_label) - 1) + sizeof(size_t);

using namespace std;

inline auto BOUNDARY_SAFE(const size_t& remain_size, const size_t& struct_size)
    -> bool
{
  return (remain_size > struct_size);
}

/**
 * PacketConverter
 */
PacketConverter::PacketConverter(const uint32_t _l2_type)
    : traffic(traffic_new()), l2_type(_l2_type)
{
}

auto PacketConverter::convert(uint64_t event_id, char* in, size_t in_len,
                              ForwardMode* msg) -> Conv::Status
{
  if (in == nullptr) {
    return Conv::Status::Fail;
  }
  if (!BOUNDARY_SAFE(in_len, sizeof(pcap_sf_pkthdr))) {
    return Conv::Status::Fail;
  }
  dst = 0;
  ip_hl = 0;
  l4_hl = 0;
  src = 0;
  dport = 0;
  sport = 0;
  proto = 0;
  match = false;
  if (!invoke(get_l2_process(), this,
              reinterpret_cast<unsigned char*>(in + sizeof(pcap_sf_pkthdr)),
              in_len - sizeof(pcap_sf_pkthdr))) {
    return Conv::Status::Fail;
  }
  if (match == true) {
    return Conv::Status::Pass;
  }
  if (l3_type == ETHERTYPE_IP) {
    // update_pack_message(event_id, pm, in, in_len);
    payload_only_message(event_id, msg, in, in_len);
  } else {
    /* only the packets with ip header will be send to kafka.
    std::vector<unsigned char> binary_data(in + sizeof(pcap_sf_pkthdr),
                                           in + in_len);
    if (!forward_mode_append_raw(msg, event_id, "message", binary_data.data(),
                                 binary_data.size())) {
      // invalid raw entry
      return Conv::Status::Fail;
    }
     */
    return Conv::Status::Fail;
  }

#ifdef DEBUG
  std::vector<unsigned char> binary_data_db(in + sizeof(pcap_sf_pkthdr),
                                            in + in_len);
  std::stringstream ss;
  ss << std::hex;
  for (size_t i = 0; i < binary_data_db.size(); ++i) {
    ss << std::setw(2) << std::setfill('0') << (int)binary_data_db[i] << ' ';
  }
  Util::dprint(F, "Binary packet data : ", ss.str());
#endif

  return Conv::Status::Success;
}

auto PacketConverter::get_l2_process()
    -> bool (PacketConverter::*)(unsigned char* offset, size_t length)
{
  switch (l2_type) {
  case 1:
    return &PacketConverter::l2_ethernet_process;
  default:
    break;
  }

  return &PacketConverter::l2_null_process;
}

auto PacketConverter::get_l3_process()
    -> bool (PacketConverter::*)(unsigned char* offset, size_t length)
{
  switch (l3_type) {
  case ETHERTYPE_IP:
    return &PacketConverter::l3_ipv4_process;
  case ETHERTYPE_ARP:
    return &PacketConverter::l3_arp_process;
  default:
    break;
  }

  return &PacketConverter::l3_null_process;
}

auto PacketConverter::get_l4_process()
    -> bool (PacketConverter::*)(unsigned char* offset, size_t length)
{
  switch (l4_type) {
  case IPPROTO_ICMP:
    return &PacketConverter::l4_icmp_process;
  case IPPROTO_TCP:
    return &PacketConverter::l4_tcp_process;
  case IPPROTO_UDP:
    return &PacketConverter::l4_udp_process;
  default:
    break;
  }

  return &PacketConverter::l4_null_process;
}

auto PacketConverter::l2_ethernet_process(unsigned char* offset, size_t length)
    -> bool
{
  if (!BOUNDARY_SAFE(length, sizeof(ether_header))) {
    return false;
  }

  auto eh = reinterpret_cast<ether_header*>(offset);

  offset += sizeof(ether_header);
  length -= sizeof(ether_header);

  l3_type = ntohs(eh->ether_type);

  if (l3_type == 0x8100) { // IEEE 802.1Q VLAN tagging
    l3_type = ntohs(*(reinterpret_cast<uint16_t*>(offset + 2)));
    length -= 4;
    offset += 4;
    vlan = 4;
  }

  if (!invoke(get_l3_process(), this, offset, length)) {
    return false;
  }

  return true;
}

auto PacketConverter::l3_ipv4_process(unsigned char* offset, size_t length)
    -> bool
{
  auto iph = reinterpret_cast<ip*>(offset);
  ip_hl = IP_HL(iph->ip_vhl) * 4;
  if (!BOUNDARY_SAFE(length, ip_hl) || ip_hl < IP_MINLEN) {
    return false;
  }
  offset += ip_hl;
  l4_type = iph->ip_p;
  proto = iph->ip_p;
  src = ntohl(*(reinterpret_cast<uint32_t*>(iph->ip_src)));
  dst = ntohl(*(reinterpret_cast<uint32_t*>(iph->ip_dst)));
  return invoke(get_l4_process(), this, offset, length - ip_hl);
}

auto PacketConverter::l3_arp_process(unsigned char* offset, size_t length)
    -> bool
{
  // Use additional inspection if necessary
  return true;
}

auto PacketConverter::l2_null_process(unsigned char* offset, size_t length)
    -> bool
{
  return true;
}

auto PacketConverter::l3_null_process(unsigned char* offset, size_t length)
    -> bool
{
  return true;
}

auto PacketConverter::l4_null_process(unsigned char* offset, size_t length)
    -> bool
{
  return true;
}

auto PacketConverter::l4_tcp_process(unsigned char* offset, size_t length)
    -> bool
{
  auto tcph = reinterpret_cast<tcphdr*>(offset);
  l4_hl = TCP_HLEN(tcph->th_offx2) * 4;
  if (!BOUNDARY_SAFE(length, l4_hl) || l4_hl < TCP_MINLEN) {
    return false;
  }
  offset += l4_hl;
  sport = ntohs(tcph->th_sport);
  dport = ntohs(tcph->th_dport);
  if (matc) {
    match =
        matcher_match(matc, reinterpret_cast<char*>(offset), length - l4_hl);
  }

  return true;
}

auto PacketConverter::l4_udp_process(unsigned char* offset, size_t length)
    -> bool
{
  auto udph = reinterpret_cast<udphdr*>(offset);
  l4_hl = sizeof(struct udphdr);
  if (!BOUNDARY_SAFE(length, l4_hl) || l4_hl < sizeof(struct udphdr)) {
    return false;
  }
  sport = ntohs(udph->uh_sport);
  dport = ntohs(udph->uh_dport);
  offset += l4_hl;
  if (matc) {
    match =
        matcher_match(matc, reinterpret_cast<char*>(offset), length - l4_hl);
  }

  return true;
}

auto PacketConverter::l4_icmp_process(unsigned char* offset, size_t length)
    -> bool
{
  l4_hl = ICMP_MINLEN;
  if (!BOUNDARY_SAFE(length, l4_hl) || l4_hl < ICMP_MINLEN) {
    return false;
  }
  offset += ICMP_MINLEN;
  if (matc) {
    match =
        matcher_match(matc, reinterpret_cast<char*>(offset), length - l4_hl);
  }
  return true;
}

auto PacketConverter::payload_only_message(uint64_t event_id, ForwardMode* msg,
                                           const char* in, size_t in_len)
    -> Conv::Status
{
  if (in == nullptr || in_len == 0) {
    return Conv::Status::Fail;
  }

  size_t data_offset =
      sizeof(pcap_sf_pkthdr) + sizeof(ether_header) + ip_hl + l4_hl + vlan;

  std::vector<unsigned char> binary_data(in + data_offset, in + in_len);
  if (!forward_mode_append_raw(msg, event_id, "message", binary_data.data(),
                               binary_data.size())) {
    // invalid raw entry
    return Conv::Status::Fail;
  }

  return Conv::Status::Success;
}

void PacketConverter::update_pack_message(uint64_t event_id, ForwardMode* msg,
                                          size_t max_bytes, const char* in,
                                          size_t in_len)
{
  if (in == nullptr && in_len == 0) {
    traffic_make_next_message(traffic, event_id, msg, max_bytes);
    return;
  }
  if (in == nullptr || in_len == 0) {
    return;
  }
  size_t data_offset =
      sizeof(pcap_sf_pkthdr) + sizeof(ether_header) + ip_hl + l4_hl + vlan;
  traffic_update_session(traffic, src, dst, sport, dport, proto,
                         in + data_offset, in_len - data_offset, event_id);

  size_t estimated_data = (traffic_session_count(traffic) *
                           (SESSION_EXTRA_BYTES + message_n_label_bytes)) +
                          traffic_data_len(traffic) +
                          forward_mode_serialized_len(msg);
  if (estimated_data >= max_bytes) {
    traffic_make_next_message(traffic, event_id, msg, max_bytes);
  }
}

/**
 * LogConverter
 */

auto LogConverter::convert(uint64_t event_id, char* in, size_t in_len,
                           ForwardMode* msg) -> Conv::Status
{
  if (in == nullptr) {
    return Conv::Status::Fail;
  }

  if (matc) {
    if (matcher_match(matc, in, in_len)) {
      return Conv::Status::Pass;
    }
  }

  std::vector<unsigned char> binary_data(in, in + in_len);
  if (!forward_mode_append_raw(msg, event_id, "message", binary_data.data(),
                               binary_data.size())) {
    // invalid raw entry
    return Conv::Status::Fail;
  }

  return Conv::Status::Success;
}

// vim: et:ts=2:sw=2
