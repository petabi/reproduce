use super::SizedForwardMode;
use std::cmp;
use std::collections::HashMap;
use std::mem::size_of;

const MAX_AGE: usize = 128;
const MAX_SAMPLE_SIZE: usize = 2048;
const MIN_SAMPLE_SIZE: usize = 128;
const SESSION_EXTRA_BYTES: usize = 4 /* "src" */ + 4 /* "dst" */ + 6 /* "sport" */ + 6 /* "dport" */ + 6 /* "proto" */ + 2 * 5 + 2 * size_of::<u32>() + 2 * size_of::<u16>() + size_of::<u8>();
const MESSAGE_LABEL_LEN: usize = 8 + size_of::<usize>();

/// An IPv4 session.
struct Sessionv4 {
    src_addr: u32,
    dst_addr: u32,
    src_port: u16,
    dst_port: u16,
    proto: u8,
    event_id: u64,
    age: usize,
    sampling: bool,
    bytes_sampled: usize,
    payload: Vec<u8>,
}

impl Sessionv4 {
    fn new(
        src_addr: u32,
        dst_addr: u32,
        src_port: u16,
        dst_port: u16,
        proto: u8,
        event_id: u64,
        payload: &[u8],
    ) -> Self {
        Sessionv4 {
            src_addr,
            dst_addr,
            src_port,
            dst_port,
            proto,
            event_id,
            age: 0,
            sampling: true,
            bytes_sampled: 0,
            payload: payload.to_vec(),
        }
    }
}

/// A summary of network traffic as a collection of sessions.
pub struct Traffic {
    sessions: HashMap<u64, Sessionv4>,
    message_data: usize,
    entropy_ratio: f64,
    entropy_scratch: [usize; 256],
}

impl Traffic {
    pub fn make_next_message(
        &mut self,
        event_id: u64,
        msg: &mut SizedForwardMode,
        max_len: usize,
    ) -> u64 {
        let mut removal = Vec::new();
        let mut seq_no = (event_id & 0x0000_0000_ffff_ff00) >> 8;
        for (hash, session) in &mut self.sessions {
            if !session.sampling || session.payload.len() < MIN_SAMPLE_SIZE {
                if session.age >= MAX_AGE {
                    removal.push(*hash);
                }
                session.age += 1;
                continue;
            }

            if entropy(&session.payload, &mut self.entropy_scratch)
                / maximum_entropy(session.payload.len())
                < self.entropy_ratio
            {
                if msg.serialized_len()
                    + session.payload.len()
                    + SESSION_EXTRA_BYTES
                    + MESSAGE_LABEL_LEN
                    > max_len
                {
                    continue;
                }

                let _ = msg.push_packet(
                    session.event_id,
                    "message",
                    &session.payload,
                    session.src_addr,
                    session.dst_addr,
                    session.src_port,
                    session.dst_port,
                    session.proto,
                );
                session.bytes_sampled += session.payload.len();
                session.age = 0;
                seq_no += 1;
                if session.bytes_sampled >= MAX_SAMPLE_SIZE {
                    session.sampling = false;
                }
                self.message_data -= session.payload.len();
                session.payload.clear();
                if msg.serialized_len() >= max_len {
                    break;
                }
            } else {
                session.sampling = false;
                session.bytes_sampled = 0;
                self.message_data -= session.payload.len();
                session.payload.clear();
            }
        }

        if !removal.is_empty() {
            for r in removal {
                self.message_data -= self.sessions[&r].payload.len();
                self.sessions.remove(&r);
            }
        }
        (event_id & 0xffff_ffff_0000_00ff) | ((seq_no & 0x00ff_ffff) << 8)
    }

    #[allow(clippy::too_many_arguments)]
    pub fn update_session(
        &mut self,
        src_addr: u32,
        dst_addr: u32,
        src_port: u16,
        dst_port: u16,
        proto: u8,
        data: &[u8],
        event_id: u64,
    ) -> bool {
        let mut is_new = false;
        let hash = hash(src_addr, dst_addr, src_port, dst_port, proto);
        let mut read_len = cmp::min(data.len(), MAX_SAMPLE_SIZE);
        if let Some(session) = self.sessions.get_mut(&hash) {
            if !session.sampling || session.bytes_sampled + session.payload.len() >= MAX_SAMPLE_SIZE
            {
                return is_new;
            }

            read_len = cmp::min(
                read_len,
                MAX_SAMPLE_SIZE - (session.bytes_sampled + session.payload.len()),
            );
            session.payload.extend(&data[0..read_len]);
        } else {
            let mut session = Sessionv4::new(
                src_addr,
                dst_addr,
                src_port,
                dst_port,
                proto,
                event_id,
                &data[0..read_len],
            );
            if read_len < MAX_SAMPLE_SIZE {
                session.payload.reserve(MAX_SAMPLE_SIZE);
            }
            self.sessions.insert(hash, session);
            is_new = true;
        };
        self.message_data += read_len;
        is_new
    }
}

impl Default for Traffic {
    fn default() -> Self {
        let entropy_scratch = [0; 256];
        Traffic {
            sessions: HashMap::new(),
            message_data: 0,
            entropy_ratio: 0.9,
            entropy_scratch,
        }
    }
}

fn hash(src_addr: u32, dst_addr: u32, src_port: u16, dst_port: u16, proto: u8) -> u64 {
    ((u64::from(src_addr) + u64::from(dst_addr)) << 31)
        + (u64::from(proto) << 17)
        + u64::from(src_port)
        + u64::from(dst_port)
}

