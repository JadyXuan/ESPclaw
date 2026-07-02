#pragma once

#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include <lgfx/v1/platforms/esp32s3/Panel_RGB.hpp>
#include <lgfx/v1/platforms/esp32s3/Bus_RGB.hpp>

class LGFX_JC8048W550 : public lgfx::LGFX_Device {
    lgfx::Bus_RGB     _bus;
    lgfx::Panel_RGB   _panel;
    lgfx::Light_PWM   _light;
    lgfx::Touch_GT911 _touch;

public:
    LGFX_JC8048W550(void) {
        {
            auto cfg = _panel.config();
            cfg.memory_width  = 800;
            cfg.memory_height = 480;
            cfg.panel_width   = 800;
            cfg.panel_height  = 480;
            cfg.offset_x = 0;
            cfg.offset_y = 0;
            _panel.config(cfg);
        }
        {
            auto cfg = _panel.config_detail();
            cfg.use_psram = 1;
            _panel.config_detail(cfg);
        }

        {
            auto cfg = _bus.config();
            cfg.panel = &_panel;

            cfg.pin_d0  = GPIO_NUM_8;   // B0
            cfg.pin_d1  = GPIO_NUM_3;   // B1
            cfg.pin_d2  = GPIO_NUM_46;  // B2
            cfg.pin_d3  = GPIO_NUM_9;   // B3
            cfg.pin_d4  = GPIO_NUM_1;   // B4
            cfg.pin_d5  = GPIO_NUM_5;   // G0
            cfg.pin_d6  = GPIO_NUM_6;   // G1
            cfg.pin_d7  = GPIO_NUM_7;   // G2
            cfg.pin_d8  = GPIO_NUM_15;  // G3
            cfg.pin_d9  = GPIO_NUM_16;  // G4
            cfg.pin_d10 = GPIO_NUM_4;   // G5
            cfg.pin_d11 = GPIO_NUM_45;  // R0
            cfg.pin_d12 = GPIO_NUM_48;  // R1
            cfg.pin_d13 = GPIO_NUM_47;  // R2
            cfg.pin_d14 = GPIO_NUM_21;  // R3
            cfg.pin_d15 = GPIO_NUM_14;  // R4

            cfg.pin_henable = GPIO_NUM_40;  // DE
            cfg.pin_vsync   = GPIO_NUM_41;
            cfg.pin_hsync   = GPIO_NUM_39;
            cfg.pin_pclk    = GPIO_NUM_42;
            cfg.freq_write  = 16000000;

            cfg.hsync_polarity    = 0;
            cfg.hsync_front_porch = 8;
            cfg.hsync_pulse_width = 4;
            cfg.hsync_back_porch  = 8;
            cfg.vsync_polarity    = 0;
            cfg.vsync_front_porch = 8;
            cfg.vsync_pulse_width = 4;
            cfg.vsync_back_porch  = 8;
            cfg.pclk_active_neg   = 1;
            cfg.de_idle_high      = 0;
            cfg.pclk_idle_high    = 0;

            _bus.config(cfg);
        }
        _panel.setBus(&_bus);

        {
            auto cfg = _light.config();
            cfg.pin_bl = GPIO_NUM_2;
            _light.config(cfg);
        }
        _panel.light(&_light);

        {
            auto cfg = _touch.config();
            cfg.x_min = 0;
            cfg.x_max = 799;
            cfg.y_min = 0;
            cfg.y_max = 479;
            cfg.pin_int = GPIO_NUM_18;
            cfg.pin_rst = GPIO_NUM_38;
            cfg.bus_shared = false;
            cfg.offset_rotation = 0;
            cfg.i2c_port = I2C_NUM_0;
            cfg.pin_sda = GPIO_NUM_19;
            cfg.pin_scl = GPIO_NUM_20;
            cfg.freq = 400000;
            cfg.i2c_addr = 0x5D;
            _touch.config(cfg);
            _panel.setTouch(&_touch);
        }

        setPanel(&_panel);
    }
};
