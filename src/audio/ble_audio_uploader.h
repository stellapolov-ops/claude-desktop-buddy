// BLE audio uploader — consumer of audio::ring_buffer; emits voice_* commands.
//
// Wire format (project-defined):
//   Control commands (voice_start, voice_end, voice_session_abort) → JSON line + '\n' over NUS.
//   Audio chunks → custom binary frame, distinguished from JSON by leading 0xFE 0xFE magic.
//
// Binary frame layout (one ADPCM chunk per frame; total = 16 + payload_len bytes):
//   magic     [2]  0xFE 0xFE
//   sid       [8]  raw bytes (hex form is what voice_start JSON carries)
//   seq      [2]  little-endian uint16 (per-session monotonic from 0)
//   len      [2]  little-endian uint16, payload length in bytes
//   payload   [len]
//   crc16    [2]  little-endian CRC-16/CCITT-FALSE (poly 0x1021, init 0xFFFF) over magic..payload
//
// Lifecycle (called from main.cpp button state machine):
//   uploader_begin_session()  — generate sid, emit voice_start
//   uploader_pump(N) per loop tick — drain ring_buffer to BLE binary frames
//   uploader_end_session()    — drain remainder, emit voice_end
//   uploader_abort_session()  — drop ring_buffer, emit voice_session_abort

#pragma once

#include <stddef.h>
#include <stdint.h>

namespace audio {

bool        uploader_begin_session();
size_t      uploader_pump(uint32_t max_frames);
void        uploader_end_session();
void        uploader_abort_session(const char* reason);
bool        uploader_in_session();
const char* uploader_current_sid();   // 16-char lowercase hex, NUL-terminated
uint16_t    uploader_current_seq();

}  // namespace audio
