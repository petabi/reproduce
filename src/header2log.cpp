#include <arpa/inet.h>

#include <cstdio>
#include <cstring>
#include <ctime>
#include <functional>
#include <iomanip>
#include <memory>
#include <stdarg.h>
#include <stdexcept>

#include "arp.h"
#include "ethernet.h"
#include "header2log.h"
#include "icmp.h"
#include "ipv4.h"
#include "rdkafka_producer.h"
#include "service.h"
#include "tcp.h"
#include "udp.h"

using namespace std;

Pcap::Pcap(const string& filename)
{
  pcapfile = fopen(filename.c_str(), "r");
  if (pcapfile == nullptr) {
    throw runtime_error("could not open [" + filename + "]");
  }

  struct pcap_file_header pfh;
  if (fread(&pfh, 1, sizeof(pfh), pcapfile) != sizeof(pfh)) {
    throw runtime_error(filename + " is not an appropriate pcap file");
  }
  if (pfh.magic != 0xa1b2c3d4) {
    throw runtime_error(filename + " is not an appropriate pcap file");
  }

  linktype = pfh.linktype;
}

Pcap::Pcap(Pcap&& other) noexcept
{
  FILE* tmp = other.pcapfile;
  other.pcapfile = nullptr;
  if (pcapfile != nullptr) {
    fclose(pcapfile);
  }
  pcapfile = tmp;
}

Pcap::~Pcap()
{
  if (pcapfile != nullptr) {
    fclose(pcapfile);
  }
}

bool Pcap::skip_packets(size_t count_skip)
{
  if (count_skip < 0)
    return false;

  struct pcap_pkthdr pp;
  size_t count = 0;
  while (count < count_skip) {
    if (fread(&pp, 1, sizeof(pp), pcapfile) != sizeof(pp)) {
      return false;
    }
    fseek(pcapfile, pp.caplen, SEEK_CUR);
    count++;
  }

  return true;
}

size_t Pcap::get_next_stream(char* message, size_t size)
{
  stream_length = 0;
  ptr = message;

  size_t packet_len = pcap_header_process();
  if (packet_len == static_cast<size_t>(-1)) {
    return 0;
  }
#if 0
  // we assume packet_len < size
  if (packet_len >= size) {
    throw runtime_error("message buffer too small");
  }
#endif

  if (fread(packet_buf, 1, packet_len, pcapfile) != packet_len) {
    return 0;
  }

  size_t process_len = invoke(get_datalink_process(), this, packet_buf);
  if (process_len == static_cast<size_t>(-1)) {
    throw runtime_error("failed to read packet header");
  }

#if 0
  // TODO: Add payload process
  packet_len -= process_len;
  if (!payload_process(packet_len)) {
    throw runtime_error("failed to read packet payload");
  }
#endif

  return stream_length;
}

size_t (Pcap::*Pcap::get_datalink_process())(char* offset)
{
  switch (linktype) {
  case 1:
    return &Pcap::ethernet_process;
#if 0
  case 6:
    add_token_to_stream("%s ", "Token Ring ");
    break;
  case 10:
    add_token_to_stream("%s ", "FDDI ");
    break;
  case 0:
    add_token_to_stream("%s ", "Loopback ");
    break;
#endif
  default:
    add_token_to_stream("Unknown_L2(%d) ", linktype);
    break;
  }

  return &Pcap::null_process;
}

size_t (Pcap::*Pcap::get_internet_process(uint16_t ether_type))(char* offset)
{
  switch (htons(ether_type)) {
  case ETHERTYPE_IP:
    return &Pcap::ipv4_process;
  case ETHERTYPE_ARP:
    return &Pcap::arp_process;
  default:
    add_token_to_stream("Unknown_L3(%d) ", htons(ether_type));
    break;
  }

  return &Pcap::null_process;
}

size_t (Pcap::*Pcap::get_transport_process(uint8_t ip_p))(char* offset)
{
  switch (ip_p) {
  case IPPROTO_ICMP:
    return &Pcap::icmp_process;
  case IPPROTO_TCP:
    return &Pcap::tcp_process;
  case IPPROTO_UDP:
    return &Pcap::udp_process;
  default:
    add_token_to_stream("Unknown_L4(%d) ", ip_p);
    break;
  }

  return &Pcap::null_process;
}

size_t Pcap::pcap_header_process()
{
  struct pcap_pkthdr pp;
  if (fread(&pp, 1, sizeof(pp), pcapfile) != sizeof(pp)) {
    return -1;
  }

#if 0
  // TODO: Fix to enhance performance
  char* cap_time = nullptr;
  cap_time = (char*)ctime((const time_t*)&sec);
  cap_time[strlen(cap_time) - 1] = '\0';
#else
  add_token_to_stream("%d ", pp.ts.tv_sec);
#endif

  return (size_t)pp.caplen;
}

