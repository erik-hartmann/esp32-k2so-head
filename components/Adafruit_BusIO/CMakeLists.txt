# Adafruit Bus IO Library
# https://github.com/adafruit/Adafruit_BusIO
# MIT License
#
# Upstream's own CMakeLists.txt requires a component named "arduino-esp32";
# in this project the arduino-esp32 core lives at components/arduino, so its
# ESP-IDF component name is "arduino" (derived from the directory). Adjusted
# below to match.

idf_component_register(
    SRCS "Adafruit_I2CDevice.cpp" "Adafruit_BusIO_Register.cpp" "Adafruit_SPIDevice.cpp" "Adafruit_GenericDevice.cpp"
    INCLUDE_DIRS "."
    REQUIRES arduino
)
