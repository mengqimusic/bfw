//   ▄▀▀█▄▄   ▄▀▀▀▀▄      ▄▀▀█▀▄    ▄▀▀▄▀▀▀▄  ▄▀▀▄▀▀▀▄  ▄▀▀▀▀▄   ▄▀▀▀▀▄
//  ▐ ▄▀   █ █    █      █   █  █  █   █   █ █   █   █ █      █ █      █
//    █▄▄▄▀  ▐    █      ▐   █  ▐  ▐  █▀▀▀▀  ▐  █▀▀▀▀  █      █ █      █
//    █   █      █           █        █         █      ▀▄    ▄▀ ▀▄    ▄▀
//   ▄▀▄▄▄▀    ▄▀▄▄▄▄▄▄▀  ▄▀▀▀▀▀▄   ▄▀        ▄▀         ▀▀▀▀     ▀▀▀▀
//  █    ▐     █         █       █ █         █
//  ▐          ▐         ▐       ▐ ▐         ▐
//   ▄▀▀▀█▄    ▄▀▀▀▀▄   ▄▀▀▄▀▀▀▄
//  █  ▄▀  ▀▄ █      █ █   █   █
//  ▐ █▄▄▄▄   █      █ ▐  █▀▀█▀
//   █    ▐   ▀▄    ▄▀  ▄▀    █
//   █          ▀▀▀▀   █     █
//  █                  ▐     ▐
//  ▐
//   ▄▀▀▄    ▄▀▀▄  ▄▀▀█▀▄    ▄▀▀▄ ▀▄  ▄▀▀▀▀▄    ▄▀▀█▀▄    ▄▀▀█▄▄▄▄
//  █   █    ▐  █ █   █  █  █  █ █ █ █         █   █  █  ▐  ▄▀   ▐
//  ▐  █        █ ▐   █  ▐  ▐  █  ▀█ █    ▀▄▄  ▐   █  ▐    █▄▄▄▄▄
//    █   ▄    █      █       █   █  █     █ █     █       █    ▌
//     ▀▄▀ ▀▄ ▄▀   ▄▀▀▀▀▀▄  ▄▀   █   ▐▀▄▄▄▄▀ ▐  ▄▀▀▀▀▀▄   ▄▀▄▄▄▄
//           ▀    █       █ █    ▐   ▐         █       █  █    ▐
//                ▐       ▐ ▐                  ▐       ▐  ▐

// mengqimusic.com

#include <I2Cdev.h>

#include <Preferences.h>
#define RW_MODE false
#define RO_MODE true

#include "FS.h"
#include "SPIFFS.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"

#include "AC101.h"
#include <Adafruit_AW9523.h>
#include <Wire.h>
#include "blippoo.h"
#include "WiFi.h"
#include <MIDI.h>

#define FORMAT_SPIFFS_IF_FAILED true

#define MIC_BOOST 0xAAC4 // 0xFFC4 = 48dB, 0x99C4 = 30dB, 0x88C4 = 0dB
#define DAC_VOL 0xA2A2 // 左右通道输出音量 A0 = 0dB, A2 = 1.5dB, A4 = 3dB

#define CC_RATEA 1
#define CC_RATEB 2
#define CC_R_TO_RATEA 3
#define CC_R_TO_RATEB 4
#define CC_SH_TO_RATEA 5
#define CC_SH_TO_RATEB 6
#define CC_PEAK1 7
#define CC_PEAK2 8
#define CC_R_TO_PEAK1 9
#define CC_R_TO_PEAK2 10
#define CC_SH_SP_PEAKS 11
#define CC_WET_DRY_MIX 12
#define CC_SH_SOURCE_MIX 13
#define CC_VOLUME 14
#define CC_R1_SOURCE 15
#define CC_R2_SOURCE 16

#define CC_MIDI_CH_L 20
#define CC_MIDI_CH_R 21
#define CC_MIDI_CH_BOTH 22
#define CC_A3_FREQ_MSB 23
#define CC_A3_FREQ_LSB 55

#define MSB 0
#define LSB 1

#define MIX 0
#define SH_SOURCE_MIX 1
#define VOL 2
#define R1_SOURCE 3
#define R2_SOURCE 4

#define FAST 0.01
#define SLOW 0.0025
#define BASE_OCT 1

#define SAVE false
#define LOAD true

#define slider_movement_detect 256

struct MySettings : public midi::DefaultSettings
{
  static const unsigned SysExMaxSize = 16;
  static const bool UseRunningStatus = false;
  static const bool Use1ByteParsing = true;
  static const long BaudRate = 31250;
};


Blippoo dsp(32000, 128);
AC101 ac;
Adafruit_AW9523 aw0; // left channel
Adafruit_AW9523 aw1; // right channel
MIDI_CREATE_CUSTOM_INSTANCE(HardwareSerial, Serial2, MIDI, MySettings);
TaskHandle_t controlCore;
TaskHandle_t low_priority_handle;
File patch;

