#include "eye_renderer.h"
#include <math.h>

// ----------------------------------------------------------------
//  HSL → RGB565
// ----------------------------------------------------------------
uint16_t EyeRenderer::hslToRgb565(float h, float s, float l) {
    if (s <= 0.0f) {
        uint8_t v = (uint8_t)(l * 255);
        return ((v >> 3) << 11) | ((v >> 2) << 5) | (v >> 3);
    }
    float c  = (1.0f - fabsf(2.0f * l - 1.0f)) * s;
    float x  = c * (1.0f - fabsf(fmodf(h / 60.0f, 2.0f) - 1.0f));
    float m  = l - c * 0.5f;
    float r1, g1, b1;
    if      (h < 60)  { r1 = c; g1 = x; b1 = 0; }
    else if (h < 120) { r1 = x; g1 = c; b1 = 0; }
    else if (h < 180) { r1 = 0; g1 = c; b1 = x; }
    else if (h < 240) { r1 = 0; g1 = x; b1 = c; }
    else if (h < 300) { r1 = x; g1 = 0; b1 = c; }
    else              { r1 = c; g1 = 0; b1 = x; }
    uint8_t r = (uint8_t)((r1 + m) * 255);
    uint8_t g = (uint8_t)((g1 + m) * 255);
    uint8_t b = (uint8_t)((b1 + m) * 255);
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
}

// ----------------------------------------------------------------
//  Pre-compute every colour the renderer uses
// ----------------------------------------------------------------
void EyeRenderer::buildPalette() {
    float h = _cfg.irisHue;
    float s = _cfg.irisSat;
    float b = _cfg.irisBrightness;

    // iris gradient: band 0 = outermost (dark), band N-1 = center (bright)
    for (int i = 0; i < GRADIENT_BANDS; i++) {
        float t = (float)i / (GRADIENT_BANDS - 1);
        float l = b * (0.03f + 0.52f * t);
        float sat = s * (1.0f - t * 0.10f);
        _gradColors[i] = hslToRgb565(h, sat, l);
        _gradRadii[i]  = _R - (int16_t)(_R * 0.97f * t);
        if (_gradRadii[i] < 1) _gradRadii[i] = 1;
    }

    // highlight: outer glow → white core
    for (int i = 0; i < HIGHLIGHT_BANDS; i++) {
        float t = (float)i / (HIGHLIGHT_BANDS - 1);
        float l  = 0.35f + t * 0.60f;
        float hs = s * (1.0f - t * 0.85f);
        _hlColors[i] = hslToRgb565(h, hs, l);
    }

    _borderColor = hslToRgb565(h, s * 0.6f, 0.08f);
    _shadowColor = hslToRgb565(h, s * 0.5f, b * 0.06f);
    _lidEdgeColor = hslToRgb565(h, s * 0.5f, b * 0.25f + 0.10f);
    _pupilGlow1  = _gradColors[GRADIENT_BANDS / 2];
    _pupilGlow2  = _gradColors[GRADIENT_BANDS / 3];
}

// ----------------------------------------------------------------
//  Init
// ----------------------------------------------------------------
void EyeRenderer::init(lgfx::LGFX_Sprite* sprite, const Config& cfg) {
    _sp  = sprite;
    _cfg = cfg;
    _R   = cfg.screenSize / 2;
    buildPalette();
}

void EyeRenderer::setHue(float hue) {
    _cfg.irisHue = hue;
    buildPalette();
}

