/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  Example of using UnitWeightI2C and UnitMiniScales via UnitPaHub

  Board ---> PaHub (0x70)
               |- ch:0 UnitWeightI2C  (0x26)
               `- ch:1 UnitMiniScales (0x26)

  Both units use the same I2C address (0x26).
  Each PaHub channel is an independent I2C bus, so they can be used at the same time.

  The screen is split into two: UnitWeightI2C (top) and UnitMiniScales (bottom).
  BtnA: hold to tare both units
  The button of UnitMiniScales tares by the unit firmware itself; the example only shows TARE.
  The LED of UnitMiniScales lights in its display color, brighter as the weight increases (up to 5 kg).
*/
#include <algorithm>
#include <cmath>
#include <string>

#include <M5Unified.h>
#include <M5UnitUnified.h>
#include <M5UnitUnifiedHUB.h>  // UnitPaHub
#include <M5UnitUnifiedWEIGHT.h>
#include <M5Utility.h>
#include <wiring/m5_unit_unified_wiring.hpp>  // Board-aware connection helpers (include last)

namespace {
auto& lcd = M5.Display;
m5::unit::UnitUnified Units;

// The default address of PaHub is 0x70
m5::unit::UnitPaHub hub;

m5::unit::UnitWeightI2C weight_i2c;
m5::unit::UnitMiniScales mini_scales;

// PaHub channels used by each unit
constexpr uint8_t CH_WEIGHT_I2C{0};
constexpr uint8_t CH_MINI_SCALES{1};

constexpr uint32_t weight_i2c_color{0x66CCFFU};   // Sky blue (RGB888)
constexpr uint32_t mini_scales_color{0x00CC99U};  // Mint (RGB888)
constexpr float WEIGHT_MAX{5000.0f};              // Full scale of the bar gauge and the LED (g)
constexpr uint32_t notice_duration{1000};         // How long a notice is shown (ms)

bool has_lcd{};

// One unit on half of the screen: label, unit, weight and a bar gauge
class View {
public:
    View(LovyanGFX& dst, const char* label, const uint32_t color) : _label{label}
    {
        _sprite.setPsram(false);
        _sprite.setColorDepth(1);  // Two colors: background and text
        _w     = dst.width();
        _h     = dst.height() >> 1;
        _valid = _sprite.createSprite(_w, _h);
        if (_valid) {
            _sprite.setPaletteColor(0, TFT_BLACK);
            _sprite.setPaletteColor(1, color);
            _sprite.setTextColor(1, 0);
        }
    }

    bool valid() const
    {
        return _valid;
    }

    void set(const float weight)
    {
        const std::string text = m5::utility::formatString("%.2f", weight);
        if (text != _text) {
            _text   = text;
            _weight = weight;
            _dirty  = true;
        }
    }

    void notice(const char* text)
    {
        _notice    = text;
        _notice_at = m5::utility::millis();
        _dirty     = true;
    }

    void update()
    {
        if (_notice && m5::utility::hasElapsed(_notice_at, notice_duration)) {
            _notice = nullptr;
            _dirty  = true;
        }
    }

    void push(LovyanGFX* dst, const int32_t x, const int32_t y)
    {
        if (!_dirty) {
            return;
        }
        _dirty = false;
        _sprite.fillSprite(0);

        const int32_t m{3};
        const int32_t bar_h = std::max<int32_t>(_h / 8, 4);
        const int32_t bar_y = _h - bar_h - m;
        const int32_t bar_w = _w - m * 2;

        // Bar gauge: outline + proportional fill (0 to WEIGHT_MAX)
        _sprite.drawRect(m, bar_y, bar_w, bar_h, 1);
        const float t      = std::fmin(1.0f, std::fmax(0.0f, _weight / WEIGHT_MAX));
        const int32_t fill = static_cast<int32_t>((bar_w - 2) * t);
        if (fill > 0) {
            _sprite.fillRect(m + 1, bar_y + 1, fill, bar_h - 2, 1);
        }

        // Small labels: name (top-left), unit (top-right)
        _sprite.setFont(&fonts::Font0);
        _sprite.setTextSize(1);
        _sprite.setTextDatum(top_left);
        _sprite.drawString(_label, m + 1, m);
        _sprite.setTextDatum(top_right);
        _sprite.drawString("g", _w - m - 1, m);

        // Large weight (or the notice), centered above the bar
        _sprite.setFont(&fonts::Orbitron_Light_32);
        _sprite.setTextSize(1);
        const int32_t text_top{10};
        const int32_t text_area = bar_y - text_top;
        const float sx          = _w * 0.9f / _sprite.textWidth("-0000.00");
        const float sy          = text_area / static_cast<float>(_sprite.fontHeight());
        _sprite.setTextSize(std::max(0.3f, std::min(sx, sy)));
        _sprite.setTextDatum(middle_center);
        _sprite.drawString(_notice ? _notice : _text.c_str(), _w / 2, text_top + text_area / 2);

        _sprite.pushSprite(dst, x, y);
    }

private:
    LGFX_Sprite _sprite{};
    const char* _label{};
    std::string _text{};
    float _weight{};
    const char* _notice{};
    m5::utility::elapsed_time_t _notice_at{};
    int32_t _w{}, _h{};
    bool _valid{}, _dirty{true};
};
View* view[2]{};

// LED shows the display color of UnitMiniScales, brighter as the weight increases
void update_led(const float weight)
{
    const float t     = std::fmin(1.0f, std::fmax(0.0f, weight / WEIGHT_MAX));
    const float level = 0.1f + 0.9f * t;  // Keep it dimly lit at 0 g
    const uint32_t c  = mini_scales_color;
    const uint8_t r   = static_cast<uint8_t>(((c >> 16) & 0xFF) * level);
    const uint8_t g   = static_cast<uint8_t>(((c >> 8) & 0xFF) * level);
    const uint8_t b   = static_cast<uint8_t>((c & 0xFF) * level);
    static uint32_t last{0xFFFFFFFFU};  // Not a valid RGB, so the first call always writes
    const uint32_t rgb = (static_cast<uint32_t>(r) << 16) | (static_cast<uint32_t>(g) << 8) | b;
    if (rgb != last) {  // Write only when the color changes
        last = rgb;
        mini_scales.writeLEDColor(r, g, b);
    }
}

void show_notice(const uint32_t idx, const char* text)
{
    if (has_lcd) {
        view[idx]->notice(text);
    }
}

}  // namespace

void setup()
{
    M5.begin();
    M5.setTouchButtonHeightByRatio(100);

    // The screen shall be in landscape mode
    if (lcd.height() > lcd.width()) {
        lcd.setRotation(1);
    }

    if (!hub.add(weight_i2c, CH_WEIGHT_I2C) || !hub.add(mini_scales, CH_MINI_SCALES)) {
        M5_LOGE("Failed to add");
        m5::unit::wiring::failStop();
    }

    // The hub is the I2C device on the port
    // NessoN1 -> SoftwareI2C on GROVE, NanoC6/NanoH2 -> M5.Ex_I2C, others -> Wire
    if (!m5::unit::wiring::addI2C(Units, hub) || !Units.begin()) {
        M5_LOGE("Failed to begin");
        M5_LOGW("%s", Units.debugInfo().c_str());
        m5::unit::wiring::failStop();
    }

    weight_i2c.resetOffset();
    mini_scales.resetOffset();

    M5_LOGI("M5UnitUnified initialized");
    M5_LOGI("%s", Units.debugInfo().c_str());
    M5_LOGI("BtnA: hold to tare both units");

    // Display setup (skip for no-LCD or EPD devices)
    has_lcd = lcd.width() > 0 && lcd.height() > 0 && !lcd.isEPD();
    if (has_lcd) {
        lcd.fillScreen(TFT_BLACK);
        view[0] = new View(lcd, "WeightI2C", weight_i2c_color);
        view[1] = new View(lcd, "MiniScales", mini_scales_color);
        has_lcd = view[0]->valid() && view[1]->valid();
    }
    update_led(0.0f);
}

void loop()
{
    M5.update();
    Units.update();

    // Can be checked e.g. by serial plotters
    if (weight_i2c.updated()) {
        const float w = weight_i2c.weight();
        M5.Log.printf(">WeightI2C:%f\n", w);
        if (has_lcd) {
            view[0]->set(w);
        }
    }
    if (mini_scales.updated()) {
        const float w = mini_scales.weight();
        M5.Log.printf(">MiniScales:%f\n", w);
        update_led(w);
        if (has_lcd) {
            view[1]->set(w);
        }
    }

    // Tare both units
    if (M5.BtnA.wasHold()) {
        const bool ok_w = weight_i2c.resetOffset();
        const bool ok_m = mini_scales.resetOffset();
        M5.Log.printf("Tare: WeightI2C %s, MiniScales %s\n", ok_w ? "OK" : "FAILED", ok_m ? "OK" : "FAILED");
        show_notice(0, ok_w ? "TARE" : "FAILED");
        show_notice(1, ok_m ? "TARE" : "FAILED");
    }

    // Button on MiniScales: the unit firmware tares while it is pressed, so just show it
    if (mini_scales.wasPressed()) {
        M5.Log.printf("MiniScales button: tare\n");
        show_notice(1, "TARE");
    }

    if (has_lcd) {
        lcd.startWrite();
        for (uint32_t i = 0; i < 2; ++i) {
            view[i]->update();
            view[i]->push(&lcd, 0, i == 0 ? 0 : lcd.height() >> 1);
        }
        lcd.endWrite();
    }
}

#if !defined(ARDUINO)
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_timer.h>

#if CONFIG_FREERTOS_UNICORE
// Single-core SoCs: run loop() back-to-back, but every ~2 s yield a 5 ms slice so the IDLE task
// runs and feeds the task watchdog (default 5 s).
static inline void feedIdleTaskPeriodically(void)
{
    constexpr uint32_t FEED_INTERVAL_MS{2000};
    constexpr TickType_t FEED_SLEEP_TICKS{pdMS_TO_TICKS(5)};
    static uint32_t s_last_feed_ms{};
    const uint32_t now_ms{static_cast<uint32_t>(esp_timer_get_time() / 1000)};
    if (now_ms - s_last_feed_ms >= FEED_INTERVAL_MS) {
        s_last_feed_ms = now_ms;
        vTaskDelay(FEED_SLEEP_TICKS);
    }
}
#endif

extern "C" void app_main(void)
{
    setup();
    for (;;) {
#if CONFIG_FREERTOS_UNICORE
        feedIdleTaskPeriodically();
#endif
        loop();
    }
}
#endif
