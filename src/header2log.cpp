#include "header2log.h"
#include "rdkafka_producer.h"
#include <cstdio>
#include <cstring>
#include <ctime>
#include <iomanip>
#include <net/ethernet.h>
#include <netinet/if_ether.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>

bool Pcap::open_pcap(const std::string& filename)
{
  pcapfile = fopen(filename.c_str(), "r");
  if (pcapfile == NULL)
    return false;
  if (global_header_process() == false)
    return false;
  return true;
}

bool Pcap::skip_bytes(size_t size)
{
  size_t process_len = 0;
  size_t packet_len = 0;
  while (packet_len < size) {
    packet_len = pcap_header_process();
    if (packet_len == -1) {
      return false;
    }
    if (!payload_process(packet_len))
      return false;
  }
  return true;
}

bool Pcap::get_next_stream()
{
  size_t process_len = 0;
  size_t packet_len = 0;

  log_stream.str("");
  log_stream.clear();
  packet_len = pcap_header_process();
  if (packet_len == -1)
    return false;
  size_t (Pcap::*datalink_process)() = get_datalink_process();
  process_len = (this->*datalink_process)();
  if (process_len == -1)
    return false;
  packet_len -= process_len;
  if (!payload_process(packet_len))
    return false;

  return true;
}

bool Pcap::produce_to_rdkafka(const std::string& brokers,
                              const std::string& topic)
{
  Rdkafka_producer rp;
  if (rp.server_conf(brokers, topic) == false)
    return false;
  if (rp.produce(log_stream.str()) == false)
    return false;

  return true;
}

void Pcap::print_log_stream() { std::cout << log_stream.str() << '\n'; }

bool Pcap::global_header_process()
{
  struct pcap_file_header pfh;
  if (fread(&pfh, (int)sizeof(pfh), 1, pcapfile) < 1)
    return false;
  if (pfh.magic != 0xa1b2c3d4) {
    std::cout << "pcap is worng..\n";
    return false;
  }

  linktype = pfh.linktype;
  return true;
}

size_t (Pcap::*Pcap::get_datalink_process())()
{
  switch (linktype) {
  case 1:
    log_stream << "Ethernet2 ";
    return &Pcap::ethernet_process;
  case 6:
    log_stream << " Token Ring ";
    return NULL;
  case 10:
    log_stream << " FDDI ";
    return NULL;
  case 0:
    log_stream << " Loopback ";
    return NULL;
  default:
    log_stream << " Unknown link type : " << linktype;
    return NULL;
  }
}

size_t Pcap::pcap_header_process()
{
  // Pcap File Header
  struct pcap_pkthdr pp;
  char* cap_time = NULL;
  long sec;
  if (fread(&pp, sizeof(pp), 1, pcapfile) < 1)
    return -1;
  sec = (long)pp.ts.tv_sec;
  cap_time = (char*)ctime((const time_t*)&sec);
  cap_time[strlen(cap_time) - 1] = '\0';
  log_stream << pp.len << "bytes ";
  log_stream << cap_time << " ";
  // log_stream(".%06d ", pp.ts.tv_usec);

  return (size_t)pp.caplen;
}

size_t Pcap::ethernet_process()
{
  struct ether_header eh;
  size_t i = 0;
  size_t process_len = 0;

  if (fread(&eh, sizeof(eh), 1, pcapfile) < 1)
    return -1;
  process_len += sizeof(eh);

  for (i = 0; i < sizeof(eh.ether_dhost); i++) {
    if (i != 0)
      log_stream << ":";
    log_stream << std::setfill('0') << std::setw(2) << std::hex
               << static_cast<int>(eh.ether_dhost[i]);
  }
  log_stream << " ";
  for (i = 0; i < sizeof(eh.ether_dhost); i++) {
    if (i != 0)
      log_stream << ":";
    log_stream << std::setfill('0') << std::setw(2) << std::hex
               << static_cast<int>(eh.ether_shost[i]);
  }
  log_stream << " ";
  switch (htons(eh.ether_type)) {
  case ETHERTYPE_IP:
    log_stream << "IPv4 ";
    process_len += ipv4_process();
    break;

  case ETHERTYPE_ARP:
    log_stream << "ARP ";
    break;

  default:
    break;
  }
  return process_len;
}