Preferences prefs;


//
// hardware pins
//

const int lOctPin[2] = {6, 5}; // on aw1
const int rOctPin[2] = {13, 12}; // on aw1
const int modePin[2] = {4, 7}; // on aw1
const int sourcePin = 14; // on aw1

const int intn[2] = {23, 5};
const int rstn[2] = {22, 19};

const int potPin[3] = {34, 39, 36}, ledPin[2][2] = {{14, 13}, {21, 15}}, ledColor[4] = {0, 1, 2, 3}; // 红 紫 黄 白
const int sources[2] = {0x1040, 0x0408}; // MIC, LINE

//
// hardware values
//
int pot_val_realtime[3], pot_val_sampled[3];
int switch_val_realtime[2], switch_val_sampled[2];
bool realtime_value_valid[5] = {true, true, true, true, true};
bool sourceChanged = false, sourceChanged2 = false;
unsigned long sourceChangedMillis = 0;
int volume = 0;
volatile bool an[2] = {0, 0};
bool keyChanged = false;
bool source, key[2][12], keyPrev[2][12];
int allKeys[2] = {0, 0};
byte modeButtonState[2], mode_button_value;

bool keyFlag[2][12] = {false};

unsigned long currentMillis, startupMillis = 0, routineReadTimer = 0, save_global_settings_timer = 0;

byte prev_led_color[2];

//
// Blippoo
//
int octave = 3, octave_change;
float rateA, rateB, r_to_rateA, r_to_rateB, sh_to_rateA, sh_to_rateB, peak1, peak2, r_to_peak1, r_to_peak2, sh_sp_peaks;
float Mix, sh_source_mix, Volume; // with sliders
float base_note, base_peak;
float param_speed = FAST;
bool keyboard_mode = false, change_keyboard_mode = false, osc_current_poly = 0, filter_current_poly = 0;
//bool note_ratio_mode = false, peak_ratio_mode = false;
//int note_ratio[2] = {15, 15}, peak_ratio[2] = {15, 15};
//const float ratios[30] = {
//  0.0625, 0.066667, 0.071429, 0.076923, 0.083333, 0.090909, 0.1, 0.111111, 0.125, 0.142857, 0.166667, 0.2, 0.25, 0.333333, 0.5,
//  1.,
//  2., 3., 4., 5., 6., 7., 8., 9., 10., 11., 12., 13., 14., 15.
//};

bool saving = false, loading = false;

//
// for MIDI
//
int midi_ch_l, midi_ch_r, midi_ch_both;
byte midi_value[14][2], a3_freq_midi_value[2];
bool dirty[4] = {false};
bool poly_flip_rate, poly_flip_peak, poly_lipper;


//
// global settings
//
float a3_freq;

bool startup = true, core_print = true; // 启动淡入所用

void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_MODE_NULL);
  btStop();

  MIDI.begin(MIDI_CHANNEL_OMNI);
  MIDI.setHandleNoteOn(handleNoteOn);
  MIDI.setHandleControlChange(handleControlChange);
  MIDI.MidiInterface::turnThruOff();

  Serial.print("setup running on core ");
  Serial.println(xPortGetCoreID());

  dsp.start();

  xTaskCreatePinnedToCore(
    control,        /* Task function. */
    "control",      /* name of task. */
    10000,          /* Stack size of task */
    NULL,           /* parameter of the task */
    1,              /* priority of the task */
    &controlCore,   /* Task handle to keep track of created task */
    1);             /* pin task to core 1 */
}

void loop() {
  if (core_print) {
    core_print = false;
    Serial.print("loop running on core ");
    Serial.println(xPortGetCoreID());
  }

  MIDI.read();
}

void Pressed0() {
  an[0] = true;
}

void Pressed1() {
  an[1] = true;
}

float ratio_pitch(float ratio, float original_0_1, float base = 440.) {
  float original_note = original_0_1 * 227. - 100.;
  float original_freq = (base * exp(.057762265 * (original_note - 69.))); // mtof
  Serial.printf("original_freq = %f\n", original_freq);
  float new_freq = original_freq * ratio;
  float new_note = 12. * log10(new_freq / base) / log10(2) + 69; // ftom
  Serial.printf("new_note = %f\n", new_note);
  float new_0_1 = (new_note + 100.) / 227.;
  Serial.printf("new_0_1 = %f\n", new_0_1);
  return new_0_1;
}

float rate_to_semitone(float rate) {
  return rate * 227. - 100.;
}

float semitone_to_rate(float semitone) {
  return (semitone + 100.) / 227.;
}

float peak_to_semitone(float peak) {
  return peak * 150. - 20.;
}
