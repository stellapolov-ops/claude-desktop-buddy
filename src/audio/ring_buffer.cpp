// See ring_buffer.h.

#include "ring_buffer.h"

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/stream_buffer.h>

namespace audio {
namespace {

StreamBufferHandle_t stream = nullptr;

}  // namespace

bool ring_buffer_init() {
  if (stream != nullptr) return true;
  stream = xStreamBufferCreate(RING_BUFFER_CAPACITY, /*trigger_level=*/1);
  if (stream == nullptr) {
    Serial.println("[ring_buffer] xStreamBufferCreate failed");
    return false;
  }
  Serial.printf("[ring_buffer] init OK: %u bytes\n",
                (unsigned)RING_BUFFER_CAPACITY);
  return true;
}

void ring_buffer_reset() {
  if (stream != nullptr) xStreamBufferReset(stream);
}

size_t ring_buffer_push(const uint8_t* data, size_t len) {
  if (stream == nullptr || data == nullptr || len == 0) return 0;
  return xStreamBufferSend(stream, data, len, /*ticks=*/0);
}

size_t ring_buffer_pop(uint8_t* out, size_t max_len, uint32_t timeout_ms) {
  if (stream == nullptr || out == nullptr || max_len == 0) return 0;
  TickType_t ticks = pdMS_TO_TICKS(timeout_ms);
  return xStreamBufferReceive(stream, out, max_len, ticks);
}

size_t ring_buffer_bytes_available() {
  return stream ? xStreamBufferBytesAvailable(stream) : 0;
}

size_t ring_buffer_bytes_free() {
  return stream ? xStreamBufferSpacesAvailable(stream) : 0;
}

}  // namespace audio
