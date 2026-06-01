// Audio state machine — tracks the M5 voice-input UX state.
// State transitions are driven by:
//   - button input (long-press B → kRecording, short-press A in kPreview → kDraftIdle)
//   - PC ack callbacks (voice_preview → kPreview, voice_error → kDraftIdle/kNormal)
//   - approval preemption (heartbeat snapshot prompt != null → kApproval)
//

#pragma once

namespace audio {

enum class State {
  kNormal,              // no draft; buddy default
  kRecording,           // long-press B held; PDM → ADPCM → ring buffer active
  kAwaitingTranscript,  // B released; waiting for PC voice_preview / voice_error
  kPreview,             // PC sent voice_preview; user can append (A) or discard (B)
  kDraftIdle,           // has un-submitted draft; LCD shows [draft: N字]
  kApproval,            // approval prompt active — mutex with all audio states
};

State        get_state();
void         set_state(State s);
const char*  state_name(State s);

// Track the sid we entered kAwaitingTranscript with so PC → M5 voice_preview /
// voice_error can be validated per §6.1.3.1 (late arrivals → sid_mismatch).
// Caller (main.cpp button release) sets this on entry; data.h reads it.
void         set_awaiting_sid(const char* sid);
const char*  get_awaiting_sid();   // 16-char hex + NUL; empty when not awaiting

}  // namespace audio
