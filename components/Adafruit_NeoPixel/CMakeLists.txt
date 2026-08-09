idf_component_register(
    SRCS
        "Adafruit_NeoPixel.cpp"
        "esp.c"
        "esp8266.c"
        "Adafruit_Neopixel_RP2.cpp"
        "kendyte_k210.c"
        "psoc6.c"
    INCLUDE_DIRS "."
    REQUIRES "arduino"
)
