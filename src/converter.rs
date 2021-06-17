use crate::{Matcher, SizedForwardMode};
use anyhow::Result;
use nom::{
    bytes::complete::take,
    error::{Error, ErrorKind},
    number::complete::{be_u16, u8},
    Err, IResult,
};
use pcap::Linktype;
use std::io;

pub enum Converter {
    Log(LogConverter),
    Packet(PacketConverter),
}

impl Converter {
    #[must_use]
    pub fn with_log(matcher: Option<Matcher>) -> Self {
        Self::Log(LogConverter::new(matcher))
    }

    #[must_use]
    pub fn with_packet(l2_type: Linktype, matcher: Option<Matcher>) -> Self {
        Self::Packet(PacketConverter::new(l2_type, matcher))
    }

    /// # Errors
    ///
    /// Returns an error if storing `input` in `msg` fails.
    pub fn convert(
        &mut self,
        event_id: u64,
        input: &[u8],
        msg: &mut SizedForwardMode,
    ) -> Result<bool> {
        match self {
            Self::Log(c) => c.convert(event_id, input, msg),
            Self::Packet(c) => c.convert(event_id, input, msg),
        }
    }

    #[must_use]
    pub fn has_matcher(&self) -> bool {
        match self {
            Self::Log(c) => c.matcher.is_some(),
            Self::Packet(c) => c.matcher.is_some(),
        }
    }
}

#[allow(clippy::module_name_repetitions)]
pub struct LogConverter {
    matcher: Option<Matcher>,
}

impl LogConverter {
    fn new(matcher: Option<Matcher>) -> Self {
        Self { matcher }
    }

    fn convert(&mut self, event_id: u64, input: &[u8], msg: &mut SizedForwardMode) -> Result<bool> {
        if let Some(ref mut matcher) = self.matcher {
            if matcher.scan(input)? {
                return Ok(false);
            }
        }
        msg.push_raw(event_id, "message", input)?;
        Ok(true)
    }
}

#[allow(clippy::module_name_repetitions)]
pub struct PacketConverter {
    l2_type: Linktype,
    matcher: Option<Matcher>,
}

impl PacketConverter {
    fn new(l2_type: Linktype, matcher: Option<Matcher>) -> Self {
        Self { l2_type, matcher }
    }

    fn convert(&mut self, event_id: u64, input: &[u8], msg: &mut SizedForwardMode) -> Result<bool> {
        if self.l2_type != Linktype::ETHERNET {
            return Err(io::Error::new(io::ErrorKind::InvalidData, "unsupported link type").into());
        }

        let (input, ethertype) = if let Ok(r) = parse_ethernet_frame(input) {
            r
        } else {
            return Err(
                io::Error::new(io::ErrorKind::InvalidData, "invalid Ethernet frame").into(),
            );
        };

        match ethertype {
            0x0800 => {
                let (input, proto) = if let Ok(r) = parse_ipv4_packet(input) {
                    r
                } else {
                    return Err(
                        io::Error::new(io::ErrorKind::InvalidData, "invalid IPv4 packet").into(),
                    );
                };
                let payload = match proto {
                    0x01 => parse_icmp_packet(input).map_err(|_| {
                        io::Error::new(io::ErrorKind::InvalidData, "invalid ICMP packet")
                    })?,
                    0x06 => parse_tcp_packet(input).map_err(|_| {
                        io::Error::new(io::ErrorKind::InvalidData, "invalid TCP packet")
                    })?,
                    0x11 => parse_udp_packet(input).map_err(|_| {
                        io::Error::new(io::ErrorKind::InvalidData, "invalid UDP packet")
                    })?,
                    _ => return Ok(true),
                }
                .0;
                if let Some(ref mut m) = self.matcher {
                    let is_matched = m.scan(payload)?;
                    if !is_matched {
                        msg.push_raw(event_id, "message", payload)?;
                    }
                    Ok(!is_matched)
                } else {
                    msg.push_raw(event_id, "message", payload)?;
                    Ok(true)
                }
            }
            _ => Err(io::Error::new(io::ErrorKind::InvalidData, "non-IP packet").into()),
        }
    }
}

fn parse_ethernet_frame(input: &[u8]) -> IResult<&[u8], u16> {
    let (input, _) = take(12_usize)(input)?;
    let (input, ethertype) = be_u16(input)?;
    if ethertype == 0x8100 {
        let (input, _) = take(2_usize)(input)?;
        be_u16(input)
    } else {
        Ok((input, ethertype))
    }
}

fn parse_ipv4_packet(input: &[u8]) -> IResult<&[u8], u8> {
    let (input, _) = take(9_usize)(input)?;
    let (input, proto) = u8(input)?;
    let (input, _) = take(10_usize)(input)?;
    Ok((input, proto))
}

fn parse_icmp_packet(input: &[u8]) -> IResult<&[u8], &[u8]> {
    take(8_usize)(input)
}

