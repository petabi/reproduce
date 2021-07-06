use crate::{Matcher, SizedForwardMode};
use anyhow::Result;

pub struct Converter {
    matcher: Option<Matcher>,
}

impl Converter {
    #[must_use]
    pub fn new(matcher: Option<Matcher>) -> Self {
        Self { matcher }
    }

    /// # Errors
    ///
    /// Returns an error if there is no more space to store a new message.
    pub fn convert(
        &mut self,
        event_id: u64,
        input: &[u8],
        msg: &mut SizedForwardMode,
    ) -> Result<bool> {
        if let Some(ref mut matcher) = self.matcher {
            if matcher.scan(input)? {
                return Ok(false);
            }
        }
        msg.push_raw(event_id, "message", input)?;
        Ok(true)
    }

    #[must_use]
    pub fn has_matcher(&self) -> bool {
        self.matcher.is_some()
    }
}

#[cfg(test)]
mod tests {
    use super::Converter;
    use crate::{Matcher, SizedForwardMode};

    #[test]
    fn log_converter() {
        let msg_skip = b"this message contains abc.";
        let msg_send = b"this message doesn't contain it.";
        let mut msg = SizedForwardMode::default();

        let mut conv = Converter::new(None);
        assert_eq!(conv.convert(1, msg_skip, &mut msg).expect("Ok"), true);
        assert_eq!(conv.convert(1, msg_send, &mut msg).expect("Ok"), true);

        let exps = "abc\nxyz\n";
        let matcher = Matcher::from_read(exps.as_bytes()).expect("valid exps");
        let mut conv = Converter::new(Some(matcher));
        assert_eq!(conv.convert(1, msg_skip, &mut msg).expect("Ok"), false);
        assert_eq!(conv.convert(1, msg_send, &mut msg).expect("Ok"), true);
    }
}
