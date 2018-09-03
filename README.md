# Packetproducer

## Overview

### Introduction

  Packetproducer is a program that reads raw packet values such as a pcap file, converts them into log-type streams through specific field values or characteristics, and outputs the conversion result to a file or to a kafka server.
Packet translation is up to the transport layer, and the protocols currently supported are Ethernet, IP, ARP, TCP, UDP, and ICMP.

## Function Specification

* The program converts packets into log-type streams
* The program performs a conversion that converts the log to a new format, and removes unnecessary elements or adds features
* The program sends transformed streams via kafka platform

### 1. Data entry

Specify a single pcap file or network interface to be converted through the program's options.

### 2. Conversion

Packetproducer Converts the incoming packet to a stream format with space as delimiter, as in the following Conversion Format. The conversion starts with the sec value in the time_t structure representing the timestamp, and then converts from the lower layer to the higher layer of the protocol.

#### Conversion Example

1531980829 Ethernet2 a4:7b:2c:1f:eb:61 40:61:86:82:e9:26 IP 4 5 0 56325 19069 64 127 7184 59.7.91.91 121.205.88.134 ip_opt TCP 3389 63044 1092178785 2869829243 20 AP 64032 5779 0

\[Seconds of Timestamp\] \[Protocol Name\] \[Destination MAC Address\] \[Source MAC Address\] \[Protocol Name\] \[Version\] \[IHL\] \[ToS\] \[Total Length\] \[Identification\] \[Fragment Offset\] \[TTL\] \[Header Checksum\] \[Source IP Address\] \[Destination IP Address\] \[Presence of option field\] \[Protocol name\] \[Source Port Address\] \[Destination Port Address\] \[Squence Number\] \[Acknowledge Number\] \[Hlen\] \[Flags(UAPRSF)\] \[Window Size\] \[Checksum\] \[Urgent Pointer\]

### 3. Output

Packetproducer outputs the converted result in a form specified by the user(Stdout, File, Transmission to kafka server).

### Conversion Format

#### Ethernet

| Order |     Type    |       description       |      example      |
|:-----:|:-----------:|:------------------------|:------------------|
|   1   |     Text    | Protocol Name           | Ethernet2         |
|   2   | MAC Address | Destination MAC Address | 40:61:86:82:e9:26 |
|   3   | MAC Address | Source MAC Address      | a4:7b:2c:1f:eb:61 |

#### IP

| Order |    Type    |       description         |     example     |
|:-----:|:----------:|:--------------------------|:----------------|
|   1   |    Text    | Protocol Name             | IP              |
|   2   |   Decimal  | Version                   | 4               |
|   3   |   Decimal  | IHL                       | 5               |
|   4   |   Decimal  | ToS                       | 0               |
|   5   |   Decimal  | Total Length              | 10240           |
|   6   |   Decimal  | Identification            | 29865           |
|   7   |   Decimal  | Fragment Offset           | 64              |
|   8   |   Decimal  | TTL                       | 54              |
|   9   |   Decimal  | Header Checksum           | 49311           |
|   10  | IP Address | Source IP Address         | 125.209.230.110 |
|   11  | IP Address | Destination IP Address    | 59.7.91.240     |
|   12  |    Text    | Presence of option field  | ip_opt          |

#### ARP

| Order |                        Type                        |                 description                  |                     example                     |
|:-----:|:--------------------------------------------------:|:---------------------------------------------|:------------------------------------------------|
|   1   |                        Text                        | Protocol name                                | ARP                                             |
|   2   |                        Text                        | Keyword To Protocol Type Field               | Request                                         |
|       |                                                    |   * Request                                  |                                                 |
|       |                                                    |   * Reply                                    |                                                 |
|       |                                                    |   * Reverse Request                          |                                                 |
|       |                                                    |   * Reverse Reply                            |                                                 |
|       |                                                    |   * Inverse Request                          |                                                 |
|       |                                                    |   * Inverse Reply                            |                                                 |
|       |                                                    |   * NACK Reply                               |                                                 |
|   3   |                        Text                        | Keyword To Hardware Type Field               | Ethernet                                        |
|       |                                                    |   * Ethernet                                 |                                                 |
|       |                                                    |   * TokenRing                                |                                                 |
|       |                                                    |   * ArcNet                                   |                                                 |
|       |                                                    |   * FrameRelay                               |                                                 |
|       |                                                    |   * Strip                                    |                                                 |
|       |                                                    |   * IEEE 1394                                |                                                 |
|       |                                                    |   * ATM                                      |                                                 |
|   4   | According to the OP Code, it is divided as follows |                                              |                                                 |
|  4-a  |       [Text] [IP Address] [Text] [IP Address]      | OP Code : 1(ARP Request)                     | who-has 192.168.0.254 tell 192.168.0.1          |
|  4-b  |          [IP Address] [Text] [MAC Address]         | OP Code : 2(ARP Reply)                       | 192.168.0.254 is-at a4:7b:2c:3f:eb:24           |
|  4-c  |      [Text] [MAC Address] [Text] [MAC Address]     | OP Code : 3(RARP Request)                    | who-is 192.168.0.254 tell 192.168.0.1           |
|  4-d  |          [MAC Address] [Text] [IP Address]         | OP Code : 4(RARP Reply)                      | a4:7b:2c:3f:eb:24 at 192.168.0.254              |
|  4-e  |       [Text] [IP Address] [Text] [IP Address]      | OP Code : 8(InARP Request)                   | who-is 40:61:86:82:e9:26 tell a4:7b:2c:3f:eb:24 |
|  4-f  |          [MAC Address] [Text] [IP Address]         | OP Code : 9(InARP Reply)                     | a4:7b:2c:3f:eb:24 at 192.168.0.254              |

