use anyhow::Result;
use chrono::{DateTime, Duration, Utc};
use kafka::error::Error as KafkaError;
use kafka::producer::Record;
use std::fs::File;
use std::io::Write;

#[allow(clippy::large_enum_variant)]
pub enum Producer {
    File(File),
    Kafka(Kafka),
    Null,
}

impl Producer {
    pub fn new_file(filename: &str) -> Result<Self> {
        let output = File::create(filename)?;
        Ok(Producer::File(output))
    }

    /// Constructs a new Kafka producer.
    ///
    /// # Errors
    ///
    /// Returns an error if it fails to create the underlying Kafka producer.
    pub fn new_kafka(
        broker: &str,
        topic: &str,
        input_type: KafkaInput,
        queue_size: usize,
        queue_period: i64,
        grow: bool,
    ) -> Result<Self, KafkaError> {
        const IDLE_TIMEOUT: u64 = 540;
        const ACK_TIMEOUT: u64 = 5;
        let producer = kafka::producer::Producer::from_hosts(vec![broker.to_string()])
            .with_connection_idle_timeout(std::time::Duration::new(IDLE_TIMEOUT, 0))
            .with_ack_timeout(std::time::Duration::new(ACK_TIMEOUT, 0))
            .create()?;
        let period_check = grow && input_type == KafkaInput::Nic;
        let last_time = Utc::now();
        Ok(Self::Kafka(Kafka {
            inner: producer,
            topic: topic.to_string(),
            queue_data: Vec::new(),
            queue_data_cnt: 0,
            queue_size,
            queue_period: Duration::seconds(queue_period),
            period_check,
            last_time,
        }))
    }

    pub fn new_null() -> Self {
        Self::Null
    }

    pub fn max_bytes() -> usize {
        const DEFAULT_MAX_BYTES: usize = 100_000;
        DEFAULT_MAX_BYTES
    }

    pub fn produce(&mut self, message: &[u8], flush: bool) -> Result<()> {
        match self {
            Producer::File(f) => {
                f.write_all(message)?;
                f.write_all(b"\n")?;
                if flush {
                    f.flush()?;
                }
                Ok(())
            }
            Producer::Kafka(inner) => {
                if !message.is_empty() {
                    inner.queue_data.extend(message);
                    if !flush {
                        inner.queue_data_cnt += 1;
                    }
                }

                if flush || inner.periodic_flush() || inner.queue_data.len() >= inner.queue_size {
                    if let Err(e) = inner.send(message) {
                        inner.queue_data.clear();
                        inner.queue_data_cnt = 0;
                        // TODO: error handling
                        return Err(anyhow::Error::msg(e.to_string()));
                    }
                    inner.queue_data.clear();
                    inner.queue_data_cnt = 0;

                    inner.last_time = Utc::now();
                } else {
                    inner.queue_data.push(b'\n');
                }
                Ok(())
            }
            Producer::Null => Ok(()),
        }
    }
}

#[derive(Clone, Copy, PartialEq)]
pub enum KafkaInput {
    Pcap,
    PcapNg,
    Nic,
    Log,
    Dir,
}

pub struct Kafka {
    inner: kafka::producer::Producer,
    topic: String,
    queue_data: Vec<u8>,
    queue_data_cnt: usize,
    queue_size: usize,
    queue_period: Duration,
    period_check: bool,
    last_time: DateTime<Utc>,
}

impl Kafka {
    fn periodic_flush(&mut self) -> bool {
        if !self.period_check {
            return false;
        }

        let current = Utc::now();
        current - self.last_time > self.queue_period
    }

    /// Sends a message to the Kafka server.
    ///
    /// # Errors
    ///
    /// Returns an error if transmission fails.
    pub fn send(&mut self, msg: &[u8]) -> Result<(), KafkaError> {
        self.inner.send(&Record::from_value(&self.topic, msg))
    }
}

#[cfg(test)]
mod tests {
    use super::Producer;

    #[test]
    fn null() {
        let mut producer = Producer::new_null();
        assert!(producer.produce(b"A message", true).is_ok());
    }
}
