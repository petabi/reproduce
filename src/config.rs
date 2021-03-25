use clap::{App, Arg};
use std::ffi::CString;
use std::fmt;
use std::num::{NonZeroU8, NonZeroUsize};

pub struct Config {
    // user
    pub mode_eval: bool,        // report statistics
    pub mode_grow: bool,        // convert while tracking the growing file
    pub mode_polling_dir: bool, // polling the input directory
    pub count_skip: usize,      // count to skip
    pub queue_size: usize,      // how many bytes sent at once
    pub queue_period: usize,    // how much time queued data is kept for
    pub input: CString,         // input: packet/log/none
    pub output: CString,        // output: kafka/file/none
    pub offset_prefix: CString, // prefix of offset file to read from and write to
    pub packet_filter: CString,
    pub kafka_broker: CString,
    pub kafka_topic: CString,
    pub kafka_conf: CString,
    pub pattern_file: CString,
    pub file_prefix: CString, // file name prefix when sending multiple files or a directory

    pub datasource_id: u8,
    pub initial_seq_no: u32,

    pub entropy_ratio: f64,

    // internal
    pub count_sent: usize,
    pub input_type: InputType,
    pub output_type: OutputType,
}

pub enum InputType {
    Pcap,
    PcapNg,
    Nic,
    Log,
    Dir,
}

pub enum OutputType {
    None,
    Kafka,
    File,
}

impl Default for Config {
    fn default() -> Self {
        Config {
            mode_eval: false,
            mode_grow: false,
            mode_polling_dir: false,
            count_skip: 0,
            queue_size: 900_000,
            queue_period: 3,
            input: CString::new(Vec::new()).expect("no 0 byte"),
            output: CString::new(Vec::new()).expect("no 0 byte"),
            offset_prefix: CString::new(Vec::new()).expect("no 0 byte"),
            packet_filter: CString::new(Vec::new()).expect("no 0 byte"),
            kafka_broker: CString::new(Vec::new()).expect("no 0 byte"),
            kafka_topic: CString::new(Vec::new()).expect("no 0 byte"),
            kafka_conf: CString::new(Vec::new()).expect("no 0 byte"),
            pattern_file: CString::new(Vec::new()).expect("no 0 byte"),
            file_prefix: CString::new(Vec::new()).expect("no 0 byte"),
            datasource_id: 1,
            initial_seq_no: 0,
            entropy_ratio: 0.9,
            count_sent: 0,
            input_type: InputType::Pcap,
            output_type: OutputType::None,
        }
    }
}

impl fmt::Display for Config {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        writeln!(f, "mode_eval={}", self.mode_eval)?;
        writeln!(f, "mode_grow={}", self.mode_grow)?;
        writeln!(f, "count_sent={}", self.count_sent)?;
        writeln!(f, "count_skip={}", self.count_skip)?;
        writeln!(f, "queue_size={}", self.queue_size)?;
        writeln!(
            f,
            "input={}",
            self.input.clone().into_string().expect("valid UTF-8")
        )?;
        writeln!(
            f,
            "output={}",
            self.output.clone().into_string().expect("valid UTF-8")
        )?;
        writeln!(
            f,
            "offset_prefix={}",
            self.offset_prefix
                .clone()
                .into_string()
                .expect("valid UTF-8")
        )?;
        writeln!(
            f,
            "packet_filter={}",
            self.packet_filter
                .clone()
                .into_string()
                .expect("valid UTF-8")
        )?;
        writeln!(
            f,
            "kafka_broker={}",
            self.kafka_broker
                .clone()
                .into_string()
                .expect("valid UTF-8")
        )?;
        writeln!(
            f,
            "kafka_topic={}",
            self.kafka_topic.clone().into_string().expect("valid UTF-8")
        )?;
        writeln!(
            f,
            "kafka_conf={}",
            self.kafka_conf.clone().into_string().expect("valid UTF-8")
        )?;
        writeln!(
            f,
            "file_prefix={}",
            self.file_prefix.clone().into_string().expect("valid UTF-8")
        )?;
        writeln!(f, "datasource_id={}", self.datasource_id)
    }
}

