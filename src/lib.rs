use byteorder::{BigEndian, WriteBytesExt};
use eventio::fluentd::{Entry, ForwardMode};
use libc::c_char;
use rmp_serde::Serializer;
use serde::Serialize;
use serde_bytes::ByteBuf;
use std::collections::HashMap;
use std::ffi::CStr;
use std::mem::size_of;
use std::slice;

#[no_mangle]
pub extern "C" fn forward_mode_new() -> *mut ForwardMode {
    Box::into_raw(Box::new(ForwardMode {
        tag: String::default(),
        entries: Vec::default(),
        option: None,
    }))
}

#[no_mangle]
pub unsafe extern "C" fn forward_mode_free(ptr: *mut ForwardMode) {
    if ptr.is_null() {
        return;
    }
    Box::from_raw(ptr);
}

#[no_mangle]
pub unsafe extern "C" fn forward_mode_size(ptr: *const ForwardMode) -> usize {
    assert!(!ptr.is_null());
    let forward_mode = &*ptr;
    forward_mode.entries.len()
}

#[no_mangle]
pub unsafe extern "C" fn forward_mode_set_tag(ptr: *mut ForwardMode, ctag: *const c_char) -> bool {
    assert!(!ptr.is_null());
    let forward_mode = &mut *ptr;
    assert!(!ctag.is_null());
    let ctag = CStr::from_ptr(ctag);
    let rstag = match ctag.to_str() {
        Ok(v) => v,
        Err(_) => return false,
    };
    forward_mode.tag = rstag.to_string();
    true
}

#[no_mangle]
pub unsafe extern "C" fn forward_mode_append_raw(
    ptr: *mut ForwardMode,
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
    let mut record = HashMap::new();
    record.insert(rs_key.into(), ByteBuf::from(value));
    let entry = Entry { time, record };
    forward_mode.entries.push(entry);
    true
}

#[no_mangle]
pub unsafe extern "C" fn forward_mode_append_packet(
    ptr: *mut ForwardMode,
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
    let src_key = "src".to_string();
    let dst_key = "dst".to_string();
    let src_port_key = "sport".to_string();
    let dst_port_key = "dport".to_string();
    let proto_key = "proto".to_string();

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
    let mut record = HashMap::new();
    record.insert(rs_key.into(), ByteBuf::from(payload));
    let mut buf = Vec::with_capacity(size_of::<u32>());
    buf.write_u32::<BigEndian>(src_ip)
        .expect("no I/O error from Vec");
    record.insert(src_key, ByteBuf::from(buf));
    let mut buf = Vec::with_capacity(size_of::<u32>());
    buf.write_u32::<BigEndian>(dst_ip)
        .expect("no I/O error from Vec");
    record.insert(dst_key, ByteBuf::from(buf));
    let mut buf = Vec::with_capacity(size_of::<u16>());
    buf.write_u16::<BigEndian>(src_port)
        .expect("no I/O error from Vec");
    record.insert(src_port_key, ByteBuf::from(buf));
    let mut buf = Vec::with_capacity(size_of::<u16>());
    buf.write_u16::<BigEndian>(dst_port)
        .expect("no I/O error from Vec");
    record.insert(dst_port_key, ByteBuf::from(buf));
    let mut buf = Vec::with_capacity(size_of::<u8>());
    buf.write_u8(proto).expect("no I/O error from Vec");
    record.insert(proto_key, ByteBuf::from(buf));
    let entry = Entry { time, record };
    forward_mode.entries.push(entry);
    true
}

#[no_mangle]
pub unsafe extern "C" fn forward_mode_add_option(
    ptr: *mut ForwardMode,
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
    if forward_mode.option.is_none() {
        forward_mode.option = Some(HashMap::new());
    }
    forward_mode.option.as_mut().map(|m| {
        m.insert(rs_key.to_string(), rs_value.to_string());
        m
    });
    true
}

#[no_mangle]
pub unsafe extern "C" fn forward_mode_clear(ptr: *mut ForwardMode) {
    assert!(!ptr.is_null());
    let forward_mode = &mut *ptr;
    forward_mode.tag.clear();
    forward_mode.entries.clear();
    forward_mode.option = None;
}

#[no_mangle]
pub unsafe extern "C" fn forward_mode_serialize(
    ptr: *const ForwardMode,
    buf: *mut Vec<u8>,
) -> bool {
    assert!(!ptr.is_null());
    let forward_mode = &*ptr;
    assert!(!buf.is_null());
    let buf = &mut *buf;
    buf.clear();
    forward_mode.serialize(&mut Serializer::new(buf)).is_ok()
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
