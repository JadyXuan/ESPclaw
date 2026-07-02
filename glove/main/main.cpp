#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <Preferences.h>

#include "config.h"
#include "protocol.h"

// ============================================================
//  Globals
// ============================================================
static const uint8_t broadcastAddr[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static Preferences prefs;

static const int flexPins[NUM_FINGERS] = {FLEX_INDEX_PIN, FLEX_MIDDLE_PIN, FLEX_RING_PIN};

struct CalData {
    uint16_t minVal[NUM_FINGERS];
    uint16_t maxVal[NUM_FINGERS];
};

static CalData cal;
static float smoothed[NUM_FINGERS] = {};
static bool  calMode = false;

// ============================================================
//  Calibration
// ============================================================
static void loadCalibration() {
    prefs.begin("glove", true);
    for (int i = 0; i < NUM_FINGERS; i++) {
        char keyMin[8], keyMax[8];
        snprintf(keyMin, sizeof(keyMin), "min%d", i);
        snprintf(keyMax, sizeof(keyMax), "max%d", i);
        cal.minVal[i] = prefs.getUShort(keyMin, DEFAULT_CAL_MIN);
        cal.maxVal[i] = prefs.getUShort(keyMax, DEFAULT_CAL_MAX);
    }
    prefs.end();
}

static void saveCalibration() {
    prefs.begin("glove", false);
    for (int i = 0; i < NUM_FINGERS; i++) {
        char keyMin[8], keyMax[8];
        snprintf(keyMin, sizeof(keyMin), "min%d", i);
        snprintf(keyMax, sizeof(keyMax), "max%d", i);
        prefs.putUShort(keyMin, cal.minVal[i]);
        prefs.putUShort(keyMax, cal.maxVal[i]);
    }
    prefs.end();
}

static void runCalibration() {
    Serial.println("=== CALIBRATION MODE ===");
    Serial.println("Straighten all fingers, then slowly bend each one fully.");
    Serial.println("Press BOOT button again when done.");

    for (int i = 0; i < NUM_FINGERS; i++) {
        cal.minVal[i] = 4095;
        cal.maxVal[i] = 0;
    }

    while (true) {
        for (int i = 0; i < NUM_FINGERS; i++) {
            uint16_t raw = analogRead(flexPins[i]);
            if (raw < cal.minVal[i]) cal.minVal[i] = raw;
            if (raw > cal.maxVal[i]) cal.maxVal[i] = raw;
        }

        Serial.printf("min: [%d, %d, %d]  max: [%d, %d, %d]\r",
                       cal.minVal[0], cal.minVal[1], cal.minVal[2],
                       cal.maxVal[0], cal.maxVal[1], cal.maxVal[2]);

        if (digitalRead(CAL_BUTTON_PIN) == LOW) {
            delay(50);
            if (digitalRead(CAL_BUTTON_PIN) == LOW) {
                while (digitalRead(CAL_BUTTON_PIN) == LOW) delay(10);
                break;
            }
        }
        delay(10);
    }

    // sanity: ensure min < max with at least 100 counts range
    for (int i = 0; i < NUM_FINGERS; i++) {
        if (cal.maxVal[i] - cal.minVal[i] < 100) {
            cal.minVal[i] = DEFAULT_CAL_MIN;
            cal.maxVal[i] = DEFAULT_CAL_MAX;
        }
    }

    saveCalibration();
    Serial.println("\nCalibration saved!");
}

// ============================================================
//  Gesture detection
// ============================================================
static uint8_t detectGesture(const uint8_t finger[NUM_FINGERS]) {
    bool bent[NUM_FINGERS];
    for (int i = 0; i < NUM_FINGERS; i++) {
        bent[i] = finger[i] > BEND_THRESHOLD;
    }

    if (bent[0] && bent[1] && bent[2])   return GESTURE_FIST;
    if (!bent[0] && !bent[1] && !bent[2]) return GESTURE_OPEN;
    if (!bent[0] && bent[1] && bent[2])   return GESTURE_POINT;
    if (!bent[0] && !bent[1] && bent[2])  return GESTURE_PEACE;

    return GESTURE_NONE;
}

// ============================================================
//  Setup
// ============================================================
void setup() {
    Serial.begin(115200);
    Serial.println("Fursuit Glove starting...");

    // ADC setup
    analogReadResolution(12);
    for (int i = 0; i < NUM_FINGERS; i++) {
        pinMode(flexPins[i], INPUT);
    }

    // calibration button
    pinMode(CAL_BUTTON_PIN, INPUT_PULLUP);

    // check for calibration mode (BOOT held during startup)
    delay(100);
    if (digitalRead(CAL_BUTTON_PIN) == LOW) {
        calMode = true;
        runCalibration();
    }
    loadCalibration();

    Serial.printf("Calibration: min=[%d,%d,%d] max=[%d,%d,%d]\n",
                  cal.minVal[0], cal.minVal[1], cal.minVal[2],
                  cal.maxVal[0], cal.maxVal[1], cal.maxVal[2]);

    // init smoothing with current readings
    for (int i = 0; i < NUM_FINGERS; i++) {
        smoothed[i] = analogRead(flexPins[i]);
    }

    // ESP-NOW
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE);

    if (esp_now_init() != ESP_OK) {
        Serial.println("ESP-NOW init failed!");
        return;
    }

    esp_now_peer_info_t peer = {};
    memcpy(peer.peer_addr, broadcastAddr, 6);
    peer.channel = ESPNOW_CHANNEL;
    peer.encrypt = false;
    esp_now_add_peer(&peer);

    Serial.println("Ready. Broadcasting glove data...");
}

// ============================================================
//  Loop
// ============================================================
void loop() {
    static uint32_t lastSend = 0;
    uint32_t now = millis();

    if (now - lastSend < SEND_INTERVAL_MS) return;
    lastSend = now;

    GlovePacket pkt;
    pkt.type = PKT_GLOVE_DATA;

    for (int i = 0; i < NUM_FINGERS; i++) {
        uint16_t raw = analogRead(flexPins[i]);
        smoothed[i] += (raw - smoothed[i]) * SMOOTHING_ALPHA;

        float val = constrain(smoothed[i], cal.minVal[i], cal.maxVal[i]);
        pkt.finger[i] = (uint8_t)map((long)val, cal.minVal[i], cal.maxVal[i], 0, 255);
    }

    pkt.gesture = detectGesture(pkt.finger);

    esp_now_send(broadcastAddr, (uint8_t*)&pkt, sizeof(pkt));

    // debug output every 500ms
    static uint32_t lastPrint = 0;
    if (now - lastPrint > 500) {
        lastPrint = now;
        Serial.printf("finger: [%3d, %3d, %3d]  gesture: %d\n",
                       pkt.finger[0], pkt.finger[1], pkt.finger[2], pkt.gesture);
    }
}
