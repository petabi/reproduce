#include <cstdio>
#include <cstring>
#include <ctime>
#include <functional>
#include <iomanip>
#include <stdexcept>

#include "ethernet.h"
#include "header2log.h"
#include "icmp.h"
#include "ipv4.h"
#include "rdkafka_producer.h"
#include "service.h"
#include "tcp.h"
#include "udp.h"
#include "util.h"

Pcap::Pcap(const std::string& filename)
{
  pcapfile = fopen(filename.c_str(), "r");
  if (pcapfile == nullptr)
    throw std::runtime_error("could not open [" + filename + "]");

  struct pcap_file_header pfh;
  if (fread(&pfh, sizeof(pfh), 1, pcapfile) < 1)
    throw std::runtime_error(filename + " is not an appropriate pcap file");
  if (pfh.magic != 0xa1b2c3d4) {
    throw std::runtime_error(filename + " is not an appropriate pcap file");
  }

  linktype = pfh.linktype;
}

Pcap::Pcap(Pcap&& other) noexcept : log_stream(std::move(other.log_stream))
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
  if (pcapfile != nullptr)
    fclose(pcapfile);
}

bool Pcap::skip_bytes(size_t size)
{
  size_t packet_len = 0;
  while (packet_len < size) {
    packet_len = pcap_header_process();
    if (packet_len == static_cast<size_t>(-1)) {
      return false;
    }
    if (!payload_process(packet_len))
      return false;
  }
  return true;
}

std::string Pcap::get_next_stream()
{
  size_t process_len = 0;
  size_t packet_len = 0;

  log_stream.str("");
  log_stream.clear();
  packet_len = pcap_header_process();
  if (packet_len == static_cast<size_t>(-1))
    return {};
  process_len = std::invoke(get_datalink_process(), this);
  if (process_len == static_cast<size_t>(-1))
    throw std::runtime_error("failed to read packet header");
  packet_len -= process_len;
  if (!payload_process(packet_len))
    throw std::runtime_error("failed to read packet payload");

  return log_stream.str();
}

size_t (Pcap::*Pcap::get_datalink_process())()
{
  switch (linktype) {
  case 1:
    log_stream << "Ethernet2 ";
    return &Pcap::ethernet_process;
    /*
      case 6:
        log_stream << " Token Ring ";
        break;
      case 10:
        log_stream << " FDDI ";
        break;
      case 0:
        log_stream << " Loopback ";
        break;
    */
  default:
    log_stream << " Unknown link type : " << linktype;
    break;
  }
  return &Pcap::null_process;
}

