#include <unistd.h>

#include "config.h"
#include "version.h"

extern "C" {

Config* config_new(int, const char* const*);
void config_free(Config*);

} // extern "C"

using namespace std;

static constexpr size_t default_queue_period = 3;
static constexpr uint8_t default_datasource_id = 1;
static constexpr size_t default_queue_size = 900000;

auto Config::set(int argc, char** argv) -> bool
{
  int o;

  while ((o = getopt(argc, argv, "b:c:d:eE:f:ghi:j:k:m:n:o:p:q:r:s:t:vV")) !=
         -1) {
    switch (o) {
    case 'b':
      kafka_broker = optarg;
      break;
    case 'c':
      count_send = strtoul(optarg, nullptr, 0);
      break;
    case 'd':
      datasource_id = static_cast<uint8_t>(stoul(optarg, nullptr, 0));
      if (datasource_id == 0) {
        cerr << "Warning: datasource id is out of range (1 ~ 255). Defaulting "
                "to 1\n";
        datasource_id = default_datasource_id;
      }
      break;
    case 'E':
      entropy_ratio = strtof(optarg, nullptr);
      if (entropy_ratio <= 0.0 || entropy_ratio > 1.0) {
        cerr << "Warning: Entropy ratio is out of range. Defaulting to 0.9.\n";
        entropy_ratio = 0.9;
      }
      break;
    case 'e':
      mode_eval = true;
      break;
    case 'f':
      packet_filter = optarg;
      break;
    case 'g':
      mode_grow = true;
      break;
    case 'h': {
      const char* args[] = {"reproduce", "-h"};
      Config* conf = config_new(2, args);
      config_free(conf);
    }
      return false;
    case 'i':
      input = optarg;
      break;
    case 'j':
      initial_seq_no = static_cast<uint32_t>(stoul(optarg, nullptr, 0));
      break;
    case 'k':
      kafka_conf = optarg;
      break;
    case 'm':
      pattern_file = optarg;
      break;
    case 'n':
      file_prefix = optarg;
      break;
    case 'o':
      output = optarg;
      break;
    case 'p':
      queue_period = strtoul(optarg, nullptr, 0);
      break;
    case 'q':
      queue_size = strtoul(optarg, nullptr, 0);
      break;
    case 'r':
      offset_prefix = optarg;
      break;
    case 's':
      count_skip = strtoul(optarg, nullptr, 0);
      break;
    case 't':
      kafka_topic = optarg;
      break;
    case 'v':
      mode_polling_dir = true;
      break;
    case 'V': {
      const char* args[] = {"reproduce", "-V"};
      Config* conf = config_new(2, args);
      config_free(conf);
    }
    default:
      break;
    }
  }

  set_default();
  show();
  check();

  return true;
}

void Config::set_default() noexcept
{
  Util::dprint(F, "set default config");
  if (queue_size <= 0 || queue_size > default_queue_size) {
    queue_size = default_queue_size;
  }
  if (queue_period == 0) {
    queue_period = default_queue_period;
  }
}

void Config::check() const
{
  if (output.empty() && kafka_broker.empty()) {
    throw runtime_error("You must specify kafka broker list(-b)");
  }
  if (output.empty() && kafka_topic.empty()) {
    throw runtime_error("You must specify kafka topic(-t)");
  }
  if (input.empty() && output == "none") {
    throw runtime_error("You must specify input(-i) or output(-o) is not none");
  }

  // and so on...
}

void Config::show() const noexcept
{
  Util::dprint(F, "mode_eval=", mode_eval);
  Util::dprint(F, "mode_grow=", mode_grow);
  Util::dprint(F, "count_send=", count_send);
  Util::dprint(F, "count_skip=", count_skip);
  Util::dprint(F, "queue_size=", queue_size);
  Util::dprint(F, "queue_period=", queue_period);
  Util::dprint(F, "input=", input);
  Util::dprint(F, "output=", output);
  Util::dprint(F, "offset_prefix=", offset_prefix);
  Util::dprint(F, "packet_filter=", packet_filter);
  Util::dprint(F, "kafka_broker=", kafka_broker);
  Util::dprint(F, "kafka_topic=", kafka_topic);
  Util::dprint(F, "kafka_conf=", kafka_conf);
  Util::dprint(F, "file_prefix=", file_prefix);
  Util::dprint(F, "datasource_id=", (int)datasource_id);
}

// vim: et:ts=2:sw=2