#[allow(clippy::too_many_lines)]
pub fn parse(args: &[&str]) -> Config {
    let m = App::new("reproduce")
    .version(env!("CARGO_PKG_VERSION"))
    .arg(
        Arg::with_name("broker")
            .short("b")
            .takes_value(true)
	    .value_name("host1:port1,host2:port2,..")
            .help("Kafka broker list"),
    )
    .arg(
        Arg::with_name("count")
            .short("c")
            .takes_value(true)
            .help("Send count"),
    )
    .arg(
        Arg::with_name("source")
            .short("d")
            .takes_value(true)
            .default_value("1")
            .help("Data source ID (1-255)"),
    )
    .arg(
        Arg::with_name("ratio")
            .short("E")
            .takes_value(true)
            .default_value("0.9")
            .help(
                "Entropy ratio. The amount of maximum entropy allowed for a session (0.0-1.0). \
                 Only relevant for network packets.",
            ),
    )
    .arg(
        Arg::with_name("eval")
            .short("e")
            .help("Evaluation mode. Outputs statistics of transmission.")
    )
    .arg(
        Arg::with_name("filter")
            .short("f")
            .takes_value(true)
            .help("tcpdump filter (when input is NIC or PCAP). \
                   See https://www.tcpdump.org/manpages/pcap-filter.7.html.")
    )
    .arg(
        Arg::with_name("continuous")
            .short("g")
            .help("Continues to read from a growing input file")
    )
    .arg(
        Arg::with_name("input")
            .short("i")
            .takes_value(true)
            .help("Input [PCAPFILE/LOGFILE/DIR/NIC] \
	    	   If not given, internal sample data will be used.")
    )
    .arg(
        Arg::with_name("seq")
            .short("j")
            .takes_value(true)
            .help("Sets the initial sequence number (0-16777215).")
    )
    .arg(
        Arg::with_name("config")
            .short("k")
            .takes_value(true)
            .value_name("FILE")
            .help("Kafka config filename to override default Kafka configuration values.")
    )
    .arg(
        Arg::with_name("pattern")
            .short("m")
            .takes_value(true)
            .value_name("FILE")
            .help("Pattern filename")
    )
    .arg(
        Arg::with_name("prefix")
            .short("n")
            .takes_value(true)
            .help("Prefix of file names to send multiple files or a directory")
    )
    .arg(
        Arg::with_name("output")
            .short("o")
            .takes_value(true)
            .help("Output type [TEXTFILE/none]. \
                   If not given, the output is sent to Kafka.")
    )
    .arg(
        Arg::with_name("period")
            .short("p")
            .takes_value(true)
            .default_value("3")
            .help("Sepcifies how long data may be kept in the queue.")
    )
    .arg(
        Arg::with_name("size")
            .short("q")
            .takes_value(true)
            .default_value("900000")
            .help("Specifies the maximum number of bytes to be sent to Kafka in a single message.")
    )
    .arg(
        Arg::with_name("offset")
            .short("r")
            .takes_value(true)
            .help("Record (prefix of offset file). Using this option will start the conversation \
                   after the previous conversation. The offset file name is managed by \
                   <input_file>_<prefix>. Not used if input is NIC.")
    )
    .arg(
        Arg::with_name("skip")
            .short("s")
            .takes_value(true)
            .help("Skip count")
    )
    .arg(
        Arg::with_name("topic")
            .short("t")
            .takes_value(true)
            .help("Kafka topic name. The topic should be available on the broker.")
    )
    .arg(
        Arg::with_name("polling")
            .short("v")
            .help("Polls the input directory")
    )
    .get_matches_from(args);

    let kafka_broker = CString::new(m.value_of("broker").unwrap_or_default()).expect("no 0 byte");
    let count_sent = m
        .value_of("count")
        .map(|v| {
            let result = v.parse::<usize>();
            match result {
                Ok(v) => v,
                Err(e) => {
                    eprintln!("ERROR: invalid count: {}", v);
                    eprintln!("\t{}", e);
                    std::process::exit(1);
                }
            }
        })
        .unwrap_or_default();
    let datasource_id = m.value_of("source").map_or(1, |v| {
        let result = v.parse::<NonZeroU8>();
        match result {
            Ok(v) => v.get(),
            Err(e) => {
                eprintln!("ERROR: invalid data source ID: {}", v);
                eprintln!("\t{}", e);
                std::process::exit(1);
            }
        }
    });
    let entropy_ratio = m.value_of("source").map_or(0.9, |v| {
        let result = v.parse::<f64>();
        match result {
            Ok(v) => {
                if v <= 0.0 || v > 1.0 {
                    eprintln!("ERROR: entropy ratio should be in [0,1)");
                    std::process::exit(1);
                }
                v
            }
            Err(e) => {
                eprintln!("ERROR: invalid entropy ratio: {}", v);
                eprintln!("\t{}", e);
                std::process::exit(1);
            }
        }
    });
    let mode_eval = m.is_present("eval");
    let packet_filter = CString::new(m.value_of("filter").unwrap_or_default()).expect("no 0 byte");
    let mode_grow = m.is_present("continuous");
    let input = CString::new(m.value_of("input").unwrap_or_default()).expect("no 0 byte");
    let initial_seq_no = m
        .value_of("seq")
        .map(|v| {
            let result = v.parse::<u32>();
            match result {
                Ok(v) => v,
                Err(e) => {
                    eprintln!("ERROR: invalid sequence number: {}", v);
                    eprintln!("\t{}", e);
                    std::process::exit(1);
                }
            }
        })
        .unwrap_or_default();
    let kafka_conf = CString::new(m.value_of("config").unwrap_or_default()).expect("no 0 byte");
    let pattern_file = CString::new(m.value_of("pattern").unwrap_or_default()).expect("no 0 byte");
    let file_prefix = CString::new(m.value_of("prefix").unwrap_or_default()).expect("no 0 byte");
    let output = CString::new(m.value_of("output").unwrap_or_default()).expect("no 0 byte");
    let queue_period = m.value_of("period").map_or(3, |v| {
        let result = v.parse::<NonZeroUsize>();
        match result {
            Ok(v) => v.get(),
            Err(e) => {
                eprintln!("ERROR: invalid queue period: {}", v);
                eprintln!("\t{}", e);
                std::process::exit(1);
            }
        }
    });
    let queue_size = m.value_of("size").map_or(900_000, |v| {
        let result = v.parse::<usize>();
        match result {
            Ok(v) => {
                if v > 900_000 {
                    eprintln!("ERROR: queue size is too large (should be no larger than 900,000");
                    std::process::exit(1);
                }
                v
            }
            Err(e) => {
                eprintln!("ERROR: invalid queue size: {}", v);
                eprintln!("\t{}", e);
                std::process::exit(1);
            }
        }
    });
    let offset_prefix = CString::new(m.value_of("offset").unwrap_or_default()).expect("no 0 byte");
    let count_skip = m
        .value_of("skip")
        .map(|v| {
            let result = v.parse::<usize>();
            match result {
                Ok(v) => v,
                Err(e) => {
                    eprintln!("ERROR: invalid queue size: {}", v);
                    eprintln!("\t{}", e);
                    std::process::exit(1);
                }
            }
        })
        .unwrap_or_default();
    let kafka_topic = CString::new(m.value_of("topic").unwrap_or_default()).expect("no 0 byte");
    let mode_polling_dir = m.is_present("polling");
    if output.to_bytes().is_empty() && kafka_broker.to_bytes().is_empty() {
        eprintln!("ERROR: Kafka broker (-b) required");
        std::process::exit(1);
    }
    if output.to_bytes().is_empty() && kafka_topic.to_bytes().is_empty() {
        eprintln!("ERROR: Kafka topic (-t) required");
        std::process::exit(1);
    }
    if input.to_bytes().is_empty() && output.to_bytes() == b"none" {
        eprintln!("ERROR: input (-i) required if output (-o) is \"none\"");
        std::process::exit(1);
    }
    Config {
        mode_eval,
        mode_grow,
        mode_polling_dir,
        count_skip,
        queue_size,
        queue_period,
        input,
        output,
        offset_prefix,
        packet_filter,
        kafka_broker,
        kafka_topic,
        kafka_conf,
        pattern_file,
        file_prefix,
        datasource_id,
        initial_seq_no,
        entropy_ratio,
        count_sent,
        ..Config::default()
    }
}
