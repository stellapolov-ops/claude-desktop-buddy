// See audio_state.h.

#include "audio_state.h"

#include <Arduino.h>

namespace audio {
namespace {

volatile State current = State::kNormal;

}  // namespace

State get_state() { return current; }

void set_state(State s) {
  if (current == s) return;
  Serial.printf("[audio_state] %s -> %s\n", state_name(current), state_name(s));
  current = s;
}

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
