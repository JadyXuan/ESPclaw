#pragma once

// ============================================================
//  Flex sensor ADC pins (ESP32-C3)
//  Connect each sensor with a 10k voltage divider:
//    3.3V -> [10k resistor] -> ADC_PIN -> [flex sensor] -> GND
// ============================================================
#define FLEX_INDEX_PIN   0  // GPIO0 (ADC1_CH0)
#define FLEX_MIDDLE_PIN  1  // GPIO1 (ADC1_CH1)
#define FLEX_RING_PIN    2  // GPIO2 (ADC1_CH2)

#define NUM_FINGERS 3

// ============================================================
//  Calibration defaults
//  Run calibration mode to get actual values for your sensors.
//  Hold GPIO9 (BOOT button) during power-on to enter calibration.
// ============================================================
#define CAL_BUTTON_PIN  9  // BOOT button on ESP32-C3-DevKitM-1

#define DEFAULT_CAL_MIN 500   // ADC value when finger straight
#define DEFAULT_CAL_MAX 3000  // ADC value when finger fully bent

// ============================================================
//  Filtering & timing
// ============================================================
#define SMOOTHING_ALPHA  0.15f  // EMA filter (0-1, lower = smoother)
#define SEND_INTERVAL_MS 20     // 50 Hz update rate
#define BEND_THRESHOLD   128    // 0-255, above = "bent"
