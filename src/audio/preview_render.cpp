// See preview_render.h.
//
// State is internal to this TU; main loop only sees the bool/render hooks.
// volatile guards reads from ISR / BLE callback context (data.h _applyJson
// runs from the main loop, but on_voice_preview could in principle be wired
// up later from a BLE thread).

#include "preview_render.h"

#include <M5StickCPlus.h>   // brings in Arduino.h + TFT_eSPI / TFT_eSprite + TFT_* color macros
#include <string.h>

namespace audio {
namespace {

volatile PreviewMode mode = PreviewMode::kInactive;
char          sid_buf[17] = "";       // 16 hex chars + NUL
int           full_chars  = 0;
char          reason_buf[24] = "";    // short error code, e.g. "model_missing"
uint32_t      error_expire_at = 0;

const uint32_t ERROR_TTL_MS = 3000;
const int      W = 135;               // matches spr.createSprite(W, H) in main.cpp

}  // namespace

bool preview_active() { return mode != PreviewMode::kInactive; }

PreviewMode preview_mode() { return mode; }

void preview_set(const char* sid, int n) {
  if (sid) {
    strncpy(sid_buf, sid, sizeof(sid_buf) - 1);
    sid_buf[sizeof(sid_buf) - 1] = 0;
  } else {
    sid_buf[0] = 0;
  }
  full_chars = n;
  reason_buf[0] = 0;
  error_expire_at = 0;
  mode = PreviewMode::kPreview;
  Serial.printf("[preview] preview sid=%s chars=%d\n", sid_buf, n);
}

void preview_set_error(const char* reason) {
  strncpy(reason_buf, reason ? reason : "?", sizeof(reason_buf) - 1);
  reason_buf[sizeof(reason_buf) - 1] = 0;
  sid_buf[0] = 0;
  full_chars = 0;
  error_expire_at = millis() + ERROR_TTL_MS;
  mode = PreviewMode::kError;
  Serial.printf("[preview] error %s (TTL %lums)\n",
                reason_buf, (unsigned long)ERROR_TTL_MS);
}

void preview_set_confirm(int draft_chars) {
  sid_buf[0] = 0;
  full_chars = draft_chars;
  reason_buf[0] = 0;
  error_expire_at = 0;   // no auto-expire; user must answer
  mode = PreviewMode::kConfirmDiscard;
  Serial.printf("[preview] confirm discard chars=%d\n", draft_chars);
}

bool preview_tick(uint32_t now_ms) {
  if (mode != PreviewMode::kError) return false;
  if (error_expire_at && (int32_t)(now_ms - error_expire_at) >= 0) {
    Serial.println("[preview] error TTL expired → inactive");
    mode = PreviewMode::kInactive;
    error_expire_at = 0;
    return true;
  }
  return false;
}

void preview_render(TFT_eSprite& spr) {
  if (mode == PreviewMode::kInactive) return;

  spr.fillSprite(TFT_BLACK);
  spr.setTextDatum(MC_DATUM);

  const char* footer = "A:append  B:discard";
  if (mode == PreviewMode::kPreview) {
    spr.setTextSize(2);
    spr.setTextColor(TFT_WHITE, TFT_BLACK);
    spr.drawString("Preview", W / 2, 40);

    char buf[16];
    snprintf(buf, sizeof(buf), "%d", full_chars);
    spr.setTextSize(4);
    spr.setTextColor(TFT_YELLOW, TFT_BLACK);
    spr.drawString(buf, W / 2, 105);

    spr.setTextSize(2);
    spr.setTextColor(TFT_WHITE, TFT_BLACK);
    spr.drawString("chars", W / 2, 150);
  } else if (mode == PreviewMode::kError) {
    spr.setTextSize(2);
    spr.setTextColor(TFT_RED, TFT_BLACK);
    spr.drawString("STT failed", W / 2, 70);

    spr.setTextSize(1);
    spr.setTextColor(TFT_WHITE, TFT_BLACK);
    spr.drawString(reason_buf, W / 2, 120);
    footer = "A:dismiss  B:dismiss";
  } else {
    // kConfirmDiscard
    spr.setTextSize(2);
    spr.setTextColor(TFT_ORANGE, TFT_BLACK);
    spr.drawString("Discard", W / 2, 35);
    spr.drawString("draft?", W / 2, 60);

    char buf[16];
    snprintf(buf, sizeof(buf), "%d chars", full_chars);
    spr.setTextSize(2);
    spr.setTextColor(TFT_WHITE, TFT_BLACK);
    spr.drawString(buf, W / 2, 115);

    spr.setTextSize(1);
    spr.setTextColor(0x7BEF, TFT_BLACK);
    spr.drawString("(will be cleared)", W / 2, 145);
    footer = "A:yes  B:no";
  }

  spr.setTextSize(1);
  spr.setTextColor(0x7BEF, TFT_BLACK);
  spr.drawString(footer, W / 2, 220);

  // Restore default datum so subsequent buddy draws don't inherit MC.
  spr.setTextDatum(TL_DATUM);
}

void preview_clear() {
  mode = PreviewMode::kInactive;
  sid_buf[0] = 0;
  full_chars = 0;
  reason_buf[0] = 0;
  error_expire_at = 0;
}

const char* preview_sid() { return sid_buf; }

}  // namespace audio
