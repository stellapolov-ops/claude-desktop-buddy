// See ble_audio_uploader.h for wire format spec.

#include "ble_audio_uploader.h"

#include "ring_buffer.h"

#include "../ble_bridge.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <esp_random.h>
#include <string.h>

namespace audio {
namespace {

constexpr size_t  PAYLOAD_MAX        = 240;          // one ADPCM 30 ms frame
constexpr size_t  FRAME_HEADER_BYTES = 14;
constexpr size_t  FRAME_TRAILER_BYTES = 2;
constexpr size_t  FRAME_MAX_TOTAL    = FRAME_HEADER_BYTES + PAYLOAD_MAX + FRAME_TRAILER_BYTES;
constexpr uint8_t MAGIC_LO           = 0xFE;
constexpr uint8_t MAGIC_HI           = 0xFE;
constexpr uint32_t DRAIN_TIMEOUT_MS  = 1000;

bool      in_session_     = false;
uint8_t   sid_bin_[8]     = {0};
char      sid_hex_[17]    = {0};
uint16_t  seq_            = 0;

uint16_t crc16_ccitt(const uint8_t* data, size_t len) {
  uint16_t crc = 0xFFFF;
  for (size_t i = 0; i < len; i++) {
    crc ^= (uint16_t)data[i] << 8;
    for (int j = 0; j < 8; j++) {
      crc = (crc & 0x8000) ? (uint16_t)((crc << 1) ^ 0x1021) : (uint16_t)(crc << 1);
    }
  }
  return crc;
}

void hex_encode_8(const uint8_t* in, char* out17) {
  static const char* kHex = "0123456789abcdef";
  for (int i = 0; i < 8; i++) {
    out17[i * 2]     = kHex[(in[i] >> 4) & 0x0F];
    out17[i * 2 + 1] = kHex[in[i] & 0x0F];
  }
  out17[16] = '\0';
}

bool send_json_line(JsonDocument& doc) {
  char buf[200];
  size_t n = serializeJson(doc, buf, sizeof(buf));
  if (n == 0 || n >= sizeof(buf)) return false;
  size_t written = bleWrite((const uint8_t*)buf, n);
  if (written != n) {
    Serial.printf("[uploader] bleWrite short JSON: %u/%u\n", (unsigned)written, (unsigned)n);
    return false;
  }
  if (bleWrite((const uint8_t*)"\n", 1) != 1) return false;
  Serial.printf("[uploader] %.*s\n", (int)n, buf);
  return true;
}

bool emit_binary_frame(const uint8_t* payload, size_t len) {
  if (len == 0 || len > PAYLOAD_MAX) return false;

  uint8_t frame[FRAME_MAX_TOTAL];
  frame[0] = MAGIC_LO;
  frame[1] = MAGIC_HI;
  memcpy(&frame[2], sid_bin_, 8);
  frame[10] = (uint8_t)(seq_ & 0xFF);
  frame[11] = (uint8_t)((seq_ >> 8) & 0xFF);
  frame[12] = (uint8_t)(len & 0xFF);
  frame[13] = (uint8_t)((len >> 8) & 0xFF);
  memcpy(&frame[FRAME_HEADER_BYTES], payload, len);
  uint16_t crc = crc16_ccitt(frame, FRAME_HEADER_BYTES + len);
  frame[FRAME_HEADER_BYTES + len]     = (uint8_t)(crc & 0xFF);
  frame[FRAME_HEADER_BYTES + len + 1] = (uint8_t)((crc >> 8) & 0xFF);

  size_t total   = FRAME_HEADER_BYTES + len + FRAME_TRAILER_BYTES;
  size_t written = bleWrite(frame, total);
  if (written != total) {
    Serial.printf("[uploader] bleWrite short frame: %u/%u (seq=%u)\n",
                  (unsigned)written, (unsigned)total, (unsigned)seq_);
    return false;
  }
  seq_++;
  return true;
}

}  // namespace

bool uploader_begin_session() {
  if (in_session_) return false;
  if (!bleConnected()) {
    Serial.println("[uploader] begin_session: BLE not connected");
    return false;
  }

  uint32_t r0 = esp_random();
  uint32_t r1 = esp_random();
  sid_bin_[0] = (uint8_t)(r0 & 0xFF);
  sid_bin_[1] = (uint8_t)((r0 >> 8) & 0xFF);
  sid_bin_[2] = (uint8_t)((r0 >> 16) & 0xFF);
  sid_bin_[3] = (uint8_t)((r0 >> 24) & 0xFF);
  sid_bin_[4] = (uint8_t)(r1 & 0xFF);
  sid_bin_[5] = (uint8_t)((r1 >> 8) & 0xFF);
  sid_bin_[6] = (uint8_t)((r1 >> 16) & 0xFF);
  sid_bin_[7] = (uint8_t)((r1 >> 24) & 0xFF);
  hex_encode_8(sid_bin_, sid_hex_);
  seq_ = 0;

  JsonDocument doc;
  doc["cmd"]   = "voice_start";
  doc["sid"]   = sid_hex_;
  doc["codec"] = "adpcm-ima";
  doc["rate"]  = 16000;
  doc["ch"]    = 1;
  if (!send_json_line(doc)) {
    sid_hex_[0] = '\0';
    return false;
  }
  in_session_ = true;
  return true;
}

size_t uploader_pump(uint32_t max_frames) {
  if (!in_session_) return 0;
  size_t sent = 0;
  uint8_t buf[PAYLOAD_MAX];
  for (uint32_t i = 0; i < max_frames; i++) {
    size_t got = ring_buffer_pop(buf, sizeof(buf), /*timeout_ms=*/0);
    if (got == 0) break;
    if (!emit_binary_frame(buf, got)) break;
    sent++;
  }
  return sent;
}

void uploader_end_session() {
  if (!in_session_) return;

  // Allow pipeline task to flush its current frame, then drain ring_buffer.
  delay(10);
  uint32_t deadline = millis() + DRAIN_TIMEOUT_MS;
  while (millis() < deadline && ring_buffer_bytes_available() > 0) {
    if (uploader_pump(8) == 0) break;  // BLE write failing; stop trying
  }

  JsonDocument doc;
  doc["cmd"]          = "voice_end";
  doc["sid"]          = sid_hex_;
  doc["total_chunks"] = (uint32_t)seq_;
  send_json_line(doc);

  in_session_ = false;
}

void uploader_abort_session(const char* reason) {
  if (!in_session_) return;
  ring_buffer_reset();

  JsonDocument doc;
  doc["cmd"] = "voice_session_abort";
  doc["sid"] = sid_hex_;
  if (reason != nullptr && reason[0] != '\0') doc["reason"] = reason;
  send_json_line(doc);

  in_session_ = false;
}

bool        uploader_in_session()    { return in_session_; }
const char* uploader_current_sid()   { return sid_hex_; }
uint16_t    uploader_current_seq()   { return seq_; }

}  // namespace audio
