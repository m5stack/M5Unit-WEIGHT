/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  Example using M5UnitUnified for UnitMiniScales

  Prints the weight of each measurement to the serial output (Serial Plotter format),
  and draws it when the board has a screen.

  BtnA: click to run the selected function, double click to select the next function, hold to tare
  Functions: Float (periodic, float) -> Int (periodic, weight x100) -> Singleshot (one text measurement per click)
  The LED of the unit lights in the function color, brighter as the weight increases (up to 5 kg).
  The button of the unit tares by the unit firmware itself; the example only shows TARE.
*/
#include <cinttypes>
#include <cmath>
#include <cstdlib>
#include <string>

#include <M5Unified.h>
#include <M5UnitUnified.h>
#include <M5UnitUnifiedWEIGHT.h>
#include <M5Utility.h>
#include <wiring/m5_unit_unified_wiring.hpp>  // Board-aware connection helpers (include last)

using m5::unit::weighti2c::Mode;

namespace {
auto& lcd = M5.Display;
m5::unit::UnitUnified Units;
m5::unit::UnitMiniScales unit;

constexpr uint32_t notice_duration{1000};  // How long a notice is shown (ms)

LGFX_Sprite sprite;
bool has_lcd{};
float number_scale{1.0f};  // Text size of the weight
float label_scale{1.0f};   // Text size of the function state
std::string value_text{};  // Weight shown at the center
bool redraw{true};
const char* notice{};  // Shown instead of the weight for a while (e.g. after tare)
m5::utility::elapsed_time_t notice_at{};

// BtnA: single click runs the selected function, double click selects the next one, hold tares
enum class Function : uint8_t { Float, Int, Singleshot };
constexpr const char* function_name[] = {"Float", "Int (x100)", "Singleshot"};
// Text color for each function (RGB888)
constexpr uint32_t function_color[] = {0x66CCFFU /* Sky blue */, 0xFFB400U /* Orange */, 0xCC66FFU /* Lavender */};

Function function{Function::Float};
uint32_t ss_count{};

constexpr float WEIGHT_MIN{0.0f};
constexpr float WEIGHT_MAX{5000.0f};  // MiniScales: 5kg load cell
float last_weight{};                  // Latest weight (g) for the LED

// LED shows the function color (same as the text), brighter as the weight increases
void update_led()
{
    const float t     = std::fmin(1.0f, std::fmax(0.0f, (last_weight - WEIGHT_MIN) / (WEIGHT_MAX - WEIGHT_MIN)));
    const float level = 0.1f + 0.9f * t;  // Keep it dimly lit at 0 g
    const uint32_t c  = function_color[m5::stl::to_underlying(function)];
    const uint8_t r   = static_cast<uint8_t>(((c >> 16) & 0xFF) * level);
    const uint8_t g   = static_cast<uint8_t>(((c >> 8) & 0xFF) * level);
    const uint8_t b   = static_cast<uint8_t>((c & 0xFF) * level);
    static uint32_t last{0xFFFFFFFFU};  // Not a valid RGB, so the first call always writes
    const uint32_t rgb = (static_cast<uint32_t>(r) << 16) | (static_cast<uint32_t>(g) << 8) | b;
    if (rgb != last) {  // Write only when the color changes
        last = rgb;
        unit.writeLEDColor(r, g, b);
    }
}

bool start_periodic()
{
    const Mode mode = (function == Function::Int) ? Mode::Int : Mode::Float;
    return unit.startPeriodicMeasurement(mode, unit.config().interval);
}

// Selected function and its current state
std::string function_state()
{
    if (function == Function::Singleshot) {
        return m5::utility::formatString("Singleshot #%" PRIu32, ss_count);
    }
    return function_name[m5::stl::to_underlying(function)];
}

void run_function()
{
    if (function != Function::Singleshot) {
        return;  // Float / Int: nothing to do
    }
    char buf[16]{};  // The unit returns up to 15 characters
    if (unit.measureSingleshot(buf)) {
        ++ss_count;
        M5.Log.printf(">Singleshot:%s\n", buf);
        value_text  = buf;
        last_weight = std::strtof(buf, nullptr);
        update_led();
    } else {
        M5_LOGE("Failed to measure");
    }
    redraw = true;
}

void select_next_function()
{
    auto next = m5::stl::to_underlying(function) + 1;
    if (next >= m5::stl::size(function_name)) {
        next = 0;
    }
    function = static_cast<Function>(next);

    // Singleshot measures only while periodic measurement is stopped
    unit.stopPeriodicMeasurement();
    if (function == Function::Singleshot) {
        ss_count = 0;
    } else {
        start_periodic();
    }
    value_text.clear();
    update_led();  // Change the LED to the color of the new function
    M5.Log.printf("Function: %s\n", function_state().c_str());
    redraw = true;
}

void tare()
{
    const bool success = unit.resetOffset();
    M5.Log.printf("Tare: %s\n", success ? "OK" : "FAILED");
    notice    = success ? "TARE" : "FAILED";
    notice_at = m5::utility::millis();
    redraw    = true;
}

void handle_button()
{
    if (M5.BtnA.wasSingleClicked()) {
        run_function();
    } else if (M5.BtnA.wasDoubleClicked()) {
        select_next_function();
    } else if (M5.BtnA.wasHold()) {
        tare();
    }
}

void draw_lcd()
{
    sprite.setPaletteColor(1, function_color[m5::stl::to_underlying(function)]);
    sprite.fillSprite(0);
    sprite.setFont(&fonts::Orbitron_Light_32);
    sprite.setTextDatum(middle_center);
    sprite.setTextSize(number_scale, number_scale);
    if (notice) {
        sprite.drawString(notice, sprite.width() / 2, sprite.height() / 2);
    } else if (!value_text.empty()) {
        sprite.drawString(value_text.c_str(), sprite.width() / 2, sprite.height() / 2);
    }
    sprite.setFont(&fonts::Font0);
    sprite.setTextSize(label_scale, label_scale);
    sprite.setTextDatum(top_left);
    sprite.drawString(function_state().c_str(), 2, 2);
    sprite.setTextDatum(bottom_right);
    // Int shows the raw value of the unit (weight x100)
    sprite.drawString(function == Function::Int ? "x0.01 g" : "g", sprite.width() - 2, sprite.height() - 2);
    sprite.pushSprite(&lcd, 0, 0);
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

    // Board-aware connection: NessoN1 -> SoftwareI2C on GROVE, NanoC6/NanoH2 -> M5.Ex_I2C, others -> Wire
    if (!m5::unit::wiring::addI2C(Units, unit) || !Units.begin()) {
        M5_LOGE("Failed to begin");
        m5::unit::wiring::failStop();
    }
    float gap{};
    unit.readGap(gap);
    M5_LOGI("GAP:%f", gap);

#if 0
    // Restore factory GAP (enable temporarily if GAP was corrupted)
    unit.writeGap(400.0f);
    M5_LOGW("Wrote GAP=400.0");
#endif

    unit.resetOffset();

    M5_LOGI("M5UnitUnified initialized");
    M5_LOGI("%s", Units.debugInfo().c_str());
    M5_LOGI("BtnA: click to run, double click to select the function, hold to tare");

    // Display setup (skip for no-LCD or EPD devices)
    has_lcd = lcd.width() > 0 && lcd.height() > 0 && !lcd.isEPD();
    if (has_lcd) {
        sprite.setPsram(false);
        sprite.setColorDepth(1);  // Two colors: background and text
        has_lcd = sprite.createSprite(lcd.width(), lcd.height());
    }
    if (has_lcd) {
        sprite.setPaletteColor(0, TFT_BLACK);
        sprite.setTextColor(1, 0);
        // Fit "-0000.00" into 90% of the width
        sprite.setFont(&fonts::Orbitron_Light_32);
        sprite.setTextSize(1);
        number_scale = sprite.width() * 0.9f / sprite.textWidth("-0000.00");
        label_scale  = (lcd.width() >= 320) ? 2.0f : 1.0f;
        sprite.fillSprite(0);
        sprite.pushSprite(&lcd, 0, 0);
    }
}

void loop()
{
    M5.update();
    Units.update();

    if (unit.updated()) {
        // Can be checked e.g. by serial plotters
        std::string text{};
        if (function == Function::Int) {
            const int32_t iw = unit.iweight();
            M5.Log.printf(">iWeight:%" PRId32 "\n", iw);
            last_weight = iw / 100.0f;
            text        = m5::utility::formatString("%" PRId32, iw);  // Raw value (weight x100)
        } else {
            last_weight = unit.weight();
            M5.Log.printf(">Weight:%f\n", last_weight);
            text = m5::utility::formatString("%.2f", last_weight);
        }
        update_led();
        if (text != value_text) {
            value_text = text;
            redraw     = true;
        }
    }

    // Button on MiniScales: the unit firmware tares while it is pressed, so just show it
    if (unit.wasPressed()) {
        M5.Log.printf("Unit button: tare\n");
        notice    = "TARE";
        notice_at = m5::utility::millis();
        redraw    = true;
    }

    handle_button();

    if (notice && m5::utility::hasElapsed(notice_at, notice_duration)) {
        notice = nullptr;
        redraw = true;
    }
    if (has_lcd && redraw) {
        redraw = false;
        draw_lcd();
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
