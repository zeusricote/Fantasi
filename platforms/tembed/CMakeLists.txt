# Fantasi / LilyGO T-Embed (ESP32-S3) platform build.
#
# This project uses the ESP-IDF build system because the ESP32-S3 has no
# standalone toolchain here - ESP-IDF provides the Xtensa LX7 toolchain,
# FreeRTOS SMP port, SPI flash drivers, TinyUSB, and esptool.
#
# The platform top-level Makefile wraps `idf.py`, so
# `make PLATFORM=tembed` (from the repo root) works like every other target.

cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(fantasi-tembed)