/// Computes the entropy of the byte sequence.
///
/// `scratch` should contain no non-zero values when this function is called.
/// All elements are cleared when this function returns, so that it can be
/// re-used in the next call.
pub fn entropy(data: &[u8], scratch: &mut [usize; 256]) -> f64 {
    for b in data {
        scratch[usize::from(*b)] += 1;
    }

    #[allow(clippy::cast_precision_loss)] // 52-bit approximation is sufficient.
    let denom = data.len() as f64;
    scratch.iter_mut().fold(0., |entropy, freq| {
        if *freq == 0 {
            entropy
        } else {
            #[allow(clippy::cast_precision_loss)] // 52-bit approximation is sufficient.
            let p = *freq as f64 / denom;
            *freq = 0;
            entropy - p * p.log2()
        }
    })
}

/// Calculates the maximum possible entropy for data of the given length.
pub fn maximum_entropy(len: usize) -> f64 {
    #[allow(clippy::cast_precision_loss)] // 52-bit approximation is sufficient.
    (len as f64).log2()
}

#[cfg(test)]
mod tests {
    use super::{Traffic, MAX_AGE, MIN_SAMPLE_SIZE};
    use crate::SizedForwardMode;

    #[test]
    fn entropy_maximum() {
        let mut scratch = [0; 256];
        assert_eq!(super::entropy(b"abcd", &mut scratch), 2.);
    }

    #[test]
    fn entropy_minimum() {
        let mut scratch = [0; 256];
        assert_eq!(super::entropy(b"aaaa", &mut scratch), 0.);
    }

    #[test]
    fn entropy_reuse_scratch() {
        let mut scratch = [0; 256];
        assert_eq!(super::entropy(b"123456789abcdef0", &mut scratch), 4.);
        assert_eq!(super::entropy(b"12345678", &mut scratch), 3.);
    }

    #[test]
    fn hash() {
        assert_eq!(super::hash(1, 2, 4, 5, 3), 6442844169);
        assert_eq!(
            super::hash(0xffff_ffff, 0xffff_ffff, 0xffff, 0xffff, 0xff),
            0xffff_ffff_01ff_fffe
        );
        assert_eq!(super::hash(0, 0, 0, 0, 0), 0);
        assert_eq!(
            super::hash(0x0102_0304, 0x0506_0708, 0x8090, 0x6070, 0x11),
            super::hash(0x0506_0708, 0x0102_0304, 0x6070, 0x8090, 0x11)
        );
    }

    #[test]
    fn traffic_update() {
        let mut tr = Traffic::default();
        let mut concat = Vec::new();
        let msg_id = 10;
        for i in 1..=9 {
            let data = format!("my message number: {}", i);
            concat.extend(data.bytes());
            let is_new = tr.update_session(
                0x6162_6364,
                0x3132_3334,
                0x4142,
                0x3839,
                0x7a,
                data.as_bytes(),
                msg_id << 8,
            );
            assert_eq!(is_new, i == 1);
            assert_eq!(tr.message_data, 20 * i);
        }

        let mut msg = SizedForwardMode::default();
        let new_msg_id = tr.make_next_message(msg_id << 8, &mut msg, 0xffff);
        assert_eq!(new_msg_id, (msg_id + 1) << 8);

        let record = &msg.message.entries[0].record;
        assert_eq!(record["src"], b"abcd");
        assert_eq!(record["dst"], b"1234");
        assert_eq!(record["sport"], b"AB");
        assert_eq!(record["dport"], b"89");
        assert_eq!(record["proto"], b"z");
        assert_eq!(record["message"], concat);
    }

    #[test]
    fn traffic_delete() {
        let mut tr = Traffic::default();
        let mut event_id = 6;
        let content = "traffic_delete";
        tr.update_session(1, 2, 4, 5, 3, content.as_bytes(), event_id);

        let mut msg = SizedForwardMode::default();
        for _ in 0..MAX_AGE {
            assert_eq!(tr.message_data, content.len());
            event_id = tr.make_next_message(0, &mut msg, 0xffff);
            assert!(!tr.sessions.is_empty());
        }

        let mut i = 2;
        while tr.message_data < MIN_SAMPLE_SIZE {
            tr.update_session(1, 2, 4, 5, 3, content.as_bytes(), event_id);
            assert_eq!(tr.message_data, i * content.len());
            assert!(!tr.sessions.is_empty());
            i += 1;
        }
        event_id = tr.make_next_message(event_id, &mut msg, 0xffff);
        assert_eq!(event_id >> 8, 1);
        assert_eq!(tr.message_data, 0);
        i = 1;
        while tr.message_data < MIN_SAMPLE_SIZE {
            tr.update_session(1, 2, 4, 5, 3, content.as_bytes(), event_id);
            assert_eq!(tr.message_data, i * content.len());
            assert!(!tr.sessions.is_empty());
            i += 1;
        }
        event_id = tr.make_next_message(event_id, &mut msg, 0xffff);
        assert_eq!(event_id >> 8, 2);
        assert_eq!(tr.message_data, 0);
        tr.update_session(1, 2, 4, 5, 3, content.as_bytes(), event_id);
        for _ in 0..MAX_AGE {
            assert_eq!(tr.message_data, content.len());
            event_id = tr.make_next_message(event_id, &mut msg, 0xffff);
            assert_eq!(event_id >> 8, 2);
            assert!(!tr.sessions.is_empty());
        }
        event_id = tr.make_next_message(event_id, &mut msg, 0xffff);
        assert_eq!(event_id >> 8, 2);
        assert_eq!(tr.message_data, 0);
        assert!(tr.sessions.is_empty());
    }
}
