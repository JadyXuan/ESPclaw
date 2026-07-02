#pragma once
#include <LovyanGFX.hpp>
#include "config.h"

struct EyeState {
    float gazeX      = 0;    // -1 (left) to 1 (right)
    float gazeY      = 0;    // -1 (up) to 1 (down)
    float openness   = 1.0f; //  0 (closed) to 1 (open)
    float pupilScale = 1.0f;
};

class EyeRenderer {
public:
    struct Config {
        int16_t screenSize;
        float   irisHue;        // 0-360
        float   irisSat;        // 0-1
        float   irisBrightness; // 0-1
        float   pupilHPct;      // pupil height fraction of radius
        float   pupilWPct;      // pupil width fraction of radius
        float   highlightPct;   // highlight size fraction of radius
        float   lidShadowPct;   // top shadow depth fraction
        bool    mirror;
    };

    void init(lgfx::LGFX_Sprite* sprite, const Config& cfg);
    void render(const EyeState& state);
    void setHue(float hue);

private:
    lgfx::LGFX_Sprite* _sp = nullptr;
    Config   _cfg;
    int16_t  _R;

    uint16_t _gradColors[GRADIENT_BANDS];
    int16_t  _gradRadii[GRADIENT_BANDS];
    uint16_t _hlColors[HIGHLIGHT_BANDS];
    uint16_t _borderColor;
    uint16_t _shadowColor;
    uint16_t _lidEdgeColor;
    uint16_t _pupilGlow1;
    uint16_t _pupilGlow2;

    void     buildPalette();
    static uint16_t hslToRgb565(float h, float s, float l);
};
