set(srcs
        "main.c"
        "sketch.cpp"
        "gamepad_input.cpp"
        "led_controller.cpp"
        "light_effects.cpp"
        "web_ui.cpp"
        "servo_controller.cpp"
        "eye_motion.cpp"
        "audio_player.cpp"
        "ota_service.cpp")

set(requires "bluepad32" "bluepad32_arduino" "arduino" "btstack" "Adafruit_NeoPixel" "Adafruit_BusIO" "Adafruit_PWMServoDriver")

idf_component_register(SRCS "${srcs}"
                    INCLUDE_DIRS "."
                    REQUIRES "${requires}")
