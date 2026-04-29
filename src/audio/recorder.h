// PDM microphone recorder for M5StickC Plus (SPM1423).
//
// Reference (verified 2026-04-29):
//   - Hardware pinmap: https://docs.m5stack.com/en/core/m5stickc_plus
//     "Microphone MIC (SPM1423): G0=CLK, G34=DATA"
//   - Driver pattern: m5stack/M5StickC-Plus
//     examples/Basics/Micophone/Micophone.ino
//
// Output format: 16 kHz mono signed 16-bit little-endian (s16le).
// Driver: ESP32 I2S0, Master RX PDM mode, DMA 4 buffers x 480 samples.
//
// Lifecycle:
//   recorder_init() — call once from setup() after M5.begin()
//   recorder_read() — pull samples from DMA into caller buffer

#pragma once

#include <stddef.h>
#include <stdint.h>

namespace audio {

constexpr uint32_t SAMPLE_RATE_HZ = 16000;
constexpr uint32_t FRAME_SAMPLES  = 480;  // 30 ms @ 16 kHz
constexpr uint32_t DMA_BUF_COUNT  = 4;    // 4 x 30 ms = 120 ms total DMA buffering

// Idempotent. Returns true on success.
bool recorder_init();

// Block until samples available or timeout. Returns int16 samples written
// (0 on timeout / error). Pass UINT32_MAX for indefinite wait.
size_t recorder_read(int16_t* out, size_t max_samples, uint32_t timeout_ms);

}  // namespace audio
