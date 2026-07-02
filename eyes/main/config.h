#pragma once

// ============================================================
//  Display Mode — uncomment ONE
// ============================================================
// #define DUAL_DISPLAY       // Production: two GC9A01 240x240 round displays
// #define SINGLE_DISPLAY     // Test: one GC9A01 with both eyes
#define BOARD_JC8048W550      // Test: Guition 5" 800x480 RGB display

// ============================================================
//  GC9A01 240x240 Round Display - Pin Assignment
// ============================================================
#ifdef DUAL_DISPLAY

#define LEFT_SCLK  12
#define LEFT_MOSI  11
#define LEFT_CS    10
#define LEFT_DC     8
#define LEFT_RST   14
#define LEFT_BL     2

#define RIGHT_SCLK 36
#define RIGHT_MOSI 35
#define RIGHT_CS   34
#define RIGHT_DC   33
#define RIGHT_RST  21
#define RIGHT_BL   47

#define SCREEN_W 240
#define SCREEN_H 240
#define EYE_SIZE 240

#endif

#ifdef SINGLE_DISPLAY

#define LEFT_SCLK  12
#define LEFT_MOSI  11
#define LEFT_CS    10
#define LEFT_DC     8
#define LEFT_RST   14
#define LEFT_BL     2

#define SCREEN_W 240
#define SCREEN_H 240
#define EYE_SIZE 120

#endif

// ============================================================
//  JC8048W550 5" 800x480 RGB Display
//  Two eyes rendered side-by-side, each 400x400
// ============================================================
#ifdef BOARD_JC8048W550

#define SCREEN_W 800
#define SCREEN_H 480
#define EYE_SIZE 400

#endif

// ============================================================
//  Gem-style fursuit eye appearance
// ============================================================
#define EYE_BG_COLOR      0x0000

#define IRIS_HUE          195
#define IRIS_SATURATION   100
#define IRIS_BRIGHTNESS   79

#define PUPIL_H_PCT       58
#define PUPIL_W_PCT       30
#define HIGHLIGHT_SIZE_PCT 25
#define LID_SHADOW_PCT    21

#define GRADIENT_BANDS    20
#define HIGHLIGHT_BANDS    5

// ============================================================
//  Animation timing
// ============================================================
#define TARGET_FPS        60
#define BLINK_DURATION_MS 180
#define BLINK_MIN_INTERVAL_MS 3000
#define BLINK_MAX_INTERVAL_MS 7000
