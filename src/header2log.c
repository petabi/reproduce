#include "header2log.h"
#include <net/ethernet.h>
#include <netinet/ether.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

unsigned int processGlobalHeader(FILE* pcapfile, FILE* logfile);
unsigned int processPcapHeader(FILE* pcapfile, FILE* logfile);
int processEthernet(FILE* pcapfile, FILE* logfile);
int processIpv4(FILE* pcapfile, FILE* logfile);
int processIcmp(FILE* pcapfile, FILE* logfile);
int processUdp(FILE* pcapfile, FILE* logfile);
int processTcp(FILE* pcapfile, FILE* logfile);
int processPayload(FILE* pcapfile, FILE* logfile, int remain_size);

int main(int argc, char** argv)
{
  if (argc < 2)
    return 0;
  FILE* pcapfile = fopen(argv[1], "r");
  FILE* logfile = fopen("log.txt", "w");
  unsigned int linktype = 0;
  unsigned int packet_size = 0;
  int read_size = 0;
  int (*processDatalink)(FILE * pcapfile, FILE * logfile);

  if (pcapfile == NULL || logfile == NULL) {
    return -1;
  }
  printf("Starting...\n");
  linktype = processGlobalHeader(pcapfile, logfile);
  switch (linktype) {
  case 1:
    printf("Ethernet2\n");
    processDatalink = processEthernet;
    break;
  case 6:
    printf("Token Ring\n");
    return 0;
  case 10:
    printf("FDDI\n");
    return 0;
  case 0:
    printf("Loopback\n");
    return 0;
  default:
    printf("Unknown\n");
    return 0;
  }
  while (!feof(pcapfile)) {
    packet_size = processPcapHeader(pcapfile, logfile);
    read_size = (int)(processDatalink(pcapfile, logfile));
    processPayload(pcapfile, logfile, packet_size - read_size);
    printf("\n");
  }
  fclose(pcapfile);
  fclose(logfile);
  printf("Finished");
  return 0;
}

unsigned int processGlobalHeader(FILE* pcapfile, FILE* logfile)
{
  struct pcap_file_header pfh;
  if (fread(&pfh, (int)sizeof(pfh), 1, pcapfile) < 1)
    return -1;
  if (pfh.magic != 0xa1b2c3d4) {
    printf("Pcap is worng..");
    return -1;
  }

  return pfh.linktype;
}

unsigned int processPcapHeader(FILE* pcapfile, FILE* logfile)
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
  printf("%ulength ", pp.len);
  printf("%s", cap_time);
  // printf(".%06d ", pp.ts.tv_usec);

  return pp.caplen;
}

int processEthernet(FILE* pcapfile, FILE* logfile)
{
  struct ether_header eh;
  int i = 0;
  int read_size = 0;

  if (fread(&eh, sizeof(eh), 1, pcapfile) < 1)
    return -1;
  read_size += sizeof(eh);

  for (i = 0; i < sizeof(ETH_ALEN); i++) {
    if (i != 0)
      printf(":");
    printf("%02x", eh.ether_dhost[i]);
  }
  printf(" ");
  for (i = 0; i < sizeof(ETH_ALEN); i++) {
    if (i != 0)
      printf(":");
    printf("%02x", eh.ether_shost[i]);
  }
  printf(" ");
  switch (htons(eh.ether_type)) {
  case ETHERTYPE_IP:
    printf("IPv4 ");
    read_size += processIpv4(pcapfile, logfile);
    break;

  case ETHERTYPE_ARP:
    printf("ARP ");
    break;

  default:
    break;
  }
  return read_size;
}

