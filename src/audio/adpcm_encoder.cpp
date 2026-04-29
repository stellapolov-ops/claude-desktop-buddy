// See adpcm_encoder.h for standard reference and byte-format spec.

#include "adpcm_encoder.h"

namespace audio {
namespace {

constexpr int16_t kStepTable[89] = {
       7,     8,     9,    10,    11,    12,    13,    14,    16,    17,
      19,    21,    23,    25,    28,    31,    34,    37,    41,    45,
      50,    55,    60,    66,    73,    80,    88,    97,   107,   118,
     130,   143,   157,   173,   190,   209,   230,   253,   279,   307,
     337,   371,   408,   449,   494,   544,   598,   658,   724,   796,
     876,   963,  1060,  1166,  1282,  1411,  1552,  1707,  1878,  2066,
    2272,  2499,  2749,  3024,  3327,  3660,  4026,  4428,  4871,  5358,
    5894,  6484,  7132,  7845,  8630,  9493, 10442, 11487, 12635, 13899,
   15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767,
};

constexpr int8_t kIndexTable[16] = {
    -1, -1, -1, -1, 2, 4, 6, 8,
    -1, -1, -1, -1, 2, 4, 6, 8,
};

inline uint8_t encode_sample(int16_t sample, AdpcmState* state) {
  int diff = (int)sample - (int)state->predictor;
  uint8_t sign = 0;
  if (diff < 0) {
    sign = 8;
    diff = -diff;
  }

  int step   = kStepTable[state->step_index];
  uint8_t delta = 0;
  int vpdiff = step >> 3;

  if (diff >= step) { delta |= 4; diff -= step; vpdiff += step; }
  step >>= 1;
  if (diff >= step) { delta |= 2; diff -= step; vpdiff += step; }
  step >>= 1;
  if (diff >= step) { delta |= 1;               vpdiff += step; }

  int new_pred = (int)state->predictor + (sign ? -vpdiff : vpdiff);
  if (new_pred >  32767) new_pred =  32767;
  if (new_pred < -32768) new_pred = -32768;
  state->predictor = (int16_t)new_pred;

  int new_idx = (int)state->step_index + kIndexTable[delta | sign];
  if (new_idx > 88) new_idx = 88;
  if (new_idx <  0) new_idx =  0;
  state->step_index = (int8_t)new_idx;

  return (uint8_t)(delta | sign);
}

}  // namespace

void adpcm_state_reset(AdpcmState* state) {
  if (state == nullptr) return;
  state->predictor  = 0;
  state->step_index = 0;
}

size_t adpcm_encode(const int16_t* pcm_in, size_t pcm_samples,
                    uint8_t* adpcm_out, AdpcmState* state) {
  if (pcm_in == nullptr || adpcm_out == nullptr || state == nullptr) return 0;

  size_t out_idx = 0;
  for (size_t i = 0; i < pcm_samples; i += 2) {
    uint8_t low  = encode_sample(pcm_in[i], state);
    uint8_t high = (i + 1 < pcm_samples) ? encode_sample(pcm_in[i + 1], state) : 0;
    adpcm_out[out_idx++] = (uint8_t)((high << 4) | (low & 0x0F));
  }
  return out_idx;
}

}  // namespace audio
