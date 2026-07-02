#pragma once
#include <cstdint>

#define ESPNOW_CHANNEL 0

enum PacketType : uint8_t {
    PKT_GLOVE_DATA = 0x01,
};

enum Gesture : uint8_t {
    GESTURE_NONE  = 0,
    GESTURE_OPEN  = 1,  // all fingers straight
    GESTURE_FIST  = 2,  // all fingers bent
    GESTURE_POINT = 3,  // only index straight
    GESTURE_PEACE = 4,  // index + middle straight
};

struct __attribute__((packed)) GlovePacket {
    uint8_t type;
    uint8_t finger[3];  // index, middle, ring: 0=straight, 255=fully bent
    uint8_t gesture;
};