size_t (Pcap::*Pcap::get_internet_process(uint16_t ether_type))()
{
  switch (htons(ether_type)) {
  case ETHERTYPE_IP:
    log_stream << "IP  ";
    return &Pcap::ipv4_process;
  case ETHERTYPE_ARP:
    log_stream << "ARP   ";
    break;
    /*
      case ETHERTYPE_GRE_ISO:
        log_stream << "GRE_ISO  ";
            break;
      case ETHERTYPE_PUP:
        log_stream << "PUP  ";
            break;
      case ETHERTYPE_REVARP:
        log_stream << "REVARP   ";
            break;
      case ETHERTYPE_NS:
        log_stream << "NS  ";
            break;
      case ETHERTYPE_SPRITE:
        log_stream << "SPRITE  ";
            break;
      case ETHERTYPE_TRAIL:
        log_stream << "TRAIL  ";
            break;
      case ETHERTYPE_MOPDL:
        log_stream << "MOPDL  ";
            break;
      case ETHERTYPE_MOPRC:
        log_stream << "MOPRC  ";
        break;
      case ETHERTYPE_DN:
        log_stream << "DN  ";
        break;
      case ETHERTYPE_LAT:
        log_stream << "LAT  ";
        break;
      case ETHERTYPE_SCA:
        log_stream << "SCA  ";
        break;
      case ETHERTYPE_TEB:
        log_stream << "TEB  ";
        break;
      case ETHERTYPE_LANBRIDGE:
        log_stream << "LANBRIDGE  ";
        break;
      case ETHERTYPE_DECDNS:
        log_stream << "DECDNS  ";
        break;
      case ETHERTYPE_DECDTS:
        log_stream << "DECDTS  ";
        break;
      case ETHERTYPE_VEXP:
        log_stream << "VEXP  ";
        break;
      case ETHERTYPE_VPROD:
        log_stream << "VPROD  ";
        break;
      case ETHERTYPE_ATALK:
        log_stream << "ATALK  ";
        break;
      case ETHERTYPE_AARP:
        log_stream << "AARP  ";
        break;
      case ETHERTYPE_TIPC:
        log_stream << "TIPC  ";
        break;
      case ETHERTYPE_8021Q:
        log_stream << "802.1Q  ";
        break;
      case ETHERTYPE_8021Q9100:
        log_stream << "802.1Q-9100  ";
        break;
      case ETHERTYPE_8021Q9200:
        log_stream << "802.1Q-9200  ";
        break;
      case ETHERTYPE_8021QinQ:
        log_stream << "802.1Q-inQ  ";
        break;
      case ETHERTYPE_IPX:
        log_stream << "IPX  ";
        break;
      case ETHERTYPE_IPV6:
        log_stream << "IPV6  ";
        break;
      case ETHERTYPE_PPP:
        log_stream << "PPP  ";
        break;
      case ETHERTYPE_MPCP:
        log_stream << "MPCP  ";
        break;
      case ETHERTYPE_SLOW:
        log_stream << "SLOW  ";
        break;
      case ETHERTYPE_MPLS:
        log_stream << "MPLS  ";
        break;
      case ETHERTYPE_MPLS_MULTI:
        log_stream << "MPLS_MULTI  ";
        break;
      case ETHERTYPE_PPPOED:
        log_stream << "PPPOED  ";
        break;
      case ETHERTYPE_PPPOES:
        log_stream << "PPPOES  ";
        break;
      case ETHERTYPE_NSH:
        log_stream << "NSH  ";
        break;
      case ETHERTYPE_PPPOED2:
        log_stream << "PPPOED2  ";
        break;
      case ETHERTYPE_PPPOES2:
        log_stream << "PPPOES2  ";
        break;
      case ETHERTYPE_MS_NLB_HB:
        log_stream << "MS_NLB_HB  ";
        break;
      case ETHERTYPE_JUMBO:
        log_stream << "JUMBO  ";
        break;
      case ETHERTYPE_LLDP:
        log_stream << "LLDP  ";
        break;
      case ETHERTYPE_EAPOL:
        log_stream << "EAPOL  ";
        break;
      case ETHERTYPE_RRCP:
        log_stream << "RRCP  ";
        break;
      case ETHERTYPE_AOE:
        log_stream << "AOE  ";
        break;
      case ETHERTYPE_LOOPBACK:
        log_stream << "LOOPBACK  ";
        break;
      case ETHERTYPE_CFM_OLD:
        log_stream << "CFM_OLD  ";
        break;
      case ETHERTYPE_CFM:
        log_stream << "CFM  ";
        break;
      case ETHERTYPE_IEEE1905_1:
        log_stream << "IEEE1905_1  ";
        break;
      case ETHERTYPE_ISO:
        log_stream << "ISO  ";
        break;
      case ETHERTYPE_CALM_FAST:
        log_stream << "CALM_FAST  ";
        break;
      case ETHERTYPE_GEONET_OLD:
        log_stream << "GEONET_OLD  ";
        break;
      case ETHERTYPE_GEONET:
        log_stream << "GEONET  ";
        break;
      case ETHERTYPE_MEDSA:
        log_stream << "MEDSA  ";
        break;
    */
  default:
    log_stream << "UKNOWN_INTERNET_" << htons(ether_type);
    break;
  }
  return &Pcap::null_process;
}

size_t (Pcap::*Pcap::get_transport_process(uint8_t ip_p))()
{
  switch (ip_p) {
  case IPPROTO_ICMP:
    log_stream << "ICMP ";
    return &Pcap::icmp_process;
  case IPPROTO_TCP:
    log_stream << "TCP ";
    return &Pcap::tcp_process;
  case IPPROTO_UDP:
    log_stream << "UDP ";
    return &Pcap::udp_process;
    /*
    case IPPROTO_IP:
    log_stream << "IP ";
    break;
    case IPPROTO_IGMP:
    log_stream << "IGMP ";
    break;
    case IPPROTO_IPV4:
    log_stream << "IPV4 ";
    return &Pcap::null_process;
    case IPPROTO_EGP:
    log_stream << "EGP ";
    break;
    case IPPROTO_PIGP:
    log_stream << "PIGP ";
    break;
    case IPPROTO_DCCP:
    log_stream << "DCCP ";
    break;
    case IPPROTO_IPV6:
    log_stream << "IPV6 ";
    break;
    case IPPROTO_ROUTING:
    log_stream << "ROUTING ";
    break;
    case IPPROTO_FRAGMENT:
    log_stream << "FRAGMENT ";
    break;
    case IPPROTO_RSVP:
    log_stream << "RSVP ";
    break;
    case IPPROTO_GRE:
    log_stream << "GRE ";
    break;
    case IPPROTO_ESP:
    log_stream << "ESP ";
    break;
    case IPPROTO_AH:
    log_stream << "AH ";
    break;
    case IPPROTO_MOBILE:
    log_stream << "MOBILE ";
    break;
    case IPPROTO_ICMPV6:
    log_stream << "ICMPV6 ";
    break;
    case IPPROTO_NONE:
    log_stream << "NONE ";
    break;
    case IPPROTO_DSTOPTS:
    log_stream << "DSTOPTS ";
    break;
    case IPPROTO_MOBILITY_OLD:
    log_stream << "OLD";
    break;
    case IPPROTO_ND:
    log_stream << "ND ";
    break;
    case IPPROTO_EIGRP:
    log_stream << "EIGRP ";
    break;
    case IPPROTO_OSPF:
    log_stream << "OSPF ";
    break;
    case IPPROTO_PIM:
    log_stream << "PIM ";
    break;
    case IPPROTO_IPCOMP:
    log_stream << "IPCOMP ";
    break;
    case IPPROTO_VRRP:
    log_stream << "VRRP ";
    break;
    case IPPROTO_PGM:
    log_stream << "PGM ";
    break;
    case IPPROTO_SCTP:
    log_stream << "SCTP ";
    break;
    case IPPROTO_MOBILITY:
    log_stream << "MOBILITY ";
    break;
    */
  default:
    log_stream << "UKNOWN_TRANSPORT_" << ip_p;
    break;
  }
  return &Pcap::null_process;
}

