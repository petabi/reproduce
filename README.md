# REproduce

## Overview

### Introduction

  REproduce is a program that reads raw packet values such as a pcap file, converts them into log-type streams through specific field values or characteristics, and outputs the conversion result to a file or to a kafka server.
Packet translation is up to the transport layer, and the protocols currently supported are Ethernet, IP, ARP, TCP, UDP, and ICMP. Also, logs and plain text files are converted to a new type of log stream by adding or removing information according to their attributes.

## Function Specification

* The program converts packets into log-type streams
* The program performs a conversion that converts the log to a new format, and removes unnecessary elements or adds features
* The program sends transformed streams via kafka platform

### 1. Data entry

Specify a single pcap file or network interface or plain text file such as log to be converted through the program's options.

### 2. Conversion

REproduce converts the incoming packet or pcap file, log to a stream format with space as delimiter, as in the following Conversion Format. The conversion of packet starts with the sec value in the time_t structure representing the timestamp, and then converts from the lower layer to the higher layer of the protocol.

#### Conversion Example

##### Packet
1531980829 Ethernet2 a4:7b:2c:1f:eb:61 40:61:86:82:e9:26 IP 4 5 0 56325 19069 64 127 7184 59.7.91.91 121.205.88.134 ip_opt TCP 3389 63044 1092178785 2869829243 20 AP 64032 5779 0

\[Seconds of Timestamp\] \[Protocol Name\] \[Destination MAC Address\] \[Source MAC Address\] \[Protocol Name\] \[Version\] \[IHL\] \[ToS\] \[Total Length\] \[Identification\] \[Fragment Offset\] \[TTL\] \[Header Checksum\] \[Source IP Address\] \[Destination IP Address\] \[Presence of option field\] \[Protocol name\] \[Source Port Address\] \[Destination Port Address\] \[Squence Number\] \[Acknowledge Number\] \[Hlen\] \[Flags(UAPRSF)\] \[Window Size\] \[Checksum\] \[Urgent Pointer\]

##### Log
20180906 e1000 enp0s3 N Link Up 1000Mbps Full Duplex Flow Control: RX

See more details in appendix.

### 3. Output

REproduce outputs the converted result in a form specified by the user(Stdout, File, Transmission to kafka server).

## Usage

### Program Usage

```> reproduce [OPTIONS]```

### OPTIONS

```
  -b: kafka broker list, [host1:port1,host2:port2,..] (default: localhost:9092)
  -c: send count
  -d: data source id (1~65535). (default: 1)
  -e: evaluation mode. output statistical result of transmission after job is terminated or stopped
  -E: entropy ratio. The amount of maximum entropy allowed for a
      session (0.0 < entropy ratio <= 1.0). Default is 0.9.
      Only relevant for network packets.
  -f: packet filter syntax when input is NIC
      (reference : https://www.tcpdump.org/manpages/pcap-filter.7.html)
  -g: follow the growing input file
  -h: help
  -i: input [PCAPFILE/LOGFILE/DIR/NIC]
      If no 'i' option is given, input is internal sample data
      If DIR is given, the g option is not supported.
  -k: kafka config file (Ex: kafka.conf)
      it overrides default kafka config to user kafka config
  -m: match [Pattern FILE]
  -n: prefix of file name to send multiple files or directory
  -o: output [TEXTFILE/none]
      If no 'o' option is given, output is kafka
  -p: queue period time. how much time keep queued data. (default: 3)
  -q: queue size. how many bytes send once to kafka. (default: 900000)
  -r: record [prefix of offset file]
      using this option will start the conversion after the previous
      conversion. The offset file name is managed by [input file]_[prefix].
      Except when the input is a NIC.
  -s: skip count
  -t: kafka topic (default: pcap)
      If the broker does not have a corresponding topic, the broker fails
      unless there is a setting that automatically creates the topic.
```

### Kafka Config

When transferring the converted result via kafka, various options can be set through the file specified with the 'k' option.
The configuration consists of two sections: global settings and topic settings. Each section consists of properties and values.
An example of a configuration file is following.

#### Kafka Config File Format

```
[global]
client.id=rdkafka
message.max.bytes=1000000
message.copy.max.bytes=65535

[topic]
request.required.acks=1
message.timeout.ms=300000
offset.store.method=broker
```

