// SPSC byte FIFO between audio_pipeline (producer) and ble_audio_uploader (consumer).
//
// Implementation: thin facade over FreeRTOS StreamBuffer (Espressif ESP-IDF).
// FreeRTOS stream buffers are SPSC-safe by design and battle-tested; we add only
// our project-specific sizing constants and Serial-logged init / reset semantics.
//
// Reference (verified 2026-04-29):
//   https://docs.espressif.com/projects/esp-idf/en/v4.4/esp32/api-reference/system/freertos.html
//   https://www.freertos.org/RTOS-stream-buffer-API.html

#pragma once

#include <stddef.h>
#include <stdint.h>

namespace audio {

// 8 KB ≈ 1 s of 16 kHz mono ADPCM (8 KB/s); covers BLE jitter up to ~1 s.
constexpr size_t RING_BUFFER_CAPACITY = 8192;

bool   ring_buffer_init();
void   ring_buffer_reset();
size_t ring_buffer_push(const uint8_t* data, size_t len);
size_t ring_buffer_pop(uint8_t* out, size_t max_len, uint32_t timeout_ms);
size_t ring_buffer_bytes_available();
size_t ring_buffer_bytes_free();

}  // namespace audio
