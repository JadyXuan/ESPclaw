#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <LovyanGFX.hpp>

#include "config.h"
#include "protocol.h"
#include "eye_renderer.h"

// ============================================================
//  Display setup — selected by #define in config.h
// ============================================================

#ifdef BOARD_JC8048W550

#include "lgfx_jc8048w550.h"
static LGFX_JC8048W550 display;
static LGFX_Sprite spriteL(&display);
static LGFX_Sprite spriteR(&display);

#else

class LGFX_Eye : public lgfx::LGFX_Device {
    lgfx::Panel_GC9A01 _panel;
    lgfx::Bus_SPI      _bus;
    lgfx::Light_PWM    _light;
public:
    LGFX_Eye(int spi_host, int sclk, int mosi, int cs, int dc, int rst, int bl) {
        auto bc        = _bus.config();
        bc.spi_host    = spi_host;
        bc.freq_write  = 80000000;
        bc.pin_sclk    = sclk;
        bc.pin_mosi    = mosi;
        bc.pin_miso    = -1;
        bc.pin_dc      = dc;
        _bus.config(bc);
        _panel.setBus(&_bus);

        auto pc           = _panel.config();
        pc.pin_cs         = cs;
        pc.pin_rst        = rst;
        pc.memory_width   = 240;
        pc.memory_height  = 240;
        pc.panel_width    = 240;
        pc.panel_height   = 240;
        _panel.config(pc);

        auto lc    = _light.config();
        lc.pin_bl  = bl;
        lc.invert  = false;
        _light.config(lc);
        _panel.setLight(&_light);

        setPanel(&_panel);
    }
};

#ifdef DUAL_DISPLAY
static LGFX_Eye displayL(SPI2_HOST, LEFT_SCLK, LEFT_MOSI, LEFT_CS, LEFT_DC, LEFT_RST, LEFT_BL);
static LGFX_Eye displayR(SPI3_HOST, RIGHT_SCLK, RIGHT_MOSI, RIGHT_CS, RIGHT_DC, RIGHT_RST, RIGHT_BL);
static LGFX_Sprite spriteL(&displayL);
static LGFX_Sprite spriteR(&displayR);
#else
static LGFX_Eye displayL(SPI2_HOST, LEFT_SCLK, LEFT_MOSI, LEFT_CS, LEFT_DC, LEFT_RST, LEFT_BL);
static LGFX_Sprite spriteL(&displayL);
static LGFX_Sprite spriteR(&displayL);
#endif

#endif // BOARD_JC8048W550

static EyeRenderer eyeL, eyeR;
static EyeState stateL, stateR;

// --- animation state ---
static float targetGazeX   = 0;
static float targetGazeY   = 0;
static float currentGazeX  = 0;
static float currentGazeY  = 0;
static float currentOpen   = 1.0f;
static float targetOpen    = 1.0f;
static float currentPupil  = 1.0f;

static bool     isBlinking   = false;
static uint32_t blinkStart   = 0;
static uint32_t nextBlinkAt  = 0;

static volatile GlovePacket gloveData = {};
static volatile bool        gloveConnected = false;
static volatile uint32_t    lastGloveTime = 0;