int processIpv4(FILE* pcapfile, FILE* logfile)
{
  struct iphdr iph;
  int i = 0, opt = 0;
  int read_size = 0;

  if (fread(&iph, sizeof(iph), 1, pcapfile) < 1)
    return -1;
  read_size += sizeof(iph);
  for (i = 0; i < sizeof(iph.saddr); i++) {
    if (i != 0)
      printf(".");
    printf("%d", ((unsigned char*)&iph.saddr)[i]);
  }
  printf(" ");
  for (i = 0; i < sizeof(iph.saddr); i++) {
    if (i != 0)
      printf(".");
    printf("%d", ((unsigned char*)&iph.daddr)[i]);
  }
  printf(" ");
  opt = iph.ihl * 4 - sizeof(iph);
  if (opt != 0) {
    fseek(pcapfile, opt, SEEK_CUR);
    read_size += opt;
    printf("ip_option ");
  }

  switch (iph.protocol) {
  case 1:
    // PrintIcmpPacket(Buffer,Size);
    printf("icmp");
    read_size += processIcmp(pcapfile, logfile);
    break;

  case 2:
    printf("igmp");
    break;

  case 6:
    printf("tcp");
    read_size += processTcp(pcapfile, logfile);
    break;

  case 17:
    printf("udp");
    read_size += processUdp(pcapfile, logfile);
    break;

  default:
    break;
  }
  return read_size;
}

int processPayload(FILE* pcapfile, FILE* logfile, int remain_size)
{
  char buffer[remain_size];
  if (fread(buffer, remain_size, 1, pcapfile) < 1)
    return -1;

  return 0;
}

int processTcp(FILE* pcapfile, FILE* logfile)
{
  struct tcphdr tcph;
  int read_size = 0;
  if (fread(&tcph, sizeof(tcph), 1, pcapfile) < 1)
    return -1;
  read_size += sizeof(tcph);

  printf(" src_port_%u ", ntohs(tcph.source));
  printf(" dst_port_%u ", ntohs(tcph.dest));
  printf(" seq_num_%u ", ntohl(tcph.seq));
  printf(" ack_num_%u ", ntohl(tcph.ack_seq));
  printf(" h_len_%d ", (unsigned int)tcph.doff * 4);
  // printf(" cwr_%d ", (unsigned int)tcph.cwr);
  // printf(" ecn %d ", (unsigned int)tcph.ece);
  printf("%c", (tcph.urg) ? 'U' : '\0');
  printf("%c", (tcph.ack) ? 'A' : '\0');
  printf("%c", (tcph.psh) ? 'P' : '\0');
  printf("%c", (tcph.rst) ? 'R' : '\0');
  printf("%c", (tcph.syn) ? 'S' : '\0');
  printf("%c", (tcph.fin) ? 'F' : '\0');
  printf(" window_%d ", ntohs(tcph.window));
  printf(" checksum_%d ", ntohs(tcph.check));
  printf(" urg_ptr_%d ", tcph.urg_ptr);

  return read_size;
}

int processUdp(FILE* pcapfile, FILE* logfile)
{
  struct udphdr udph;
  int read_size = 0;
  if (fread(&udph, sizeof(udph), 1, pcapfile) < 1)
    return -1;
  read_size += sizeof(udph);

  printf(" src_port_%d ", ntohs(udph.source));
  printf(" det_port_%d ", ntohs(udph.dest));
  printf(" length_%d ", ntohs(udph.len));
  // printf(" checksum_%d " , ntohs(udph.check));

  return read_size;
}

int processIcmp(FILE* pcapfile, FILE* logfile)
{
  struct icmphdr icmph;
  int read_size = 0;
  if (fread(&icmph, sizeof(icmph), 1, pcapfile) < 1)
    return -1;
  read_size += sizeof(icmph);
  printf(" type_%d ", (unsigned int)(icmph.type));

  if ((unsigned int)(icmph.type) == 11)
    printf(" ttl_expired ");
  else if ((unsigned int)(icmph.type) == ICMP_ECHOREPLY)
    printf(" echo_reply ");
  printf(" code_%d ", (unsigned int)(icmph.code));
  // printf(" checksum_%d ",ntohs(icmph.checksum));
  // printf(" id_%d ",ntohs(icmph.id));
  // printf(" seq_%d ",ntohs(icmph.sequence));

  return read_size;
}