size_t Pcap::pcap_header_process()
{
  // Pcap File Header
  struct pcap_pkthdr pp;
  char* cap_time = nullptr;
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
  process_len += std::invoke(get_internet_process(eh.ether_type), this);
  if (process_len == static_cast<size_t>(-1))
    throw std::runtime_error("failed to read internet header");
  return process_len;
}

size_t Pcap::ipv4_process()
{
  struct ip iph;
  size_t i = 0, opt = 0;
  size_t process_len = 0;

  if (fread(&iph, IP_MINLEN, 1, pcapfile) < 1)
    return -1;
  process_len += IP_MINLEN;
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
  opt = IP_HL(iph.ip_vhl) * 4 - sizeof(iph);
  if (opt != 0) {
    fseek(pcapfile, opt, SEEK_CUR);
    process_len += opt;
    log_stream << "ip_option ";
  }
  process_len += std::invoke(get_transport_process(iph.ip_p), this);
  if (process_len == static_cast<size_t>(-1))
    throw std::runtime_error("failed to read transport header");
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

size_t Pcap::null_process() { return 0; }

size_t Pcap::tcp_process()
{
  struct tcphdr tcph;
  size_t process_len = 0;
  if (fread(&tcph, TCP_MINLEN, 1, pcapfile) < 1)
    return -1;
  process_len += TCP_MINLEN;

  log_stream << " src_port_" << ntohs(tcph.th_sport);
  log_stream << " dst_port_" << ntohs(tcph.th_dport);
  log_stream << " seq_num_" << ntohl(tcph.th_seq);
  log_stream << " ack_num_" << ntohl(tcph.th_ack);
  log_stream << " h_len_" << TCP_HLEN(tcph.th_offx2) * 4;
  // log_stream << " cwr_" << (unsigned int)tcph.cwr;
  // log_stream << " ecn_" << (unsigned int)tcph.ece;
  log_stream << (tcph.th_flags & TH_URG ? 'U' : '\0');
  log_stream << (tcph.th_flags & TH_ACK ? 'A' : '\0');
  log_stream << (tcph.th_flags & TH_PUSH ? 'P' : '\0');
  log_stream << (tcph.th_flags & TH_RST ? 'R' : '\0');
  log_stream << (tcph.th_flags & TH_SYN ? 'S' : '\0');
  log_stream << (tcph.th_flags & TH_FIN ? 'F' : '\0');
  log_stream << " window_" << ntohs(tcph.th_win);
  log_stream << " checksum_" << ntohs(tcph.th_sum);
  log_stream << " urg_ptr_" << tcph.th_urp;

  uint16_t service = static_cast<uint16_t>(
      std::min(std::min(ntohs(tcph.th_sport), ntohs(tcph.th_dport)),
               MAX_DEFINED_PORT_NUMBER));
  if (service < MAX_DEFINED_PORT_NUMBER)
    log_stream << " " << TCP_PORT_SERV_DICT.find(service)->second;

  return process_len;
}

size_t Pcap::udp_process()
{
  struct udphdr udph;
  size_t process_len = 0;
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
  if (fread(&icmph, ICMP_MINLEN, 1, pcapfile) < 1)
    return -1;
  process_len += ICMP_MINLEN;
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
