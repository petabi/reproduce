#![allow(clippy::missing_safety_doc)]

mod fluentd;
mod matcher;
mod producer;

use crate::fluentd::SizedForwardMode;
use crate::matcher::Matcher;
use libc::c_char;
pub use producer::Kafka as KafkaProducer;
use rmp_serde::Serializer;
use serde::Serialize;
use std::ffi::CStr;
use std::ptr;
use std::slice;
use std::time::Duration;

#[no_mangle]
pub extern "C" fn forward_mode_new() -> *mut SizedForwardMode {
    Box::into_raw(Box::new(SizedForwardMode::default()))
}

#[no_mangle]
pub unsafe extern "C" fn forward_mode_free(ptr: *mut SizedForwardMode) {
    if ptr.is_null() {
        return;
    }
    Box::from_raw(ptr);
}

#[no_mangle]
pub unsafe extern "C" fn forward_mode_size(ptr: *const SizedForwardMode) -> usize {
    assert!(!ptr.is_null());
    let forward_mode = &*ptr;
    forward_mode.len()
}

#[no_mangle]
pub unsafe extern "C" fn forward_mode_serialized_len(ptr: *const SizedForwardMode) -> usize {
    assert!(!ptr.is_null());
    let forward_mode = &*ptr;
    forward_mode.serialized_len()
}

#[no_mangle]
pub unsafe extern "C" fn forward_mode_set_tag(
    ptr: *mut SizedForwardMode,
    ctag: *const c_char,
) -> bool {
    assert!(!ptr.is_null());
    let forward_mode = &mut *ptr;
    assert!(!ctag.is_null());
    let ctag = CStr::from_ptr(ctag);
    let rstag = match ctag.to_str() {
        Ok(v) => v,
        Err(_) => return false,
    };
    forward_mode.set_tag(rstag.to_string()).is_ok()
}

#[no_mangle]
pub unsafe extern "C" fn forward_mode_append_raw(
    ptr: *mut SizedForwardMode,
    time: u64,
    key: *const c_char,
    value: *const u8,
    len: usize,
) -> bool {
    assert!(!ptr.is_null());
    let forward_mode = &mut *ptr;
    assert!(!key.is_null());
    let c_key = CStr::from_ptr(key);
    let rs_key = match c_key.to_str() {
        Ok(v) => v,
        Err(_) => return false,
    };
    assert!(!value.is_null());
    let value = slice::from_raw_parts(value, len);
    forward_mode.push_raw(time, rs_key, value).is_ok()
}

#[no_mangle]
pub unsafe extern "C" fn forward_mode_append_packet(
    ptr: *mut SizedForwardMode,
    time: u64,
    key: *const c_char,
    payload: *const u8,
    len: usize,
    src_ip: u32,
    dst_ip: u32,
    src_port: u16,
    dst_port: u16,
    proto: u8,
) -> bool {
    assert!(!ptr.is_null());
    let forward_mode = &mut *ptr;
    assert!(!key.is_null());
    let c_key = CStr::from_ptr(key);
    let rs_key = match c_key.to_str() {
        Ok(v) => v,
        Err(_) => return false,
    };
    assert!(!payload.is_null());
    let payload = slice::from_raw_parts(payload, len);
    forward_mode
        .push_packet(
            time, rs_key, payload, src_ip, dst_ip, src_port, dst_port, proto,
        )
        .is_ok()
}

#[no_mangle]
pub unsafe extern "C" fn forward_mode_add_option(
    ptr: *mut SizedForwardMode,
    key: *const c_char,
    value: *const c_char,
) -> bool {
    assert!(!ptr.is_null());
    let forward_mode = &mut *ptr;
    assert!(!key.is_null());
    let c_key = CStr::from_ptr(key);
    let rs_key = match c_key.to_str() {
        Ok(v) => v,
        Err(_) => return false,
    };
    assert!(!value.is_null());
    let c_value = CStr::from_ptr(value);
    let rs_value = match c_value.to_str() {
        Ok(v) => v,
        Err(_) => return false,
    };
    forward_mode.push_option(rs_key, rs_value).is_ok()
}

