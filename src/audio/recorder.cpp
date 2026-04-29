// See recorder.h for protocol references and output format.

#include "recorder.h"

#include <Arduino.h>
#include <driver/i2s.h>

namespace audio {
namespace {

constexpr i2s_port_t I2S_PORT = I2S_NUM_0;
constexpr int        PIN_CLK  = 0;   // SPM1423 PDM clock (per official PinMap)
constexpr int        PIN_DATA = 34;  // SPM1423 PDM data  (per official PinMap)

bool initialized = false;

}  // namespace

bool recorder_init() {
  if (initialized) return true;

  i2s_config_t cfg = {};
  cfg.mode             = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_PDM);
  cfg.sample_rate      = SAMPLE_RATE_HZ;
  cfg.bits_per_sample  = I2S_BITS_PER_SAMPLE_16BIT;
  // PDM mic feeds right channel; i2s_set_clk(MONO) below reduces to one sample/frame.
  cfg.channel_format   = I2S_CHANNEL_FMT_ALL_RIGHT;
#if ESP_IDF_VERSION > ESP_IDF_VERSION_VAL(4, 1, 0)
  cfg.communication_format = I2S_COMM_FORMAT_STAND_I2S;
#else
  cfg.communication_format = I2S_COMM_FORMAT_I2S;
#endif
  cfg.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
  cfg.dma_buf_count    = DMA_BUF_COUNT;
  cfg.dma_buf_len      = FRAME_SAMPLES;

  esp_err_t err = i2s_driver_install(I2S_PORT, &cfg, 0, nullptr);
  if (err != ESP_OK) {
    Serial.printf("[recorder] i2s_driver_install err=%d\n", err);
    return false;
  }

  i2s_pin_config_t pins = {};
#if ESP_IDF_VERSION > ESP_IDF_VERSION_VAL(4, 3, 0)
  pins.mck_io_num = I2S_PIN_NO_CHANGE;
#endif
  pins.bck_io_num   = I2S_PIN_NO_CHANGE;
  pins.ws_io_num    = PIN_CLK;
  pins.data_in_num  = PIN_DATA;
  pins.data_out_num = I2S_PIN_NO_CHANGE;

  err = i2s_set_pin(I2S_PORT, &pins);
  if (err != ESP_OK) {
    Serial.printf("[recorder] i2s_set_pin err=%d\n", err);
    i2s_driver_uninstall(I2S_PORT);
    return false;
  }

  err = i2s_set_clk(I2S_PORT, SAMPLE_RATE_HZ, I2S_BITS_PER_SAMPLE_16BIT,
                    I2S_CHANNEL_MONO);
  if (err != ESP_OK) {
    Serial.printf("[recorder] i2s_set_clk err=%d\n", err);
    i2s_driver_uninstall(I2S_PORT);
    return false;
  }

  initialized = true;
  Serial.printf("[recorder] PDM init OK: %u Hz mono s16le, DMA %ux%u samples\n",
                (unsigned)SAMPLE_RATE_HZ, (unsigned)DMA_BUF_COUNT,
                (unsigned)FRAME_SAMPLES);
  return true;
}

size_t recorder_read(int16_t* out, size_t max_samples, uint32_t timeout_ms) {
  if (!initialized || out == nullptr || max_samples == 0) return 0;

  size_t bytes_read = 0;
  TickType_t ticks =
      (timeout_ms == UINT32_MAX) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
  esp_err_t err = i2s_read(I2S_PORT, out, max_samples * sizeof(int16_t),
                           &bytes_read, ticks);
  if (err != ESP_OK) return 0;
  return bytes_read / sizeof(int16_t);
}

}  // namespace audio
