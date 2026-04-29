// See audio_pipeline.h.

#include "audio_pipeline.h"

#include "adpcm_encoder.h"
#include "recorder.h"
#include "ring_buffer.h"

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace audio {
namespace {

constexpr uint32_t   TASK_STACK_SIZE = 4096;
constexpr UBaseType_t TASK_PRIORITY  = 1;
constexpr size_t     ADPCM_FRAME_BYTES = (FRAME_SAMPLES + 1) / 2;  // 240 bytes per 30 ms

TaskHandle_t      task_handle    = nullptr;
volatile bool     active         = false;
AdpcmState        enc_state      = {0, 0};
volatile uint32_t samples_total  = 0;

void pipeline_task(void* /*arg*/) {
  int16_t pcm_buf[FRAME_SAMPLES];
  uint8_t adpcm_buf[ADPCM_FRAME_BYTES];

  while (true) {
    if (!active) {
      vTaskDelay(pdMS_TO_TICKS(50));
      continue;
    }

    size_t got = recorder_read(pcm_buf, FRAME_SAMPLES, /*timeout_ms=*/100);
    if (got == 0) continue;

    size_t enc = adpcm_encode(pcm_buf, got, adpcm_buf, &enc_state);
    samples_total += got;

    size_t pushed = ring_buffer_push(adpcm_buf, enc);
    if (pushed != enc) {
      Serial.printf("[pipeline] ring buffer full: pushed %u/%u; session aborted\n",
                    (unsigned)pushed, (unsigned)enc);
      active = false;
      // Upper layer detects via pipeline_is_active() → emits voice_session_abort.
    }
  }
}

}  // namespace

bool pipeline_init() {
  if (task_handle != nullptr) return true;

  if (!ring_buffer_init()) return false;

  BaseType_t res = xTaskCreate(pipeline_task, "audio_pipeline",
                               TASK_STACK_SIZE, nullptr, TASK_PRIORITY,
                               &task_handle);
  if (res != pdPASS) {
    Serial.printf("[pipeline] xTaskCreate failed: %d\n", (int)res);
    return false;
  }
  Serial.printf("[pipeline] task started: stack=%u, priority=%u\n",
                (unsigned)TASK_STACK_SIZE, (unsigned)TASK_PRIORITY);
  return true;
}

void pipeline_start_session() {
  ring_buffer_reset();
  adpcm_state_reset(&enc_state);
  samples_total = 0;
  active = true;
}

void pipeline_stop_session() {
  active = false;
}

bool     pipeline_is_active()      { return active; }
uint32_t pipeline_samples_encoded() { return samples_total; }

}  // namespace audio
