// Voice preview / error / confirm UI overlay on LCD.
//
// Day 2 Step 4 scope: renders text-free meta info only
//   - Preview mode:  "Voice Preview / N chars / A:append  B:discard"
//   - Error   mode:  "STT failed / <reason>" (3s TTL)
// Day 3 Step 5 extension:
//   - Confirm mode:  "Discard draft? / N chars / A:yes  B:no"
//     (replaces TODO 5.2's menu-based path — decision in TODO-PHASE2.md §5.2)
//
// Chinese transcript text rendering deferred to Day 5+ (M5GFX migration
// + vlw / efont integration + scroll UI). See TODO-PHASE2.md "Day 5+".
//
// Module is self-contained: caller only needs preview_active() to decide
// whether to draw buddy character; on kPreview state main loop calls
// preview_render(spr) + preview_tick(now) once per frame.

#pragma once

#include <stdint.h>

class TFT_eSprite;

namespace audio {

bool preview_active();

// Enter preview mode: cache sid + full_chars, draw "Voice Preview / N chars
// / A:append B:discard". Caller is expected to set_state(kPreview) right
// after.
void preview_set(const char* sid, int full_chars);

// Enter error mode: short reason string ("model_missing", "timeout", ...)
// shown for ~3s then auto-clears. Caller sets_state(kPreview) too.
void preview_set_error(const char* reason);

// Enter discard-confirm mode: shows "Discard draft? / N chars / A:yes B:no".
// No auto-expire (user must answer A or B). Caller sets_state(kPreview).
void preview_set_confirm(int draft_chars);

// Inspect mode for button routing: main.cpp dispatches A/B in kPreview state
// differently depending on whether it's preview / error / confirm.
enum class PreviewMode { kInactive, kPreview, kError, kConfirmDiscard };
PreviewMode preview_mode();

// Drain auto-expire timer (error mode TTL). Returns true if state changed
// from "displaying" to "cleared" this tick — caller should set_state(kNormal).
bool preview_tick(uint32_t now_ms);

// Render to the supplied sprite at the buddy's default sprite extent.
void preview_render(TFT_eSprite& spr);

// Force back to inactive (button press, approval preempt).
void preview_clear();

// Last preview sid (16-char hex + NUL) — kept so Step 5 voice_segment_append
// can echo it back to PC. Returns empty string if no preview.
const char* preview_sid();

}  // namespace audio