#[no_mangle]
pub unsafe extern "C" fn forward_mode_clear(ptr: *mut SizedForwardMode) {
    assert!(!ptr.is_null());
    let forward_mode = &mut *ptr;
    forward_mode.clear();
}

#[no_mangle]
pub unsafe extern "C" fn forward_mode_serialize(
    ptr: *const SizedForwardMode,
    buf: *mut Vec<u8>,
) -> bool {
    assert!(!ptr.is_null());
    let forward_mode = &*ptr;
    assert!(!buf.is_null());
    let buf = &mut *buf;
    buf.clear();
    forward_mode
        .message
        .serialize(&mut Serializer::new(buf))
        .is_ok()
}

#[no_mangle]
pub unsafe extern "C" fn kafka_producer_new(
    broker: *const c_char,
    topic: *const c_char,
    idle_timeout_ms: u32,
    ack_timeout_ms: u32,
) -> *mut KafkaProducer {
    let topic = match CStr::from_ptr(topic).to_str() {
        Ok(topic) => topic,
        Err(_) => return ptr::null_mut(),
    };
    let idle_timeout = Duration::new(
        (idle_timeout_ms / 1_000).into(),
        idle_timeout_ms % 1_000 * 1_000_000,
    );
    let ack_timeout = Duration::new(
        (ack_timeout_ms / 1_000).into(),
        ack_timeout_ms % 1_000 * 1_000_000,
    );
    let producer = match CStr::from_ptr(broker).to_str() {
        Ok(broker) => match KafkaProducer::new(broker, topic, idle_timeout, ack_timeout) {
            Ok(producer) => producer,
            Err(_) => return ptr::null_mut(),
        },
        Err(_) => return ptr::null_mut(),
    };
    Box::into_raw(Box::new(producer))
}

#[no_mangle]
pub unsafe extern "C" fn kafka_producer_free(ptr: *mut KafkaProducer) {
    if ptr.is_null() {
        return;
    }
    Box::from_raw(ptr);
}

#[no_mangle]
pub unsafe extern "C" fn kafka_producer_send(
    ptr: *mut KafkaProducer,
    msg: *const u8,
    len: usize,
) -> usize {
    let producer = &mut *ptr;
    if producer.send(slice::from_raw_parts(msg, len)).is_ok() {
        1
    } else {
        0
    }
}

#[no_mangle]
pub unsafe extern "C" fn matcher_new(filename_ptr: *const c_char) -> *mut Matcher {
    let c_filename = CStr::from_ptr(filename_ptr);
    let filename = match c_filename.to_str() {
        Ok(s) => s,
        Err(_) => return ptr::null_mut(),
    };
    let matcher = match Matcher::with_file(filename) {
        Ok(m) => m,
        Err(_) => return ptr::null_mut(),
    };
    Box::into_raw(Box::new(matcher))
}

#[no_mangle]
pub unsafe extern "C" fn matcher_free(ptr: *mut Matcher) {
    if ptr.is_null() {
        return;
    }
    Box::from_raw(ptr);
}

#[no_mangle]
pub unsafe extern "C" fn matcher_match(ptr: *mut Matcher, data: *const u8, len: usize) -> usize {
    let matcher = &mut *ptr;
    if matcher
        .scan(slice::from_raw_parts(data, len))
        .unwrap_or_default()
    {
        1
    } else {
        0
    }
}

#[no_mangle]
pub extern "C" fn serialization_buffer_new() -> *mut Vec<u8> {
    Box::into_raw(Box::new(Vec::new()))
}

#[no_mangle]
pub unsafe extern "C" fn serialization_buffer_free(ptr: *mut Vec<u8>) {
    if ptr.is_null() {
        return;
    }
    Box::from_raw(ptr);
}

#[no_mangle]
pub unsafe extern "C" fn serialization_buffer_data(buf: *const Vec<u8>) -> *const u8 {
    assert!(!buf.is_null());
    let buf = &*buf;
    buf.as_slice().as_ptr()
}

#[no_mangle]
pub unsafe extern "C" fn serialization_buffer_len(buf: *const Vec<u8>) -> usize {
    assert!(!buf.is_null());
    let buf = &*buf;
    buf.len()
}