size_t Pcap::ipv4_process()
{
  struct ip iph;
  int i = 0, opt = 0;
  int process_len = 0;

  if (fread(&iph, sizeof(iph), 1, pcapfile) < 1)
    return -1;
  process_len += sizeof(iph);
  for (i = 0; i < sizeof(iph.ip_src); i++) {
    if (i != 0)
      log_stream << ".";
    log_stream << std::dec
               << static_cast<int>(((unsigned char*)&iph.ip_src)[i]);
  }
  log_stream << " ";
  for (i = 0; i < sizeof(iph.ip_src); i++) {
    if (i != 0)
      log_stream << ".";
    log_stream << std::dec
               << static_cast<int>(((unsigned char*)&iph.ip_dst)[i]);
  }
  log_stream << " ";
  opt = iph.ip_hl * 4 - sizeof(iph);
  if (opt != 0) {
    fseek(pcapfile, opt, SEEK_CUR);
    process_len += opt;
    log_stream << "ip_option ";
  }

  switch (iph.ip_p) {
  case 1:
    // PrintIcmpPacket(Buffer,Size);
    log_stream << "icmp";
    process_len += icmp_process();
    break;

  case 2:
    log_stream << "igmp";
    break;

  case 6:
    log_stream << "tcp";
    process_len += tcp_process();
    break;

  case 17:
    log_stream << "udp";
    process_len += udp_process();
    break;

  default:
    break;
  }
  return process_len;
}

bool Pcap::payload_process(size_t remain_len)
{
  if (remain_len == 0)
    return true;
  char buffer[remain_len];
  if (fread(buffer, remain_len, 1, pcapfile) < 1)
    return false;

  return true;
}

size_t Pcap::tcp_process()
{
  struct tcphdr tcph;
  int process_len = 0;
  if (fread(&tcph, sizeof(tcph), 1, pcapfile) < 1)
    return -1;
  process_len += sizeof(tcph);

  log_stream << " src_port_" << ntohs(tcph.th_sport);
  log_stream << " dst_port_" << ntohs(tcph.th_dport);
  log_stream << " seq_num_" << ntohl(tcph.th_seq);
  log_stream << " ack_num_" << ntohl(tcph.th_ack);
  log_stream << " h_len_" << tcph.th_off * 4;
  // log_stream << " cwr_" << (unsigned int)tcph.cwr;
  // log_stream << " ecn_" << (unsigned int)tcph.ece;
  log_stream << (tcph.th_flags & TH_URG) ? 'U' : '\0';
  log_stream << (tcph.th_flags & TH_ACK) ? 'A' : '\0';
  log_stream << (tcph.th_flags & TH_PUSH) ? 'P' : '\0';
  log_stream << (tcph.th_flags & TH_RST) ? 'R' : '\0';
  log_stream << (tcph.th_flags & TH_SYN) ? 'S' : '\0';
  log_stream << (tcph.th_flags & TH_FIN) ? 'F' : '\0';
  log_stream << " window_" << ntohs(tcph.th_win);
  log_stream << " checksum_" << ntohs(tcph.th_sum);
  log_stream << " urg_ptr_" << tcph.th_urp;

  return process_len;
}

size_t Pcap::udp_process()
{
  struct udphdr udph;
  int process_len = 0;
  if (fread(&udph, sizeof(udph), 1, pcapfile) < 1)
    return -1;
  process_len += sizeof(udph);

  log_stream << " src_port_" << ntohs(udph.uh_sport);
  log_stream << " det_port_" << ntohs(udph.uh_dport);
  log_stream << " length_" << ntohs(udph.uh_ulen);
  // log_stream << " checksum_" << ntohs(udph.check);

  return process_len;
}

size_t Pcap::icmp_process()
{
  struct icmp icmph;
  size_t process_len = 0;
  if (fread(&icmph, sizeof(icmph), 1, pcapfile) < 1)
    return -1;
  process_len += sizeof(icmph);
  log_stream << " type_" << (unsigned int)(icmph.icmp_type);

  if ((unsigned int)(icmph.icmp_type) == 11)
    log_stream << " ttl_expired ";
  else if ((unsigned int)(icmph.icmp_type) == ICMP_ECHOREPLY)
    log_stream << " echo_reply ";
  log_stream << " code_" << (unsigned int)(icmph.icmp_code);
  // log_stream << " checksum_" << ntohs(icmph.checksum);
  // log_stream << " id_" << ntohs(icmph.id);
  // log_stream << " seq_" << ntohs(icmph.sequence));

  return process_len;
}
