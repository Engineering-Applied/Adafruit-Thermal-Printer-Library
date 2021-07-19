cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(Printer_Test_project)

pico_sdk_init()

#First Parameter is Library name, then associated files
add_library(
	Thermal_Print
	Thermal_Print.h
	Thermal_Print.cpp
	)

target_link_libraries(Thermal_Print pico_stdlib)

#First Parameter is project name, then associated files
add_executable(Printer_Test 
	Printer_Test.cpp
)

pico_enable_stdio_uart(Printer_Test 1)

# create map/bin/hex file etc.
pico_add_extra_outputs(Printer_Test)

# Pull in our pico_stdlib which pulls in commonly used features
target_link_libraries(Printer_Test Thermal_Print pico_stdlib)
