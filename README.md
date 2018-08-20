#Header2log
=============================================

##I. Overview
-----------
* Header2log는 pcap 파일과 같은 원시 패킷 값을 사람이 이해하기 쉬우면서 특징을 잘 담아내는 로그 형태의 스트림으로 변환하고, 변환 결과를 파일로 출력하거나 kafka 서버로 전송하는 프로그램이다. 
패킷의 변환 범위는 Transport 계층까지이며, 현재 지원하는 프로토콜은 Ethernet, IP, ARP, TCP, UDP, ICMP이다.


###성능

|                   CPU                  | Intel(R) Xeon(R) CPU E5-2620 v4 @ 2.10GHz |                        |
|----------------------------------------|-------------------------------------------|------------------------|
|                 Memory                 | 62GB                                      |                        |
|             Input Pcap File            | test.pcap (976.56M)                       |                        |
|         Packet Conversion Only         | Send Packets                              | 10000000(10.00M)       |
|                                        | Send Bytes                                | 1756644346(1675.27M)   |
|                                        | Elapsed Time                              | 24.37s                 |
|                                        | Performance                               | 68.75MBps/410.39Kpps   |
|         Kafka Transmission Only        | Send Packets                              | 10000000(10.00M)       |
|                                        | Send Bytes                                | 1750000000(1668.93M)   |
|                                        | Elapsed Time                              | 6.16s                  |
|                                        | Performance                               | 270.82MBps/1622.70Kpps |
| Packet Conversion + Kafka Transmission | Send Packets                              | 10000000(10.00M)       |
|                                        | Send Bytes                                | 1756644346(1675.27M)   |
|                                        | Elapsed Time                              | 26.82s                 |
|                                        | Performance                               | 62.47MBps/372.89Kpps   |

##II. Caveats and Known Issues
----------------------------------------------------
* 현재, 데이터 입력은 pcap 파일만 지원하며 추후 텍스트 파일 및 인터페이스를 통한 입력을 지원할 예정이다.

##IV. API
-----------------------------------------------------
* Input Packet Data : a Pcap File
* Output Stream Data : Text File or Transmission to kafka server


##V. Usage
-----------------------------------------------------
```
[USAGE] header2log OPTIONS
  -b: kafka broker (default: localhost:9092)
  -c: send packet count
  -d: debug mode (print debug messages)
  -e: evaluation mode (report statistics)
  -f: tcpdump filter
  -h: help
  -i: input pcapfile or nic
  -k: do not send data to kafka
  -o: output file
  -p: do not parse packet (send hardcoded sample data. with -c option)
  -q: queue packet count (how many packet send once)
  -s: skip packet count
  -t: kafka topic (default: pcap)
```
*