# Fantasi - unified Makefile for all supported platforms.
#
# Usage:
#   make                        # build all firmwares + host CLI
#   make PLATFORM=flipper       # STM32WB55 (Cortex-M4)
#   make PLATFORM=kiisu         # STM32WB55 (Cortex-M4)
#   make PLATFORM=chameleon     # nRF52840  (Cortex-M4)
#   make PLATFORM=proxmark3     # AT91SAM7S (ARM7TDMI)
#   make PLATFORM=proxmark5     # AT32F435  (Cortex-M4)
#   make PLATFORM=tembed        # LilyGO T-Embed (ESP32-S3, Xtensa LX7)
#   make cli                    # host-side CLI (build/cli/fantasi)
#   make clean                  # remove all build artifacts (or PLATFORM=<x> for one)
#   make flash                  # auto-detect connected device + flash (PLATFORM=<x> to force)
#
# With no PLATFORM: `make` builds every firmware plus the host CLI, `make clean`
# wipes build/, and `make flash` auto-detects the connected board (erroring on
# ambiguity). `make help` lists the targets.

PLATFORM ?= help

# Every buildable firmware target - used when no PLATFORM is given so a bare
# `make` builds them all and `make clean` removes everything.
ALL_PLATFORMS := flipper kiisu chameleon proxmark3 proxmark5 tembed

# All three platforms build against TinyUSB under third_party/tinyusb/. The
# `check-tinyusb` target auto-clones it if missing, pinned to a known-
# good release tag so `make` stays reproducible. Bump TUSB_TAG when you
# want a newer version - don't track a moving branch, it breaks builds.
TUSB_URL    := https://github.com/hathach/tinyusb.git
TUSB_TAG    := 0.17.0
TUSB_DIR    := third_party/tinyusb
TUSB_MARKER := $(TUSB_DIR)/src/tusb.c

