#ifndef CONFIG_H
#define CONFIG_H

#include <cstddef>
#include <cstdint>

struct Config;

extern "C" {

Config* config_default();
Config* config_new(int, const char* const*);
void config_free(Config*);
void config_show(const Config*);
size_t config_count_sent(const Config*);
size_t config_count_skip(const Config*);
uint8_t config_datasource_id(const Config*);
double config_entropy_ratio(const Config*);
const char* config_file_prefix(const Config*);
uint32_t config_initial_seq_no(const Config*);
const char* config_input(const Config*);
void config_set_input(Config*, const char*);
int config_input_type(const Config*);
void config_set_input_type(Config*, int);
const char* config_kafka_broker(const Config*);
void config_set_kafka_broker(Config*, const char*);
const char* config_kafka_topic(const Config*);
void config_set_kafka_topic(Config*, const char*);
size_t config_mode_eval(const Config*);
size_t config_mode_grow(const Config*);
size_t config_mode_polling_dir(const Config*);
const char* config_offset_prefix(const Config*);
const char* config_output(const Config*);
void config_set_output(Config*, const char*);
int config_output_type(const Config*);
void config_set_output_type(Config*, int);
const char* config_packet_filter(const Config*);
const char* config_pattern_file(const Config*);
size_t config_queue_period(const Config*);
void config_set_queue_period(const Config*, size_t);
size_t config_queue_size(const Config*);
void config_set_queue_size(const Config*, size_t);

} // extern "C"

enum InputType { Pcap = 0, Pcapng = 1, Nic = 2, Log = 3, Dir = 4 };

enum OutputType { None = 0, Kafka = 1, File = 2 };

#endif

// vim: et:ts=2:sw=2
