cmake_minimum_required(VERSION 3.22)

# Секрети (Wi-Fi, ключ каналу, OTA-токен) живуть у sdkconfig.secrets — він у .gitignore.
# Шаблон: sdkconfig.secrets.example
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/sdkconfig.secrets")
    set(SDKCONFIG_DEFAULTS "sdkconfig.defaults;sdkconfig.secrets")
endif()

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(svitlobot)