// ----------------------------------------------------------------
//  Render one frame
// ----------------------------------------------------------------
void EyeRenderer::render(const EyeState& state) {
    auto& sp = *_sp;
    int16_t S  = _cfg.screenSize;
    int16_t R  = _R;
    int16_t cx = R;
    int16_t cy = R;

    sp.fillScreen(EYE_BG_COLOR);

    // 1 ── iris radial gradient (outer→inner, each overwrites center)
    for (int i = 0; i < GRADIENT_BANDS; i++) {
        sp.fillCircle(cx, cy, _gradRadii[i], _gradColors[i]);
    }

    // 2 ── top lid shadow (dark ellipse mostly above viewport)
    if (_cfg.lidShadowPct > 0.01f) {
        int16_t sy = -(int16_t)(R * (1.0f - _cfg.lidShadowPct * 1.5f));
        sp.fillEllipse(cx, sy, (int16_t)(R * 1.2f), R, _shadowColor);
    }

    // 3 ── pupil (vertical ellipse)
    float maxOx = R * 0.35f;
    float maxOy = R * 0.25f;
    int16_t px = cx + (int16_t)(state.gazeX * maxOx);
    int16_t py = cy + (int16_t)(state.gazeY * maxOy);
    int16_t pw = (int16_t)(R * _cfg.pupilWPct * state.pupilScale);
    int16_t ph = (int16_t)(R * _cfg.pupilHPct * state.pupilScale);
    if (pw < 2) pw = 2;
    if (ph < 2) ph = 2;

    sp.fillEllipse(px, py, pw, ph, (uint16_t)0x0000);

    // pupil edge glow (thin rings)
    sp.drawEllipse(px, py, pw + 1, ph + 1, _pupilGlow1);
    sp.drawEllipse(px, py, pw + 2, ph + 2, _pupilGlow2);

    // 4 ── main highlight (large oval, upper area)
    int16_t hlR  = (int16_t)(R * _cfg.highlightPct);
    int16_t hlX  = _cfg.mirror ? cx + (int16_t)(R * 0.22f)
                               : cx - (int16_t)(R * 0.22f);
    int16_t hlY  = cy - (int16_t)(R * 0.28f);
    int16_t hlRx = (int16_t)(hlR * 0.70f);
    int16_t hlRy = hlR;

    for (int i = 0; i < HIGHLIGHT_BANDS; i++) {
        float t   = (float)i / (HIGHLIGHT_BANDS - 1);
        int16_t rx = (int16_t)(hlRx * (1.0f - t * 0.55f));
        int16_t ry = (int16_t)(hlRy * (1.0f - t * 0.55f));
        if (rx < 1) rx = 1;
        if (ry < 1) ry = 1;
        sp.fillEllipse(hlX, hlY, rx, ry, _hlColors[i]);
    }

    // 5 ── secondary small highlight
    int16_t sh2X = _cfg.mirror ? cx - (int16_t)(R * 0.25f)
                               : cx + (int16_t)(R * 0.25f);
    int16_t sh2Y = cy + (int16_t)(R * 0.30f);
    sp.fillCircle(sh2X, sh2Y, (int16_t)(R * 0.055f), (uint16_t)0xFFFF);

    // 6 ── tiny sparkle dot
    int16_t spkX = _cfg.mirror ? cx - (int16_t)(R * 0.10f)
                               : cx + (int16_t)(R * 0.10f);
    int16_t spkY = cy - (int16_t)(R * 0.05f);
    sp.fillCircle(spkX, spkY, 2, (uint16_t)0xFFFF);

    // 7 ── eyelids (elliptical arc closing)
    if (state.openness < 0.99f) {
        float closed = 1.0f - state.openness;
        int16_t eRx  = (int16_t)(R * 1.15f);

        int16_t topY = -R + (int16_t)(R * 2.0f * closed);
        sp.fillEllipse(cx, topY, eRx, R, (uint16_t)EYE_BG_COLOR);

        int16_t botY = S + R - (int16_t)(R * 2.0f * closed);
        sp.fillEllipse(cx, botY, eRx, R, (uint16_t)EYE_BG_COLOR);

        if (state.openness > 0.05f && state.openness < 0.95f) {
            sp.drawEllipse(cx, topY, (int16_t)(R * 1.10f), R, _lidEdgeColor);
        }
    }

    // 8 ── outer border ring
    sp.drawCircle(cx, cy, R - 1, _borderColor);
    sp.drawCircle(cx, cy, R - 2, _borderColor);
}
