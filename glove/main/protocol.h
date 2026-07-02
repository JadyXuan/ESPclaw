#pragma once
#include <cstdint>

#define ESPNOW_CHANNEL 0

enum PacketType : uint8_t {
    PKT_GLOVE_DATA = 0x01,
};

enum Gesture : uint8_t {
    GESTURE_NONE  = 0,
    GESTURE_OPEN  = 1,
    GESTURE_FIST  = 2,
    GESTURE_POINT = 3,
    GESTURE_PEACE = 4,
};

struct __attribute__((packed)) GlovePacket {
    uint8_t type;
    uint8_t finger[3];
    uint8_t gesture;
};
