idf_component_register(
    SRCS "main.cpp"
    INCLUDE_DIRS "."
    REQUIRES arduino esp_wifi esp_now nvs_flash
)
