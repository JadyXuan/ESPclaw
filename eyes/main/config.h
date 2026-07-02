#pragma once

// ============================================================
//  Display Mode
//  DUAL_DISPLAY:   production mode, one GC9A01 per eye
//  SINGLE_DISPLAY: test mode, both eyes on one screen
// ============================================================
#define DUAL_DISPLAY
// #define SINGLE_DISPLAY

// ============================================================
//  GC9A01 240x240 Round Display - Pin Assignment
//  Left eye uses SPI2 (FSPI), right eye uses SPI3
// ============================================================
#ifdef DUAL_DISPLAY

// Left eye (SPI2)
#define LEFT_SCLK  12
#define LEFT_MOSI  11
#define LEFT_CS    10
#define LEFT_DC     8
#define LEFT_RST   14
#define LEFT_BL     2

// Right eye (SPI3)
#define RIGHT_SCLK 36
#define RIGHT_MOSI 35
#define RIGHT_CS   34
#define RIGHT_DC   33
#define RIGHT_RST  21
#define RIGHT_BL   47

#define SCREEN_W 240
#define SCREEN_H 240

#endif

// ============================================================
//  Single display test mode
//  Adapt these pins to your specific dev board
// ============================================================
#ifdef SINGLE_DISPLAY

#define LEFT_SCLK  12
#define LEFT_MOSI  11
#define LEFT_CS    10
#define LEFT_DC     8
#define LEFT_RST   14
#define LEFT_BL     2

#define SCREEN_W 240
#define SCREEN_H 240

#endif

// ============================================================
//  Gem-style fursuit eye appearance
// ============================================================
#define EYE_BG_COLOR      0x0000

#define IRIS_HUE          195   // HSL hue 0-360
#define IRIS_SATURATION   100   // 0-100
#define IRIS_BRIGHTNESS   79    // 0-100

#define PUPIL_H_PCT       58    // pupil ellipse height % of radius
#define PUPIL_W_PCT        30    // pupil ellipse width % of radius
#define HIGHLIGHT_SIZE_PCT 25    // main highlight size % of radius
#define LID_SHADOW_PCT     21    // top shadow depth %

#define GRADIENT_BANDS    20    // iris gradient smoothness
#define HIGHLIGHT_BANDS    5    // highlight gradient layers

// ============================================================
//  Animation timing
// ============================================================
#define TARGET_FPS        60
#define BLINK_DURATION_MS 180
#define BLINK_MIN_INTERVAL_MS 3000
#define BLINK_MAX_INTERVAL_MS 7000
