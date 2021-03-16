use kafka::error::Error as KafkaError;
use kafka::producer::{Producer, Record};
use std::time::Duration;

pub struct Kafka {
    producer: Producer,
    topic: String,
}

impl Kafka {
    /// Constructs a new Kafka producer.
    ///
    /// # Errors
    ///
    /// Returns an error if it fails to create the underlying Kafka producer.
    pub fn new(
        broker: &str,
        topic: &str,
        idle_timeout: Duration,
        ack_timeout: Duration,
    ) -> Result<Kafka, KafkaError> {
        let producer = Producer::from_hosts(vec![broker.to_string()])
            .with_connection_idle_timeout(idle_timeout)
            .with_ack_timeout(ack_timeout)
            .create()?;
        Ok(Kafka {
            producer,
            topic: topic.to_string(),
        })
    }

    /// Sends a message to the Kafka server.
    ///
    /// # Errors
    ///
    /// Returns an error if transmission fails.
    pub fn send(&mut self, msg: &[u8]) -> Result<(), KafkaError> {
        self.producer.send(&Record::from_value(&self.topic, msg))
    }
}
