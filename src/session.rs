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
    #[test]
    fn maximum() {
        let mut scratch = [0; 256];
        assert_eq!(super::entropy(b"abcd", &mut scratch), 2.);
    }

    #[test]
    fn minimum() {
        let mut scratch = [0; 256];
        assert_eq!(super::entropy(b"aaaa", &mut scratch), 0.);
    }

    #[test]
    fn reuse_scratch() {
        let mut scratch = [0; 256];
        assert_eq!(super::entropy(b"123456789abcdef0", &mut scratch), 4.);
        assert_eq!(super::entropy(b"12345678", &mut scratch), 3.);
    }
}
