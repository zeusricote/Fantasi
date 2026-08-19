# Fantasi / LilyGO T-Embed (ESP32-S3) platform build.
#
# Invoked from the top-level Makefile via `make PLATFORM=tembed`.
#
# The ESP32-S3 is an Xtensa LX7 part with no standalone toolchain in this
# tree - it builds through ESP-IDF, which provides the Xtensa toolchain,
# FreeRTOS SMP port, SPI flash drivers, TinyUSB, and esptool.
#
# Requirements:
#   - ESP-IDF installed and IDF_PATH exported (or `idf.py` on PATH)
#   - `idf.py` on PATH (ESP-IDF's Python wrapper)
#
# Usage:
#   make PLATFORM=tembed            # build
#   make PLATFORM=tembed clean      # clean
#   make PLATFORM=tembed flash      # build + flash via esptool

TARGET    := fantasi-tembed
ROOT      := $(abspath ../..)
BUILD     := $(ROOT)/build/tembed
PLATFORM_DIR := $(ROOT)/platforms/tembed

# ESP-IDF location. Prefer IDF_PATH; fall back to `idf.py` on PATH.
IDF_PATH ?= $(shell dirname $$(dirname $$(command -v idf.py 2>/dev/null) 2>/dev/null) 2>/dev/null)

# idf.py wrapper - set IDF_TARGET so the build is always esp32s3.
IDF_PY := idf.py

.PHONY: all clean flash size

all:
	@if [ -z "$$IDF_PATH" ] && [ -z "$(IDF_PATH)" ]; then \
	  echo "error: ESP-IDF not found. Install it and export IDF_PATH, or put idf.py on PATH." >&2; \
	  echo "  See https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/get-started/" >&2; \
	  exit 1; \
	fi
	@if ! command -v $(IDF_PY) >/dev/null 2>&1; then \
	  echo "error: idf.py not on PATH. Source ESP-IDF's export.sh first." >&2; \
	  exit 1; \
	fi
	cd $(PLATFORM_DIR) && \
	  IDF_TARGET=esp32s3 \
	  $(IDF_PY) build
	@mkdir -p $(BUILD)
	@cp $(PLATFORM_DIR)/build/fantasi-tembed.bin $(BUILD)/$(TARGET).bin 2>/dev/null || true
	@cp $(PLATFORM_DIR)/build/fantasi-tembed.elf $(BUILD)/$(TARGET).elf 2>/dev/null || true
	@echo "Built: $(BUILD)/$(TARGET).bin"

clean:
	@if [ -d $(PLATFORM_DIR)/build ]; then \
	  cd $(PLATFORM_DIR) && $(IDF_PY) fullclean; \
	fi
	rm -rf $(BUILD)

flash: all
	cd $(PLATFORM_DIR) && \
	  IDF_TARGET=esp32s3 \
	  $(IDF_PY) -p $${ESPTOOL_PORT:-/dev/ttyACM0} flash

size: all
	@if [ -f $(PLATFORM_DIR)/build/fantasi-tembed.elf ]; then \
	  xtensa-esp32s3-elf-size $(PLATFORM_DIR)/build/fantasi-tembed.elf; \
	fi