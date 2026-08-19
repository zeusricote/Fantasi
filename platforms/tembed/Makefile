# Fantasi / LilyGO T-Embed (ESP32-S3) platform build.
#
# Invoked from the top-level Makefile via `make PLATFORM=tembed`.
#
# The ESP32-S3 is an Xtensa LX7 part with no standalone toolchain in this
# tree - it builds through ESP-IDF, which provides the Xtensa toolchain,
# FreeRTOS SMP port, SPI flash drivers, TinyUSB, and esptool.
#
# ESP-IDF can come from:
#   1. $IDF_PATH          (your ESP-IDF install, e.g. set by export.sh)
#   2. `idf.py` on PATH   (resolved back to its IDF root)
#   3. third_party/esp-idf (auto-fetched clone, gitignored - last resort)

TARGET    := fantasi-tembed
ROOT      := $(abspath ../..)
BUILD     := $(ROOT)/build/tembed
PLATFORM_DIR := $(ROOT)/platforms/tembed

# Resolve the ESP-IDF root: local install first, third_party clone as fallback.
IDF_DIR  := $(or $(IDF_PATH), \
                $(shell command -v idf.py >/dev/null 2>&1 && dirname $$(dirname $$(command -v idf.py))), \
                $(ROOT)/third_party/esp-idf)
IDF_PY   := $(IDF_DIR)/tools/idf.py

# Xtensa toolchain bin dir (created by ESP-IDF's install.sh).
IDF_TC   := $(IDF_DIR)/tools/xtensa-esp-elf/xtensa-esp-elf/bin

.PHONY: all clean flash size

all:
	@if [ ! -x $(IDF_PY) ]; then \
	  echo "error: ESP-IDF not found at $(IDF_DIR)." >&2; \
	  echo "  Install ESP-IDF (https://docs.espressif.com/projects/esp-idf) and run export.sh," >&2; \
	  echo "  or let me auto-fetch it: make check-esp-idf" >&2; \
	  exit 1; \
	fi
	@if [ ! -d $(IDF_TC) ]; then \
	  echo "error: Xtensa toolchain not found at $(IDF_TC)." >&2; \
	  echo "  Run ./install.sh esp32s3 inside your ESP-IDF dir, or run: make check-esp-idf" >&2; \
	  exit 1; \
	fi
	cd $(PLATFORM_DIR) && \
	  IDF_PATH=$(IDF_DIR) \
	  PATH="$(IDF_TC):$$PATH" \
	  python3 $(IDF_PY) build
	@mkdir -p $(BUILD)
	@cp $(PLATFORM_DIR)/build/fantasi-tembed.bin $(BUILD)/$(TARGET).bin 2>/dev/null || true
	@cp $(PLATFORM_DIR)/build/fantasi-tembed.elf $(BUILD)/$(TARGET).elf 2>/dev/null || true
	@echo "Built: $(BUILD)/$(TARGET).bin"

clean:
	@if [ -d $(PLATFORM_DIR)/build ]; then \
	  cd $(PLATFORM_DIR) && \
	    IDF_PATH=$(IDF_DIR) \
	    PATH="$(IDF_TC):$$PATH" \
	    python3 $(IDF_PY) fullclean; \
	fi
	rm -rf $(BUILD)

flash: all
	cd $(PLATFORM_DIR) && \
	  IDF_PATH=$(IDF_DIR) \
	  PATH="$(IDF_TC):$$PATH" \
	  python3 $(IDF_PY) -p $${ESPTOOL_PORT:-/dev/ttyACM0} flash

size: all
	@if [ -f $(PLATFORM_DIR)/build/fantasi-tembed.elf ]; then \
	  $(IDF_TC)/xtensa-esp32s3-elf-size $(PLATFORM_DIR)/build/fantasi-tembed.elf; \
	fi