size_t Pcap::ethernet_process(char* offset)
{
  struct ether_header* eh = reinterpret_cast<ether_header*>(offset);
  size_t process_len = 0;
  offset += sizeof(struct ether_header);
  process_len += sizeof(struct ether_header);

  add_token_to_stream(
      "Ethernet2 %02x:%02x:%02x:%02x:%02x:%02x %02x:%02x:%02x:%02x:%02x:%02x ",
      (eh->ether_dhost)[0], (eh->ether_dhost)[1], (eh->ether_dhost)[2],
      (eh->ether_dhost)[3], (eh->ether_dhost)[4], (eh->ether_dhost)[5],
      (eh->ether_shost)[0], (eh->ether_shost)[1], (eh->ether_shost)[2],
      (eh->ether_shost)[3], (eh->ether_shost)[4], (eh->ether_shost)[5]);

  process_len += invoke(get_internet_process(eh->ether_type), this, offset);
  if (process_len == static_cast<size_t>(-1)) {
    throw runtime_error("failed to read internet header");
  }

  return process_len;
}

size_t Pcap::ipv4_process(char* offset)
{
  struct ip* iph = reinterpret_cast<ip*>(offset);
  size_t opt = 0;
  size_t process_len = 0;
  offset += sizeof(IP_MINLEN);
  process_len += IP_MINLEN;

  add_token_to_stream("IP %d %d %d %d %d %d %d %d %d.%d.%d.%d %d.%d.%d.%d ",
                      IP_V(iph->ip_vhl), IP_HL(iph->ip_vhl), iph->ip_tos,
                      iph->ip_len, iph->ip_id, iph->ip_off, iph->ip_ttl,
                      iph->ip_sum, iph->ip_src[0], iph->ip_src[1],
                      iph->ip_src[2], iph->ip_src[3], iph->ip_dst[0],
                      iph->ip_dst[1], iph->ip_dst[2], iph->ip_dst[3]);

  opt = IP_HL(iph->ip_vhl) * 4 - sizeof(IP_MINLEN);
  if (opt != 0) {
    offset += opt;
    process_len += opt;
    add_token_to_stream("%s ", "ip_opt");
  }

  process_len += invoke(get_transport_process(iph->ip_p), this, offset);
  if (process_len == static_cast<size_t>(-1)) {
    throw runtime_error("failed to read transport header");
  }

  return process_len;
}

size_t Pcap::arp_process(char* offset)
{
  struct arp_pkthdr* arph = reinterpret_cast<arp_pkthdr*>(offset);
  uint16_t hrd = 0, pro = 0;
  size_t process_len = 0;
  offset += sizeof(arph);
  process_len += sizeof(arph);
  hrd = htons(arph->ar_hrd);
  pro = htons(arph->ar_pro);

  add_token_to_stream("ARP %s %s ", arpop_values.find(hrd)->second,
                      ethertype_values.find(pro)->second);

  if ((pro != ETHERTYPE_IP && pro != ETHERTYPE_TRAIL) || arph->ar_pln != 4 ||
      arph->ar_hln != 6) {
    return process_len;
  }

  char* eth_sha = offset;
  offset += arph->ar_hln;
  process_len += arph->ar_hln;
  char* ip_spa = offset;
  offset += arph->ar_pln;
  process_len += arph->ar_pln;
  char* eth_tha = offset;
  offset += arph->ar_hln;
  process_len += arph->ar_hln;
  char* ip_tpa = offset;
  offset += arph->ar_pln;
  process_len += arph->ar_pln;

  switch (htons(arph->ar_op)) {
  case ARPOP_REQUEST:
    add_token_to_stream("who-has %d.%d.%d.%d tell %d.%d.%d.%d ", ip_tpa[0],
                        ip_tpa[1], ip_tpa[2], ip_tpa[3], ip_spa[0], ip_spa[1],
                        ip_spa[2], ip_spa[3]);
    break;
  case ARPOP_REPLY:
    add_token_to_stream("%d.%d.%d.%d is-at %02x:%02x:%02x:%02x:%02x:%02x ",
                        ip_spa[0], ip_spa[1], ip_spa[2], ip_spa[3], eth_sha[0],
                        eth_sha[1], eth_sha[2], eth_sha[3], eth_sha[4],
                        eth_sha[5]);
    break;
  case ARPOP_REVREQUEST:
    add_token_to_stream("who-is %02x:%02x:%02x:%02x:%02x:%02x tell "
                        "%02x:%02x:%02x:%02x:%02x:%02x ",
                        eth_tha[0], eth_tha[1], eth_tha[2], eth_tha[3],
                        eth_tha[4], eth_tha[5], eth_tha[0], eth_sha[1],
                        eth_sha[2], eth_sha[3], eth_sha[4], eth_sha[5]);
    break;
  case ARPOP_REVREPLY:
    add_token_to_stream("%02x:%02x:%02x:%02x:%02x:%02x at %d.%d.%d.%d ",
                        eth_tha[0], eth_tha[1], eth_tha[2], eth_tha[3],
                        eth_tha[4], eth_tha[5], ip_tpa[0], ip_tpa[1], ip_tpa[2],
                        ip_tpa[3]);
    break;
  case ARPOP_INVREQUEST:
    add_token_to_stream("who-is %02x:%02x:%02x:%02x:%02x:%02x tell "
                        "%02x:%02x:%02x:%02x:%02x:%02x ",
                        eth_tha[0], eth_tha[1], eth_tha[2], eth_tha[3],
                        eth_tha[4], eth_tha[5], eth_tha[0], eth_sha[1],
                        eth_sha[2], eth_sha[3], eth_sha[4], eth_sha[5]);
    break;
  case ARPOP_INVREPLY:
    add_token_to_stream("%02x:%02x:%02x:%02x:%02x:%02x at %d.%d.%d.%d ",
                        eth_sha[0], eth_sha[1], eth_sha[2], eth_sha[3],
                        eth_sha[4], eth_sha[5], ip_spa[0], ip_spa[1], ip_spa[2],
                        ip_spa[3]);
    break;
  default:
    break;
  }

  return process_len;
}

