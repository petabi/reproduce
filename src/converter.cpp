#include <arpa/inet.h>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <functional>
#include <iomanip>
#include <memory>
#include <stdexcept>

#include "arp.h"
#include "ethernet.h"
#include "icmp.h"
#include "ipv4.h"
#include "service.h"
#include "tcp.h"
#include "udp.h"

#include "converter.h"
#include "sessions.h"
#include "util.h"

using namespace std;

inline bool BOUNDARY_SAFE(const size_t& remain_size, const size_t& struct_size)
{
  return (remain_size > struct_size);
}

/**
 * Converter
 */
void Converter::set_matcher(const std::string& filename, const Mode& mode)
{
  matc = make_unique<Matcher>(filename, mode);
}

uint64_t Converter::get_id() const { return id; }

void Converter::set_id(const uint64_t _id) { id = _id; }

/**
 * PacketConverter
 */
PacketConverter::PacketConverter(const uint32_t _l2_type) : l2_type(_l2_type) {}

PacketConverter::PacketConverter(const uint32_t _l2_type, time_t launch_time)
    : l2_type(_l2_type)
{
  if (!session_file.is_open()) {
    char buf[80];

    if (std::filesystem::is_directory("/report")) {
      strftime(buf, sizeof(buf), "/report/sessions.txt-%Y%m%d%H%M%S",
               std::localtime(&launch_time));
    } else {
      strftime(buf, sizeof(buf), "sessions.txt-%Y%m%d%H%M%S",
               std::localtime(&launch_time));
    }
    session_file.open(buf, ios::out | ios::app);
  }
}

void PacketConverter::save_session(uint64_t event_id, uint32_t src,
                                   uint32_t dst, uint8_t proto, uint16_t sport,
                                   uint16_t dport)
{
  if (session_file.is_open()) {
    session_file << event_id << "," << src << "," << dst << ","
                 << (static_cast<uint16_t>(proto)) << "," << sport << ","
                 << dport << endl;
  }
}

Conv::Status PacketConverter::convert(uint64_t event_id, char* in,
                                      size_t in_len, PackMsg& pm)
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
    update_pack_message(event_id, pm, in, in_len);
  } else {
    std::vector<unsigned char> binary_data(in + sizeof(pcap_sf_pkthdr),
                                           in + in_len);
    pm.entry(event_id, "message", binary_data);
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

bool (PacketConverter::*PacketConverter::get_l2_process())(
    unsigned char* offset, size_t length)
{
  switch (l2_type) {
  case 1:
    return &PacketConverter::l2_ethernet_process;
  default:
    break;
  }

  return &PacketConverter::l2_null_process;
}

bool (PacketConverter::*PacketConverter::get_l3_process())(
    unsigned char* offset, size_t length)
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

bool (PacketConverter::*PacketConverter::get_l4_process())(
    unsigned char* offset, size_t length)
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

bool PacketConverter::l2_ethernet_process(unsigned char* offset, size_t length)
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

bool PacketConverter::l3_ipv4_process(unsigned char* offset, size_t length)
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

bool PacketConverter::l3_arp_process(unsigned char* offset, size_t length)
{
// Use additional inspection if necessary
#if 0
  auto arph = reinterpret_cast<arp_pkthdr*>(offset);
  uint16_t hrd = 0, pro = 0;
  offset += sizeof(arph);
  hrd = ntohs(arph->ar_hrd);
  pro = ntohs(arph->ar_pro);

  if ((pro != ETHERTYPE_IP && pro != ETHERTYPE_TRAIL) || arph->ar_pln != 4 ||
      arph->ar_hln != 6) {
    return true;
  }

  unsigned char* eth_sha = offset;
  offset += arph->ar_hln;
  unsigned char* ip_spa = offset;
  offset += arph->ar_pln;
  unsigned char* eth_tha = offset;
  offset += arph->ar_hln;
  unsigned char* ip_tpa = offset;

  switch (ntohs(arph->ar_op)) {
  case ARPOP_REQUEST:
    break;
  case ARPOP_REPLY:
    break;
  case ARPOP_REVREQUEST:
    break;
  case ARPOP_REVREPLY:
    break;
  case ARPOP_INVREQUEST:
    break;
  case ARPOP_INVREPLY:
    break;
  default:
    break;
  }
#endif

  return true;
}

bool PacketConverter::l2_null_process(unsigned char* offset, size_t length)
{
  return true;
}

bool PacketConverter::l3_null_process(unsigned char* offset, size_t length)
{
  return true;
}

bool PacketConverter::l4_null_process(unsigned char* offset, size_t length)
{
  return true;
}

bool PacketConverter::l4_tcp_process(unsigned char* offset, size_t length)
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
    match = matc->match(reinterpret_cast<char*>(offset), length - l4_hl);
  }

  return true;
}

bool PacketConverter::l4_udp_process(unsigned char* offset, size_t length)
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
    match = matc->match(reinterpret_cast<char*>(offset), length - l4_hl);
  }

  return true;
}

bool PacketConverter::l4_icmp_process(unsigned char* offset, size_t length)
{
  l4_hl = ICMP_MINLEN;
  if (!BOUNDARY_SAFE(length, l4_hl) || l4_hl < ICMP_MINLEN) {
    return false;
  }
  offset += ICMP_MINLEN;
  if (matc) {
    match = matc->match(reinterpret_cast<char*>(offset), length - l4_hl);
  }
  return true;
}

void PacketConverter::update_pack_message(uint64_t event_id, PackMsg& pm,
                                          const char* in, size_t in_len)
{
  if (in == nullptr && in_len == 0) {
    sessions.make_next_message(pm, event_id);
    return;
  }
  if (in == nullptr || in_len == 0) {
    return;
  }
  size_t data_offset =
      sizeof(pcap_sf_pkthdr) + sizeof(ether_header) + ip_hl + l4_hl + vlan;
  if (sessions.update_session(src, dst, proto, sport, dport, in + data_offset,
                              in_len - data_offset)) {
    save_session(event_id, src, dst, proto, sport, dport);
  }

  size_t estimated_data =
      (sessions.size() * (session_extra_bytes + message_n_label_bytes)) +
      sessions.get_number_bytes_in_sessions() + pm.get_bytes();
  if (estimated_data >= pm.get_max_bytes()) {
    sessions.make_next_message(pm, event_id);
  }
}

/**
 * LogConverter
 */

Conv::Status LogConverter::convert(uint64_t event_id, char* in, size_t in_len,
                                   PackMsg& pm)
{
  if (in == nullptr) {
    return Conv::Status::Fail;
  }

  if (matc) {
    if (matc->match(in, in_len)) {
      return Conv::Status::Pass;
    }
  }

  std::vector<unsigned char> binary_data(in, in + in_len);
  pm.entry(event_id, "message", binary_data);

  return Conv::Status::Success;
}

// vim: et:ts=2:sw=2