// ============================================================
//  ESP-NOW receive callback
// ============================================================
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
static void onEspNowRecv(const esp_now_recv_info_t* info, const uint8_t* data, int len) {
#else
static void onEspNowRecv(const uint8_t* mac, const uint8_t* data, int len) {
#endif
    if (len == sizeof(GlovePacket) && data[0] == PKT_GLOVE_DATA) {
        memcpy((void*)&gloveData, data, sizeof(GlovePacket));
        lastGloveTime = millis();
        gloveConnected = true;
    }
}

// ============================================================
//  Helpers
// ============================================================
static float mapf(float x, float inMin, float inMax, float outMin, float outMax) {
    return (x - inMin) * (outMax - outMin) / (inMax - inMin) + outMin;
}

static float easeInOut(float t) {
    return t < 0.5f ? 2.0f * t * t : 1.0f - (-2.0f * t + 2.0f) * (-2.0f * t + 2.0f) / 2.0f;
}

static void triggerBlink() {
    if (!isBlinking) {
        isBlinking = true;
        blinkStart = millis();
    }
}

// ============================================================
//  Animation update
// ============================================================
static void updateAnimation() {
    uint32_t now = millis();
    float t = now * 0.001f;

    float idleX = sinf(t * 0.7f) * 0.20f + sinf(t * 0.31f) * 0.10f;
    float idleY = sinf(t * 0.5f + 1.3f) * 0.15f + cosf(t * 0.23f) * 0.05f;
    float idlePupil = 1.0f + sinf(t * 1.5f) * 0.08f;

    if (gloveConnected && (now - lastGloveTime < 500)) {
        GlovePacket gd;
        noInterrupts();
        memcpy(&gd, (void*)&gloveData, sizeof(gd));
        interrupts();

        targetGazeX = mapf(gd.finger[0], 0, 255, 0.5f, -0.5f);
        targetGazeY = mapf(gd.finger[1], 0, 255, -0.3f, 0.3f);
        currentPupil = mapf(gd.finger[2], 0, 255, 1.0f, 0.6f);

        switch (gd.gesture) {
            case GESTURE_FIST:
                triggerBlink();
                break;
            case GESTURE_OPEN:
                targetOpen = 1.0f;
                break;
            default:
                break;
        }
    } else {
        targetGazeX = idleX;
        targetGazeY = idleY;
        currentPupil = idlePupil;
        targetOpen = 1.0f;
    }

    if (!isBlinking && now >= nextBlinkAt) {
        triggerBlink();
        nextBlinkAt = now + random(BLINK_MIN_INTERVAL_MS, BLINK_MAX_INTERVAL_MS);
    }

    if (isBlinking) {
        float bt = (float)(now - blinkStart) / BLINK_DURATION_MS;
        if (bt >= 1.0f) {
            isBlinking = false;
            currentOpen = 1.0f;
        } else if (bt < 0.40f) {
            currentOpen = 1.0f - easeInOut(bt / 0.40f);
        } else if (bt < 0.55f) {
            currentOpen = 0.0f;
        } else {
            currentOpen = easeInOut((bt - 0.55f) / 0.45f);
        }
    } else {
        currentOpen += (targetOpen - currentOpen) * 0.2f;
    }

    currentGazeX += (targetGazeX - currentGazeX) * 0.12f;
    currentGazeY += (targetGazeY - currentGazeY) * 0.12f;

    stateL.gazeX = currentGazeX;
    stateL.gazeY = currentGazeY;
    stateL.openness = currentOpen;
    stateL.pupilScale = currentPupil;

    stateR.gazeX = currentGazeX;
    stateR.gazeY = currentGazeY;
    stateR.openness = currentOpen;
    stateR.pupilScale = currentPupil;
}

// ============================================================
//  Setup
// ============================================================
void setup() {
    Serial.begin(115200);
    Serial.println("Fursuit Eyes starting...");

#ifdef BOARD_JC8048W550
    display.init();
    display.setRotation(0);
    display.setBrightness(200);
    display.fillScreen(EYE_BG_COLOR);
#else
    displayL.init();
    displayL.setRotation(0);
    displayL.setBrightness(200);
    displayL.fillScreen(EYE_BG_COLOR);
  #ifdef DUAL_DISPLAY
    displayR.init();
    displayR.setRotation(0);
    displayR.setBrightness(200);
    displayR.fillScreen(EYE_BG_COLOR);
  #endif
#endif

    spriteL.setColorDepth(16);
    spriteR.setColorDepth(16);
    spriteL.createSprite(EYE_SIZE, EYE_SIZE);
    spriteR.createSprite(EYE_SIZE, EYE_SIZE);

    EyeRenderer::Config eyeCfg;
    eyeCfg.screenSize     = EYE_SIZE;
    eyeCfg.irisHue        = IRIS_HUE;
    eyeCfg.irisSat        = IRIS_SATURATION / 100.0f;
    eyeCfg.irisBrightness = IRIS_BRIGHTNESS / 100.0f;
    eyeCfg.pupilHPct      = PUPIL_H_PCT / 100.0f;
    eyeCfg.pupilWPct      = PUPIL_W_PCT / 100.0f;
    eyeCfg.highlightPct   = HIGHLIGHT_SIZE_PCT / 100.0f;
    eyeCfg.lidShadowPct   = LID_SHADOW_PCT / 100.0f;

    eyeCfg.mirror = false;
    eyeL.init(&spriteL, eyeCfg);
    eyeCfg.mirror = true;
    eyeR.init(&spriteR, eyeCfg);

    // --- ESP-NOW ---
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE);

    if (esp_now_init() != ESP_OK) {
        Serial.println("ESP-NOW init failed");
    }
    esp_now_register_recv_cb(onEspNowRecv);

    nextBlinkAt = millis() + random(2000, 5000);

    Serial.println("Ready. Waiting for glove...");
}

// ============================================================
//  Loop
// ============================================================
void loop() {
    static uint32_t lastFrame = 0;
    uint32_t now = millis();

    uint32_t frameInterval = 1000 / TARGET_FPS;
    if (now - lastFrame < frameInterval) return;
    lastFrame = now;

    updateAnimation();

    eyeL.render(stateL);
    eyeR.render(stateR);

#ifdef BOARD_JC8048W550
    int16_t offsetY = (SCREEN_H - EYE_SIZE) / 2;  // 40px vertical centering
    spriteL.pushSprite(0, offsetY);
    spriteR.pushSprite(EYE_SIZE, offsetY);
#elif defined(DUAL_DISPLAY)
    spriteL.pushSprite(0, 0);
    spriteR.pushSprite(0, 0);
#else
    spriteL.pushSprite(0, 0);
    spriteR.pushSprite(EYE_SIZE, 0);
#endif
}