### ICMP

| Order |   Type  |         description        |   example   |
|:-----:|:-------:|:---------------------------|:------------|
|   1   |   Text  | Protocol name              | ICMP        |
|   2   | Decimal | Type                       | 8           |
|   3   | Decimal | Code                       | 0           |
|   4   | Decimal | Checksum                   | 1048        |
|   5   |   Text  | Keyword To ICMP Type Field | ttl_expired |
|       |         |   * ttl_expired            |             |
|       |         |   * echo_reply             |             |

### TCP

| Order |   Type  |        description       |   example  |
|:-----:|:-------:|:-------------------------|:-----------|
|   1   |   Text  | Protocol name            | TCP        |
|   2   | Decimal | Source Port Address      | 16493      |
|   3   | Decimal | Destination Port Address | 80         |
|   4   | Decimal | Squence Number           | 2622175950 |
|   5   | Decimal | Acknowledge Number       | 416662581  |
|   6   | Decimal | Hlen                     | 20         |
|   7   |   Text  | Flags(UAPRSF)            | AS         |
|   8   | Decimal | Window Size              | 134        |
|   9   | Decimal | Checksum                 | 32214      |
|   10  | Decimal | Urgent Pointer           | 0          |

### UDP

| Order |   Type  |        description       | example |
|:-----:|:-------:|:-------------------------|:--------|
|   1   |   Text  | Protocol name            | UDP     |
|   2   | Decimal | Source Port Address      | 15948   |
|   3   | Decimal | Destination Port Address | 53      |
|   4   | Decimal | Length                   | 1048    |
|   5   | Decimal | Checksum                 | 30584   |

## Usage

### Program Usage  

```./Packetproducer [OPTIONS]```

### OPTIONS

```
  -b: kafka broker (default: localhost:9092)
  -c: send count
  -d: debug mode. print debug messages
  -e: evaluation mode. report statistics
  -f: tcpdump filter (when input is PCAP or NIC)
  -h: help
  -i: input [PCAPFILE/LOGFILE/NIC]
      If no 'i' option is given, sample data is converted
  -o: output [TEXTFILE/none]
      If no 'o' input is given, it will be sent via kafka
  -q: queue size in byte. how many bytes send once
  -s: skip count
  -t: kafka topic (default: pcap)
```

### Examples

* Convert pcap file and send it to kafka server:
    * ```./packetproducer -i test.pcap -o kafka -b 192.168.10.1:9092 -t sample_topic```
* Output only debugging messages (conversion result) after converting pcap file:
    * ```./packetproducer -i test.pcap -o none -d```
* Save result file after converting pcap file:
    * ```./packetproducer -i test.pcap -o result.txt```
* Skip 10000 packets and convert 1000 packets in pcap file and evaluate performance:
    * ```./packetproducer -i test.pcap -s 10000 -c 1000 -o none -e```

## Performance

###Test environment

* CPU : Intel(R) Xeon(R) CPU E5-2620 v4 @ 2.10GHz
* Memory : 64GB
* Cores(Utilization) : 1(100%)

###Result

| Contents | Speed |
|:-----------------------|:-----------------------|
| Packet Conversion Only | 68.75MBps / 410.39Kpps |
| Kafka Transmission Only | 270.82MBps / 1622.70Kpps |
| Packet Conversion + Kafka Transmission | 62.47MBps / 372.89Kpps |

## Issue

## To do

* Real-time conversion of sending and receiving packets in network interface (related option: i)
* Add packet filtering function
* Support More protocols
