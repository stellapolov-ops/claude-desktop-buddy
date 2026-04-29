// IMA ADPCM encoder (4-bit per sample, 4:1 compression of int16 PCM).
//
// Standard reference (verified 2026-04-29):
//   IMA Digital Audio Interchange Format (1992), open standard.
//   Wikipedia: https://en.wikipedia.org/wiki/Interactive_Multimedia_Association
//   Step / index tables are public-domain constants from the standard.
//
// Companion decoder lives at pc/m5buddy/src/audio/ima_adpcm.ts.
// Both sides written from the same standard; algorithm is bit-exact symmetric.
//
// Output format: byte = (high_nibble << 4) | (low_nibble & 0x0F)
//                where low_nibble = first sample of pair, high_nibble = second.

#pragma once

#include <stddef.h>
#include <stdint.h>

namespace audio {

struct AdpcmState {
  int16_t predictor;   // last reconstructed sample
  int8_t  step_index;  // 0..88 — index into step_table
};

// Reset state to start of a new audio segment.
void adpcm_state_reset(AdpcmState* state);

// Encode pcm_samples int16 PCM samples into packed 4-bit nibbles.
// adpcm_out must have capacity >= (pcm_samples + 1) / 2 bytes.
// `state` must persist across chunks of a single segment.
// Returns number of output bytes written.
size_t adpcm_encode(const int16_t* pcm_in, size_t pcm_samples,
                    uint8_t* adpcm_out, AdpcmState* state);

}  // namespace audio