For a detailed description of the property, see the following URL:
https://github.com/edenhill/librdkafka/blob/master/CONFIGURATION.md

### Examples

* Convert pcap file and send it to kafka server:
    * ```reproduce -i test.pcap -b 192.168.10.1:9092 -t sample_topic```
* Convert log file and send it to kafka server:
    * ```reproduce -i LOG_20180906 -b 192.168.10.1:9092 -t sample_topic```
* Save result file after converting pcap file:
    * ```reproduce -i test.pcap -o result.txt```
* Skip 10000 packets and convert 1000 packets in pcap file and evaluate performance:
    * ```reproduce -i test.pcap -s 10000 -c 1000 -o none -e```
* Convert it while following, If the content of the input file continue to grow
    * ```reproduce -i test.pcap -g```
* Convert only udp packets of traffic to and from network interface enp0s3
    * ```reproduce -i enp0s3 -f "udp" -o none```
* When transmitting to kafka once, queue up to 10Kbytes, and if transmission interval is delayed more than 2 seconds, send immediately
    * ```reproduce -i test.pcap -q 10240 -p 2```
* Send all log files beginning with 'msg' prefix in the '/data/LOG' directory and it's subdirectory
    * ```reproduce -i /data/LOG -n msg -b 192.168.4.5:9092 -t syslog -e```

### Report Example

REproduce creates or opens ```/report/report.txt-YYYYMMDDHHMMSS``` first.
If it failed, it will try to open ```./report.txt-YYYYMMDDHHMMSS```.


```
root@bada-unbuntu:~/REproduce# ./REproduce -i test.pcap -e -c 10000000
root@bada-unbuntu:~/REproduce# tail -f /report/report-20190712143111.txt
--------------------------------------------------
Time:                       Mon Jul  1 14:34:23 2019
Input(PCAP):                test.pcap(976.56M)
Datasource ID:              1
Input ID:                   1 ~ 10000000
Output(KAFKA):              localhost:9092(pcap)
Statistics(Min/Max/Avg):    60/4428/121.97bytes
Process Count:              6905184(803.185116MB)
Skip Count:                 0(0B)
Elapsed Time:               23.86s
Performance:                70.22MBps/419.14Kpps
```

### Sessions Information Example

* 0.9.6 version doesn't create or update sessions.txt and only send payload data.

REproduce creates ```/report/sessions.txt-YYYYMMDDHHMMSS``` or ```./sessions.txt-YYYYMMDDHHMMSS```.

* Fields: event_id, sip, dip, proto, sport, dport
* You can get the packet number by removing the upper 16 bits (datasource_id) from the event_id
* The report and session files share the 'YYYYMMDDHHMMSS' value, and the value means that REproduce process or docker launch time.

```
root@bada-unbuntu:~/REproduce# tail -f /report/sessions-20190712143111.txt
281474976840922,316206727,3530930811,6,443,64015
281474976840923,2540037634,3530917189,6,80,63841
281474976840965,3547547186,3422459274,6,5001,63819
281474976840983,1890545971,3530923615,6,14811,10681
281474976841013,2877264388,3530918422,6,51305,5500
281474976841017,1749831047,3530917189,6,80,63848
281474976841024,3551661079,3422459095,1,0,0
281474976841027,1360408062,3530921781,6,53345,51000
```



## Performance

### Test environment

* CPU : Intel(R) Xeon(R) CPU E5-2620 v4 @ 2.10GHz
* Memory : 64GB
* Cores(Utilization) : 1(100%)

### Result

| Contents                               | Speed                    |
|:---------------------------------------|:-------------------------|
| Packet Conversion Only                 | 80.07MBps / 477.94Kpps   |
| Kafka Transmission Only                | 770.97MBps / 4619.52Kpps |
| Packet Conversion + Kafka Transmission | 71.63MBps / 427.58Kpps   |

## Issue

## To do

* Support More protocols
* Define the conversion of log and Implement it

## Appendix

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

#### Building Docker Images
try this in a directory that has a Dockerfile:
```
docker build -t reproduce .
```
#### Running Docker Images
Run with the following command
```
docker run --mount type=bind,source=[the directory containing the target],target=/data \
           --mount type=bind,source=[report or log directory],target=/report reproduce:latest -i [the target file] -b localhost:9092 -t topic
```