# Fantasi modifications to the (gitignored, fetched) TinyUSB tree - see
# third_party/tinyusb_patches/README.md. check-tinyusb re-applies them after the
# clone, idempotently and non-destructively:
#   *.patch          edits to upstream files, applied via `git apply` with a
#                    reverse-check guard (already-applied → skip; conflicts → warn,
#                    never clobber).
#   dcd_at91sam7s.c  a new upstream file (SAM7S DCD). Copied in only when absent,
#                    so a locally-edited clone driver is never overwritten (the
#                    stash is refreshed from the clone by hand - see the README).
TUSB_PATCHES  := $(wildcard third_party/tinyusb_patches/*.patch)
SAM7S_DCD_SRC := third_party/tinyusb_patches/dcd_at91sam7s.c
SAM7S_DCD_DST := $(TUSB_DIR)/src/portable/microchip/at91sam7s/dcd_at91sam7s.c
# AT32F435 DWC2 port header (Proxmark5). A new upstream file, copied in only
# when absent - same discipline as the SAM7S DCD above. The two register
# edits it needs (OPT_MCU_AT32F435, the dcd_dwc2.c include) ride in via the
# *.patch glob.
AT32_DWC2_SRC := third_party/tinyusb_patches/dwc2_at32.h
AT32_DWC2_DST := $(TUSB_DIR)/src/portable/synopsys/dwc2/dwc2_at32.h

LFS_URL     := https://github.com/littlefs-project/littlefs.git
LFS_TAG     := v2.11.3
LFS_DIR     := third_party/littlefs
LFS_MARKER  := $(LFS_DIR)/lfs.c

# nanopb code generator (host pip install, e.g. ~/.local/bin). Override if it
# lives elsewhere: `make proto NANOPB=/path/to/nanopb_generator`.
NANOPB ?= nanopb_generator

# Cross-toolchain prefix. Matches the platform Makefiles' default; override to
# point at a specific install, e.g. `make flash CROSS=/usr/bin/arm-none-eabi-`.
# check-toolchain validates this toolchain, so the check tracks what actually
# builds/flashes.
CROSS ?= arm-none-eabi-

.PHONY: all cli app launch flash storage test test-unit proto berry clean help $(PLATFORM) check-tinyusb check-littlefs check-toolchain

all:
	@if [ "$(PLATFORM)" = "help" ]; then \
	  for p in $(ALL_PLATFORMS); do \
	    echo "=== Building $$p ==="; \
	    $(MAKE) $$p || exit 1; \
	  done; \
	  echo "=== Building cli ==="; \
	  $(MAKE) cli || exit 1; \
	else \
	  $(MAKE) $(PLATFORM); \
	fi

check-toolchain:
	@if ! command -v $(CROSS)gcc >/dev/null 2>&1; then \
	  echo "error: $(CROSS)gcc not on PATH." >&2; \
	  echo "  Debian/Ubuntu:  sudo apt install gcc-arm-none-eabi picolibc-arm-none-eabi" >&2; \
	  echo "  Or set CROSS=/path/to/arm-none-eabi- to point at a specific install." >&2; \
	  exit 1; \
	fi
	@if ! $(CROSS)gcc --specs=picolibc.specs -E - </dev/null >/dev/null 2>&1; then \
	  echo "error: $(CROSS)gcc has no picolibc.specs (the firmware links picolibc)." >&2; \
	  echo "  Debian/Ubuntu:  sudo apt install picolibc-arm-none-eabi" >&2; \
	  echo "  Note: the upstream Arm GNU Toolchain tarball does not bundle picolibc - use the distro" >&2; \
	  echo "  packages, or point CROSS at a picolibc-capable toolchain." >&2; \
	  exit 1; \
	fi

help:
	@echo "Fantasi - PLATFORM is one of: flipper, kiisu, chameleon, proxmark3, proxmark5"
	@echo ""
	@echo "  make                      build all firmwares + host CLI"
	@echo "  make PLATFORM=flipper     build Flipper firmware (STM32WB55)"
	@echo "  make PLATFORM=kiisu       build Kiisu firmware (STM32WB55, Flipper-compatible)"
	@echo "  make PLATFORM=chameleon   build Chameleon Ultra firmware (nRF52840)"
	@echo "  make PLATFORM=proxmark3   build Proxmark3 firmware (AT91SAM7S)"
	@echo "  make PLATFORM=proxmark5   build Proxmark5 firmware (AT32F435)"
	@echo "  make PLATFORM=tembed      build LilyGO T-Embed firmware (ESP32-S3)"
	@echo "  make cli                  build the host CLI (build/cli/fantasi)"
	@echo "  make app APP=<name>       build a loadable app (apps/<name>/<name>.c)"
	@echo "  make launch APP=<name>    build + upload + run an app on the device"
	@echo "  make flash                auto-detect the connected device + flash it"
	@echo "  make PLATFORM=<x> flash   build + upload resources + flash the device"
	@echo "  make clean                remove all build artifacts (build/)"
	@echo "  make PLATFORM=<x> clean   remove build/<x>/"
	@echo ""
	@echo "  Flipper and Chameleon need TinyUSB in third_party/tinyusb/."
	@echo "  If missing:  git clone https://github.com/hathach/tinyusb third_party/tinyusb"

check-tinyusb:
	@if [ ! -f $(TUSB_MARKER) ]; then \
	  if ! command -v git >/dev/null 2>&1; then \
	    echo "error: git is required to fetch TinyUSB. Install git and retry." >&2; \
	    exit 1; \
	  fi; \
	  echo "Fetching TinyUSB $(TUSB_TAG) into $(TUSB_DIR)..."; \
	  rm -rf $(TUSB_DIR); \
	  git clone --depth 1 --branch $(TUSB_TAG) $(TUSB_URL) $(TUSB_DIR) || { \
	    echo "error: TinyUSB clone failed. Check network and try again." >&2; \
	    rm -rf $(TUSB_DIR); exit 1; \
	  }; \
	fi
	@# Re-apply fantasi TinyUSB modifications (idempotently).
	@if ! command -v git >/dev/null 2>&1; then \
	  echo "warning: git not found - cannot apply TinyUSB patches in $(TUSB_DIR)." >&2; \
	else \
	  for p in $(TUSB_PATCHES); do \
	    if git -C $(TUSB_DIR) apply --reverse --check "$(CURDIR)/$$p" >/dev/null 2>&1; then \
	      : ; \
	    elif git -C $(TUSB_DIR) apply --check "$(CURDIR)/$$p" >/dev/null 2>&1; then \
	      echo "Applying TinyUSB patch: $$p"; \
	      git -C $(TUSB_DIR) apply "$(CURDIR)/$$p"; \
	    else \
	      echo "warning: $$p does not apply cleanly to TinyUSB $(TUSB_TAG) (edited clone or version bump?) - skipping" >&2; \
	    fi; \
	  done; \
	fi
	@# Install the SAM7S DCD (a new upstream file) ONLY when absent - never
	@# overwrite an existing clone driver.
	@if [ -f $(SAM7S_DCD_SRC) ] && [ ! -f $(SAM7S_DCD_DST) ]; then \
	  echo "Installing SAM7S DCD -> $(SAM7S_DCD_DST)"; \
	  mkdir -p $(dir $(SAM7S_DCD_DST)); \
	  cp $(SAM7S_DCD_SRC) $(SAM7S_DCD_DST); \
	fi
	@# Install the AT32F435 DWC2 port header (a new upstream file) ONLY when absent.
	@if [ -f $(AT32_DWC2_SRC) ] && [ ! -f $(AT32_DWC2_DST) ]; then \
	  echo "Installing AT32 DWC2 header -> $(AT32_DWC2_DST)"; \
	  mkdir -p $(dir $(AT32_DWC2_DST)); \
	  cp $(AT32_DWC2_SRC) $(AT32_DWC2_DST); \
	fi

check-littlefs:
	@if [ ! -f $(LFS_MARKER) ]; then \
	  if ! command -v git >/dev/null 2>&1; then \
	    echo "error: git is required to fetch LittleFS. Install git and retry." >&2; \
	    exit 1; \
	  fi; \
	  echo "Fetching LittleFS $(LFS_TAG) into $(LFS_DIR)..."; \
	  rm -rf $(LFS_DIR); \
	  git clone --depth 1 --branch $(LFS_TAG) $(LFS_URL) $(LFS_DIR) || { \
	    echo "error: LittleFS clone failed. Check network and try again." >&2; \
	    rm -rf $(LFS_DIR); exit 1; \
	  }; \
	fi

flipper: check-toolchain check-tinyusb check-littlefs
	$(MAKE) -C platforms/$@

# Kiisu is pin-identical to the Flipper; its Makefile reuses the Flipper build.
kiisu: check-toolchain check-tinyusb check-littlefs
	$(MAKE) -C platforms/$@

chameleon: check-toolchain check-tinyusb check-littlefs
	$(MAKE) -C platforms/$@

proxmark3: check-toolchain check-tinyusb check-littlefs
	$(MAKE) -C platforms/$@

proxmark5: check-toolchain check-tinyusb check-littlefs
	$(MAKE) -C platforms/$@

# LilyGO T-Embed (ESP32-S3). Built through ESP-IDF (Xtensa LX7 toolchain,
# FreeRTOS SMP port, TinyUSB, esptool) - see platforms/tembed/Makefile.
tembed:
	$(MAKE) -C platforms/$@

flash:
	@if [ "$(PLATFORM)" = "help" ]; then \
	  plat=$$(python3 tools/flash.py --detect) || exit 1; \
	  echo "Auto-detected platform: $$plat"; \
	  $(MAKE) PLATFORM=$$plat flash; \
	else \
	  $(MAKE) $(PLATFORM) && python3 tools/flash.py --platform $(PLATFORM); \
	fi

# Host-side CLI (build/cli/fantasi). Built with the host compiler (cc), not the
# ARM cross-toolchain. Needs libreadline; BLE support compiles in when libsystemd
# is present. Has its own Makefile under cli/.
cli:
	$(MAKE) -C cli

# Build a single loadable user app: make app APP=<name>  (apps/<name>/<name>.c).
# Produces per-arch ELFs - <name>.cm4.elf (Cortex-M: Flipper, Chameleon) and
# <name>.arm7.elf (ARM7TDMI: Proxmark3); copy the matching one to /ramfs or /apps.
app:
	$(MAKE) -C apps app APP=$(APP)

# Build + upload + run an app on the connected device: make launch APP=<name>
# Auto-detects the board (force with PLATFORM=<x>), uploads to /ramfs, then opens
# an interactive launch session - the app's output streams and Ctrl-C stops it.
# Reach the device over BLE (Flipper/Chameleon) with BLE=1 (auto-discover) or
# BLE=<AA:BB:CC:DD:EE:FF> to pick a specific device when several are bonded.
launch:
	@test -n "$(APP)" || { echo "usage: make launch APP=<name> [PLATFORM=<x>] [BLE=1|BLE=<addr>]"; exit 1; }
	python3 tools/launch.py $(APP) \
	  $(if $(filter-out help,$(PLATFORM)),--platform $(PLATFORM)) \
	  $(if $(BLE),$(if $(findstring :,$(BLE)),--ble-addr=$(BLE),--ble))

storage:
	@if [ "$(PLATFORM)" = "help" ]; then \
	  echo "Set PLATFORM=<flipper|chameleon|proxmark3> for storage target"; exit 1; \
	fi
	$(MAKE) -C platforms/$(PLATFORM) storage

# Integration tests - need a connected device (run per-platform).
test:
	@if [ "$(PLATFORM)" = "help" ]; then \
	  echo "Set PLATFORM=<flipper|kiisu|chameleon|proxmark3|proxmark5> for test target"; exit 1; \
	fi
	python3 tests/run.py --platform $(PLATFORM)

# Unit tests - hardware-free (tests/unit/); what CI runs on every push.
test-unit:
	python3 tests/run_unit.py

# Regenerate the nanopb sources from proto/fantasi.proto. The committed
# proto/fantasi.pb.{c,h} are what the builds compile; run this after editing
# the .proto. `-I proto` makes the canonical name `fantasi` so the generated
# `#include "fantasi.pb.h"` resolves under the builds' `-I proto`; `-D proto`
# writes the output into proto/ (else it lands in the cwd); `-I third_party/nanopb`
# resolves the imported nanopb.proto. tests/unit/test_proto_sync fails if the
# committed sources drift from the .proto.
proto:
	$(NANOPB) proto/fantasi.proto -I proto -I third_party/nanopb -D proto

# Regenerate Berry's precompiled const tables (third_party/berry/generate/) with
# its `coc` codegen, from the source + fantasi's berry_conf.h. tests/unit/test_berry_sync
# guards drift.
BERRY_DIR := third_party/berry
berry:
	python3 $(BERRY_DIR)/tools/coc/coc -o $(BERRY_DIR)/generate $(BERRY_DIR)/src -c $(BERRY_DIR)/berry_conf.h

clean:
	@if [ "$(PLATFORM)" = "help" ]; then \
	  rm -rf build; \
	else \
	  $(MAKE) -C platforms/$(PLATFORM) clean; \
	fi
