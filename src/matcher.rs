use anyhow::Result;
use hyperscan::prelude::*;
use std::fs;

pub struct Matcher {
    db: BlockDatabase,
    scratch: Scratch,
}

impl Matcher {
    pub fn with_file(filename: &str) -> Result<Self> {
        let patterns: Patterns = fs::read_to_string(filename)?.parse()?;
        let db: BlockDatabase = patterns.build()?;
        let scratch = db.alloc_scratch()?;
        Ok(Matcher { db, scratch })
    }

    pub fn scan(&mut self, data: &[u8]) -> Result<bool> {
        let mut is_matched = false;
        self.db.scan(data, &self.scratch, |_, _, _, _| {
            is_matched = true;
            Matching::Continue
        })?;
        Ok(is_matched)
    }
}
