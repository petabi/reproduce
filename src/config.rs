use clap::{App, Arg};

pub struct Config {}

impl Default for Config {
    fn default() -> Self {
        Config {}
    }
}

#[allow(clippy::too_many_lines)]
pub fn parse(args: &[&str]) -> Config {
    let _ = App::new("reproduce")
    .version(env!("CARGO_PKG_VERSION"))
    .arg(
        Arg::with_name("host1:port1,host2:port2,...")
            .short("b")
            .takes_value(true)
            .help("Kafka broker list"),
    )
    .arg(
        Arg::with_name("count")
            .short("c")
            .takes_value(true)
            .help("Send count"),
    )
    .arg(
        Arg::with_name("ID")
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
        Arg::with_name("mode")
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
            .possible_values(&["PCAPFILE", "LOGFILE", "DIR", "NIC"])
            .help("Input (If not given, internal sample data will be used.")
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
            .possible_values(&["TEXTFILE", "none"])
            .help("Output type. If not given, the output is sent to Kafka.")
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

    Config::default()
}
