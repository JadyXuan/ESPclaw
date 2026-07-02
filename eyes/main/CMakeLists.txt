idf_component_register(
    SRCS "main.cpp" "eye_renderer.cpp"
    INCLUDE_DIRS "."
    REQUIRES arduino-esp32 LovyanGFX esp_wifi
)