fn parse_tcp_packet(input: &[u8]) -> IResult<&[u8], &[u8]> {
    let (input, _) = take(12_usize)(input)?;
    let (input, data_offset) = {
        let (input, b) = u8(input)?;
        let data_offset = (b & 0xf0) >> 4;
        if data_offset < 5 {
            return Err(Err::Failure(Error::new(input, ErrorKind::Verify)));
        }
        (input, data_offset)
    };
    take(7 + usize::from(data_offset - 5) * 4)(input)
}

fn parse_udp_packet(input: &[u8]) -> IResult<&[u8], &[u8]> {
    take(8_usize)(input)
}

#[cfg(test)]
mod tests {
    use super::{LogConverter, PacketConverter};
    use crate::{Matcher, SizedForwardMode};
    use pcap::Linktype;

    #[test]
    fn log_converter() {
        let msg_skip = b"this message contains abc.";
        let msg_send = b"this message doesn't contain it.";
        let mut msg = SizedForwardMode::default();

        let mut conv = LogConverter::new(None);
        assert_eq!(conv.convert(1, msg_skip, &mut msg).expect("Ok"), true);
        assert_eq!(conv.convert(1, msg_send, &mut msg).expect("Ok"), true);

        let exps = "abc\nxyz\n";
        let matcher = Matcher::from_read(exps.as_bytes()).expect("valid exps");
        let mut conv = LogConverter::new(Some(matcher));
        assert_eq!(conv.convert(1, msg_skip, &mut msg).expect("Ok"), false);
        assert_eq!(conv.convert(1, msg_send, &mut msg).expect("Ok"), true);
    }

    #[test]
    fn packet_converter_tcp() {
        const MSG_LEN: usize = 4;

        let pkt1: Vec<u8> = vec![
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00,
            0x45, 0x00, 0x00, 0x2B, 0x00, 0x01, 0x00, 0x00, 0x40, 0x06, 0x7C, 0xCA, 0x7F, 0x00,
            0x00, 0x01, 0x7F, 0x00, 0x00, 0x01, 0x00, 0x14, 0x00, 0x50, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x50, 0x02, 0x20, 0x00, 0xCD, 0x16, 0x00, 0x00, 0x61, 0x62,
            0x63,
        ];
        let pkt2: Vec<u8> = vec![
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00,
            0x45, 0x00, 0x00, 0x2B, 0x00, 0x01, 0x00, 0x00, 0x40, 0x06, 0x7C, 0xCA, 0x7F, 0x00,
            0x00, 0x01, 0x7F, 0x00, 0x00, 0x01, 0x31, 0x32, 0x61, 0x62, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x50, 0x02, 0x20, 0x00, 0xCD, 0x16, 0x00, 0x00, 0x31, 0x32,
            0x33,
        ];
        let exps = "abc\nxyz\n";
        let matcher = Matcher::from_read(exps.as_bytes()).expect("valid exps");
        let mut conv = PacketConverter::new(Linktype::ETHERNET, Some(matcher));
        let mut msg = SizedForwardMode::default();
        for _ in 0..MSG_LEN {
            assert_eq!(conv.convert(1, &pkt1, &mut msg).expect("Ok"), false);
            assert_eq!(conv.convert(2, &pkt2, &mut msg).expect("Ok"), true);
        }
        assert_eq!(msg.len(), MSG_LEN);
        assert_eq!(&msg.message.entries[0].record["message"][..3], b"123");
    }

    #[test]
    fn packet_converter_vlan() {
        let pkt: Vec<u8> = vec![
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x81, 0x00,
            0x00, 0x20, 0x08, 0x00, 0x45, 0x00, 0x00, 0x2B, 0x00, 0x01, 0x00, 0x00, 0x40, 0x06,
            0x7C, 0xCA, 0x7F, 0x00, 0x00, 0x01, 0x7F, 0x00, 0x00, 0x01, 0x00, 0x14, 0x00, 0x50,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x02, 0x20, 0x00, 0xCD, 0x16,
            0x00, 0x00, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C,
            0x6D, 0x6E, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C,
            0x6D, 0x6E, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C,
            0x6D, 0x6E, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C,
            0x6D, 0x6E, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C,
            0x6D, 0x6E, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C,
            0x6D, 0x6E, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C,
            0x6D, 0x6E, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C,
            0x6D, 0x6E, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C,
            0x6D, 0x6E, 0x65, 0x65,
        ];
        let mut conv = PacketConverter::new(Linktype::ETHERNET, None);
        let mut msg = SizedForwardMode::default();
        assert_eq!(conv.convert(1, &pkt, &mut msg).expect("Ok"), true);
        assert_eq!(msg.len(), 1);
        assert_eq!(
            &msg.message.entries[0].record["message"],
            b"abcdefghijklmnabcdefghijklmnabcdefghijklmnabcdefghijklmnabcdefghijklmn\
              abcdefghijklmnabcdefghijklmnabcdefghijklmnabcdefghijklmnee"
        );
    }
}
