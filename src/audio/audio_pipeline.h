// Audio pipeline — FreeRTOS task that drives PDM → ADPCM → ring_buffer when active.
//
// Lifecycle:
//   pipeline_init()         — create task once at boot; task sleeps until session starts
//   pipeline_start_session()— resets ADPCM state + ring_buffer; flips active=true
//   pipeline_stop_session() — flips active=false; task drops back to idle
//
// Back-pressure: if ring_buffer is full when a frame is encoded, pipeline auto-stops
// and logs the overflow. Upper layer (ble_audio_uploader, Step 3b) is expected to
// detect this via pipeline_is_active() returning false unexpectedly and emit
// voice_session_abort.

#pragma once

#include <stddef.h>
#include <stdint.h>

namespace audio {

bool     pipeline_init();
void     pipeline_start_session();
void     pipeline_stop_session();
bool     pipeline_is_active();
uint32_t pipeline_samples_encoded();  // since current session start

}  // namespace audio
