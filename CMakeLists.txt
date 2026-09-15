# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

option(AREA512_EXT_DISPLAY "Build for the 320x240 display with windows" OFF)
option(AREA512_CAPTFT_ILI9341 "Build the Cap TFT firmware for the ILI9341 panel instead of ST7789" OFF)

if(NOT DEFINED SDKCONFIG_DEFAULTS)
  set(SDKCONFIG_DEFAULTS "sdkconfig.defaults")
endif()

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(Area512)
