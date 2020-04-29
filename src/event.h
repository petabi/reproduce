#ifndef EVENT_H
#define EVENT_H

struct ForwardMode;
struct SerializationBuffer;

extern "C" {

auto forward_mode_add_option(ForwardMode* ptr, const char* ckey,
                             const char* cvalue) -> bool;
auto forward_mode_append_packet(ForwardMode* ptr, uint64_t time,
                                const char* key, const uint8_t* payload,
                                uintptr_t len, uint32_t src, uint32_t dst,
                                uint16_t sport, uint16_t dport, uint8_t proto)
    -> bool;
auto forward_mode_append_raw(ForwardMode* ptr, uint64_t time, const char* key,
                             const uint8_t* value, uintptr_t len) -> bool;
void forward_mode_clear(ForwardMode* ptr);
void forward_mode_free(ForwardMode* ptr);
auto forward_mode_new() -> ForwardMode*;
auto forward_mode_serialize(const ForwardMode* ptr, SerializationBuffer* buf)
    -> bool;
auto forward_mode_serialized_len(const ForwardMode* ptr) -> uintptr_t;
auto forward_mode_set_tag(ForwardMode* ptr, const char* ctag) -> bool;
auto forward_mode_size(const ForwardMode* ptr) -> uintptr_t;
void serialization_buffer_free(SerializationBuffer* ptr);
auto serialization_buffer_data(const SerializationBuffer* buf)
    -> const uint8_t*;
auto serialization_buffer_len(const SerializationBuffer* buf) -> uintptr_t;
auto serialization_buffer_new() -> SerializationBuffer*;

} // extern "C"

#endif // EVENT_H
