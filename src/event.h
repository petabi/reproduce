#ifndef EVENT_H
#define EVENT_H

struct ForwardMode;
struct SerializationBuffer;

extern "C" {

bool forward_mode_add_option(ForwardMode* ptr, const char* ckey,
                             const char* cvalue);
bool forward_mode_append_packet(ForwardMode* ptr, uint64_t time,
                                const char* key, const uint8_t* payload,
                                uintptr_t len, uint32_t src, uint32_t dst,
                                uint16_t sport, uint16_t dport, uint8_t proto);
bool forward_mode_append_raw(ForwardMode* ptr, uint64_t time, const char* key,
                             const uint8_t* value, uintptr_t len);
void forward_mode_clear(ForwardMode* ptr);
void forward_mode_free(ForwardMode* ptr);
ForwardMode* forward_mode_new();
bool forward_mode_serialize(const ForwardMode* ptr, SerializationBuffer* buf);
uintptr_t forward_mode_serialized_len(const ForwardMode* ptr);
bool forward_mode_set_tag(ForwardMode* ptr, const char* ctag);
uintptr_t forward_mode_size(const ForwardMode* ptr);
void serialization_buffer_free(SerializationBuffer* ptr);
const uint8_t* serialization_buffer_data(const SerializationBuffer* buf);
uintptr_t serialization_buffer_len(const SerializationBuffer* buf);
SerializationBuffer* serialization_buffer_new();

} // extern "C"

#endif // EVENT_H
