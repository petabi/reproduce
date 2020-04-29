# Changelog

This file documents all notable changes to this project. The format of this file
is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/), and this
project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]


## [0.9.7] 2020-04-08

### Added

- Add '-j' option: user can set the initial event_id number. Without this option, event_id will be begin at 1 or skip_count+1.
- Add '-v' option: REproduce watches the input directory and sends it when new files are found.
- Instead of the name 'report.txt', use the Kafka topic name as the file name.

### Fixed

- The default value of `message.timeout.ms` is set to 5,000 ms, the default
  value of `linger.ms`. This allows to link REproduce against librdkafka>=1.0.

## [0.9.6] 2019-07-22

### Changed

- (test) For PCAP, this version wil send payload only rather than session + payload 2KB.
  And sessions.txt does not created.
- Produce success messages are displayed in every 100 success, i.e., around 100MB sent.


## [0.9.5] 2019-07-12

### Changed

- 'report.txt', 'session.txt' file name changed to `report.txt-YYYYMMDDHHMMSS` and `sessions.txt-YYYYMMDDHHMMSS`
- bug fixed: event_id for TCP, UDP, ICMP is still session number. it's fixed to send packet number.



## [0.9.4] 2019-07-10

### Added

- When REproduce send PCAP, it will save session information into `/report/sessions.txt` file.
  If the '/report' directory does not exist, REproduce will try to open in the current directory where REproduce is running in.
  The session information is appended at the end of the file.
  You should clear it before REproduce run if you want to get clean data.



## [0.9.3] 2019-07-08

### Changed
- The event_id for pcap changed to the number of packets read from that PCAP file.
  In previous version event_id was session number.
- `report.txt` file will be created in `/report/` directory if it is exist, like `/report/report.txt`.
  If not, REproduce will try to open in the current directory where REproduce is running in.
  If you want to run REproduce in Docker, you should bind the `/report` to see the report file from the host.
- Dockerfile changed to use g++-8



## [0.9.3] 2019-06-28

### Added

- `-d` specifies the datasource id. `Default datasource id = 1`
- `-n` specifies the `prefix` of data file name. This option is
 useful when the `-i` option is DIRECTORY and you want to send
 only the files that the name begin with the specified prefix.

### Changed

- When a DIRECTORY is specified for input, reproduce will send files in `alphabetical order`.
- When a DIRECTORY is specified for input, reproduce will send the files in it's `sub-directories`.
- The `event id` will start at 1 rather than 0.
- Current `Time` will be printed in the report file.
