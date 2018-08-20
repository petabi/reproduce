#Header2log
=============================================

##Ⅰ. Overview
----------------------------------------------------
  * Header2log is a program that reads raw packet values such as a pcap file, converts them into log-type streams through specific field values or characteristics, and outputs the conversion result to a file or to a kafka server.
Packet translation is up to the transport layer, and the protocols currently supported are Ethernet, IP, ARP, TCP, UDP, and ICMP.

##Ⅱ. Usage
----------------------------------------------------
### Program Usage  
>header2log [OPTIONS]
### OPTIONS
* b: kafka broker (default: localhost:9092)
* c: send packet count
* d: debug mode (print debug messages)
* e: evaluation mode (report statistics)
* f: tcpdump filter
* h: help
* i: input pcapfile or nic
* k: do not send data to kafka
* o: output file
* p: do not parse packet (send hardcoded sample data. with -c option)
* q: queue packet count (how many packet send once)
* s: skip packet count
* t: kafka topic (default: pcap)
### Examples
* Convert pcap file and send it to kafka server : ```./header2log -i [pcap file name] -b [kafka broker addr:port] -t [kafka topic] ```
* Output only debugging messages (conversion result) after converting pcap file : ```./header2log -i [pcap file name] -d -k```
* Save result file after converting pcap file : ```./header2log -i [pcap file name] -o [output file]```
* Skip packets and convert pcap file : ```./header2log -i [pcap file name] -s [skip packet count]```

##Ⅲ. Performance
-----------------------------------------------------
###Test environment
* CPU : Intel(R) Xeon(R) CPU E5-2620 v4 @ 2.10GHz
* Memory : Intel(R) Xeon(R) CPU E5-2620 v4 @ 2.10GHz

###Result
| Contents | Speed |
|:-----------------------|:-----------------------|
| Packet Conversion Only | 68.75MBps / 410.39Kpps |
| Kafka Transmission Only | 270.82MBps / 1622.70Kpps |
| Packet Conversion + Kafka Transmission | 62.47MBps / 372.89Kpps |


##Ⅳ. Issue
-----------------------------------------------------


##Ⅴ. To do
-----------------------------------------------------
* Real-time conversion of sending and receiving packets in network interface (related option: i)
* Add packet filtering function
* Support More protocols 