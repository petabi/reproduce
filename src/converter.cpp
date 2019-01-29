#include <arpa/inet.h>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <ctime>
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
#include "util.h"

using namespace std;

inline bool BOUNDARY_SAFE(size_t& remain_size, const size_t& struct_size)
{
  if (remain_size < struct_size) {
    return false;
  }

  return true;
}

/**
 * Converter
 */
void Converter::set_matcher(const std::string& filename, const Mode& mode)
{
  matc = make_unique<Matcher>(filename, mode);
}

/**
 * PacketConverter
 */
PacketConverter::PacketConverter(const uint32_t _l2_type) : l2_type(_l2_type) {}

Conv::Status PacketConverter::convert(char* in, size_t in_len, PackMsg& pm)
{
  if (in == nullptr) {
    return Conv::Status::Fail;
  }
  if (!BOUNDARY_SAFE(in_len, sizeof(pcap_sf_pkthdr))) {
    return Conv::Status::Fail;
  }

  match = false;
  if (!invoke(get_l2_process(), this,
              reinterpret_cast<unsigned char*>(in + sizeof(pcap_sf_pkthdr)),
              in_len - sizeof(pcap_sf_pkthdr))) {
    return Conv::Status::Fail;
  }
  if (match == true) {
    return Conv::Status::Pass;
  }
  std::vector<unsigned char> binary_data(in + sizeof(pcap_sf_pkthdr),
                                         in + in_len);
  pm.entry(id++, "message", binary_data);

#ifdef DEBUG
  std::stringstream ss;
  ss << std::hex;
  for (size_t i = 0; i < binary_data.size(); ++i) {
    ss << std::setw(2) << std::setfill('0') << (int)binary_data[i] << ' ';
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

  offset += sizeof(struct ether_header);

  l3_type = htons(eh->ether_type);
  if (!invoke(get_l3_process(), this, offset, length - sizeof(ether_header))) {
    return false;
  }

  return true;
}

bool PacketConverter::l3_ipv4_process(unsigned char* offset, size_t length)
{
  if (!BOUNDARY_SAFE(length, IP_MINLEN)) {
    return false;
  }

  auto iph = reinterpret_cast<ip*>(offset);
  size_t opt = 0;
  offset += IP_MINLEN;

  opt = IP_HL(iph->ip_vhl) * 4 - IP_MINLEN;
  if (opt != 0) {
    if (!BOUNDARY_SAFE(length, IP_MINLEN + opt)) {
      return false;
    }
    offset += opt;
  }

  l4_type = iph->ip_p;
  if (!invoke(get_l4_process(), this, offset, length - (IP_MINLEN + opt))) {
    return false;
  }

  return true;
}

bool PacketConverter::l3_arp_process(unsigned char* offset, size_t length)
{
// Use additional inspection if necessary
#if 0
  auto arph = reinterpret_cast<arp_pkthdr*>(offset);
  uint16_t hrd = 0, pro = 0;
  offset += sizeof(arph);
  hrd = htons(arph->ar_hrd);
  pro = htons(arph->ar_pro);

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

  switch (htons(arph->ar_op)) {
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
  if (!BOUNDARY_SAFE(length, sizeof(TCP_MINLEN))) {
    return false;
  }

  // auto tcph = reinterpret_cast<tcphdr*>(offset);
  // TODO: exclude tcp option field from offset
  offset += TCP_MINLEN;
  if (matc) {
    match = matc->match(reinterpret_cast<char*>(offset), length);
  }

  return true;
}

bool PacketConverter::l4_udp_process(unsigned char* offset, size_t length)
{
  // auto udph = reinterpret_cast<udphdr*>(offset);
  offset += sizeof(struct udphdr);
  if (matc) {
    match = matc->match(reinterpret_cast<char*>(offset), length);
  }

  return true;
}

bool PacketConverter::l4_icmp_process(unsigned char* offset, size_t length)
{
#if 0
  // FIXME: more header processing
  auto icmph = reinterpret_cast<icmp*>(offset);
  offset += ICMP_MINLEN;
#endif
  return true;
}

/**
 * LogConverter
 */

Conv::Status LogConverter::convert(char* in, size_t in_len, PackMsg& pm)
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
  pm.entry(id++, "message", binary_data);

  return Conv::Status::Success;
}

/**
 * NullConverter
 */

Conv::Status NullConverter::convert(char* in, size_t in_len, PackMsg& pm)
{
  return Conv::Status::Success;
}

// vim: et:ts=2:sw=2
