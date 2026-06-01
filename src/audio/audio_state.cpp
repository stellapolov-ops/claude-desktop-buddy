// See audio_state.h.

#include "audio_state.h"

#include <Arduino.h>
#include <string.h>

namespace audio {
namespace {

volatile State current = State::kNormal;
char awaiting_sid_buf[17] = "";   // 16 hex + NUL; "" = not awaiting

}  // namespace

State get_state() { return current; }

void set_state(State s) {
  if (current == s) return;
  Serial.printf("[audio_state] %s -> %s\n", state_name(current), state_name(s));
  current = s;
  // Leaving awaiting → sid is no longer expected. Clear so a stale
  // voice_preview can't accidentally satisfy a different sid.
  if (s != State::kAwaitingTranscript) awaiting_sid_buf[0] = 0;
}

void set_awaiting_sid(const char* sid) {
  if (!sid) { awaiting_sid_buf[0] = 0; return; }
  strncpy(awaiting_sid_buf, sid, sizeof(awaiting_sid_buf) - 1);
  awaiting_sid_buf[sizeof(awaiting_sid_buf) - 1] = 0;
}

const char* get_awaiting_sid() { return awaiting_sid_buf; }

const char* state_name(State s) {
  switch (s) {
    case State::kNormal:             return "normal";
    case State::kRecording:          return "recording";
    case State::kAwaitingTranscript: return "awaiting";
    case State::kPreview:            return "preview";
    case State::kDraftIdle:          return "draft_idle";
    case State::kApproval:           return "approval";
  }
  return "?";
}

}  // namespace audio