bool Pcap::payload_process(size_t remain_len)
{
  if (remain_len == 0) {
    return true;
  }

  return true;
}

size_t Pcap::null_process(char* offset) { return 0; }

size_t Pcap::tcp_process(char* offset)
{
  struct tcphdr* tcph = reinterpret_cast<tcphdr*>(offset);
  size_t process_len = 0;
  offset += TCP_MINLEN;
  process_len += TCP_MINLEN;

  add_token_to_stream(
      "TCP %d %d %u %u %d %s%s%s%s%s%s %d %d %d ", ntohs(tcph->th_sport),
      ntohs(tcph->th_dport), ntohl(tcph->th_seq), ntohl(tcph->th_ack),
      TCP_HLEN(tcph->th_offx2) * 4, tcph->th_flags & TH_URG ? "U" : "",
      tcph->th_flags & TH_ACK ? "A" : "", tcph->th_flags & TH_PUSH ? "P" : "",
      tcph->th_flags & TH_RST ? "R" : "", tcph->th_flags & TH_SYN ? "S" : "",
      tcph->th_flags & TH_FIN ? "F" : "", ntohs(tcph->th_win),
      ntohs(tcph->th_sum), tcph->th_urp);

#if 0
  // TODO: Fix performance problem
  uint16_t service = static_cast<uint16_t>(
      min(min(ntohs(tcph->th_sport), ntohs(tcph->th_dport)),
        MAX_DEFINED_PORT_NUMBER));
  if (service < MAX_DEFINED_PORT_NUMBER)
    add_token_to_stream("%s ", TCP_PORT_SERV_DICT.find(service)->second);
#endif

  return process_len;
}

size_t Pcap::udp_process(char* offset)
{
  struct udphdr* udph = reinterpret_cast<udphdr*>(offset);
  size_t process_len = 0;
  offset += sizeof(struct udphdr);
  process_len += sizeof(struct udphdr);

  add_token_to_stream("UDP %d %d %d %d ", ntohs(udph->uh_sport),
                      ntohs(udph->uh_dport), ntohs(udph->uh_ulen),
                      ntohs(udph->uh_sum));

  return process_len;
}

size_t Pcap::icmp_process(char* offset)
{
  struct icmp* icmph = reinterpret_cast<icmp*>(offset);
  size_t process_len = 0;
  offset += ICMP_MINLEN;
  process_len += ICMP_MINLEN;

  if ((unsigned int)(icmph->icmp_type) == 11) {
    add_token_to_stream("ICMP %d %d %d %s ", icmph->icmp_type, icmph->icmp_code,
                        icmph->icmp_cksum, "ttl_expired");
  } else if ((unsigned int)(icmph->icmp_type) == ICMP_ECHOREPLY) {
    add_token_to_stream("ICMP %d %d %d %s ", icmph->icmp_type, icmph->icmp_code,
                        icmph->icmp_cksum, "echo_reply");
  }

  return process_len;
}

void Pcap::add_token_to_stream(const char* fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  int len = vsprintf(ptr, fmt, args);
  va_end(args);

  ptr += len;
  stream_length += len;
}

// vim: et:ts=2:sw=2
