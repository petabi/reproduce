use anyhow::Result;
use regex::bytes::RegexSet;
use std::io::Read;

pub struct Matcher {
    db: RegexSet,
}

impl Matcher {
    /// # Errors
    ///
    /// Returns an error if reading from `r` fails, or building a regular
    /// expression matcher fails.
    pub fn from_read<R: Read>(mut r: R) -> Result<Self> {
        let mut exps = String::new();
        r.read_to_string(&mut exps)?;
        let rules = trim_to_rules(&exps);
        let db: RegexSet = RegexSet::new(rules)?;
        Ok(Matcher { db })
    }

    /// # Errors
    ///
    /// Always returns `Ok`.
    pub fn scan(&mut self, data: &[u8]) -> Result<bool> {
        Ok(self.db.is_match(data))
    }
}

fn trim_to_rules(s: &str) -> Vec<&str> {
    s.lines()
        .filter_map(|line| {
            let line = line.trim();

            if line.is_empty() || line.starts_with('#') {
                None
            } else {
                let expr = match line.find(":/") {
                    Some(off) => &line[off + 1..],
                    None => line,
                };
                Some(match (expr.starts_with('/'), expr.rfind('/')) {
                    (true, Some(end)) if end > 0 => &expr[1..end],
                    _ => expr,
                })
            }
        })
        .collect::<Vec<_>>()
}

#[cfg(test)]
mod tests {
    use super::Matcher;

    #[test]
    fn scan() {
        let exps = "abc\nxyz\n";
        let mut matcher = Matcher::from_read(exps.as_bytes()).expect("valid exps");
        assert_eq!(matcher.scan(b"hello").expect("Ok"), false);
        assert_eq!(matcher.scan(b"00xyz00").expect("Ok"), true);
    }
}
