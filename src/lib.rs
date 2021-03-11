#![allow(clippy::missing_safety_doc)]

mod fluentd;

use crate::fluentd::SizedForwardMode;
use libc::c_char;
use rmp_serde::Serializer;
use serde::Serialize;
use std::ffi::CStr;
use std::slice;

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
