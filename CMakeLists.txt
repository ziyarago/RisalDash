# ESP-IDF component registration. RisalDash is an Arduino-framework library, so build it on
# ESP-IDF with arduino-esp32 present as a component (Arduino as an ESP-IDF component).
# The Arduino IDE and PlatformIO ignore this file and build via library.properties / library.json.
idf_component_register(
    SRCS "src/RisalUI.cpp"
         "src/RisalRender.cpp"
         "src/RisalProvision.cpp"
         "src/RisalApi.cpp"
         "src/RisalSensors.cpp"
         "src/RisalMqtt.cpp"
         "src/RisalAssets.cpp"
    INCLUDE_DIRS "src"
    REQUIRES arduino
)
