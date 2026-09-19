ROOT       := $(CURDIR)
PICORB     := $(ROOT)/R2P2-ESP32/components/picoruby-esp32/picoruby
LIBMRUBY     := $(PICORB)/build/esp32-femtoruby/lib/libmruby.a
LIBMRUBY_TFT := $(PICORB)/build/esp32-femtoruby-captft/lib/libmruby.a
PICORB_ESP := $(ROOT)/components/area512
PICORBC    := $(PICORB)/bin/picorbc
MPY_CROSS  := $(ROOT)/components/micropython/mpy-cross/build/mpy-cross
HOME_DIR   := $(ROOT)/storage/home
TI_GENERATED := $(ROOT)/components/area512/mrbgems/picoruby-ti/src/generated
MICROPYTHON_TI := $(ROOT)/components/area512/mrbgems/micropython-ti

FIRMWARE   := $(ROOT)/firmware

CLANG_FORMAT ?= clang-format
INPUT ?= background.png
OUTPUT ?= $(ROOT)/storage/share/backgrounds/$(basename $(notdir $(INPUT))).rgb565
SIZE ?= 320x240

# Our own C/C++ only: skip vendored trees (R2P2-ESP32, m5gfx, M5Unified,
# managed_components), build output, and storage image headers.
FMT_FILES := $(shell find $(ROOT)/main $(ROOT)/components \
	-type f \( -name '*.c' -o -name '*.cpp' -o -name '*.h' -o -name '*.hpp' \) \
	-not -path '*/build/*' \
	-not -path '*/R2P2-ESP32/*' \
	-not -path '*/m5gfx/*' \
	-not -path '*/M5Unified/*' \
	-not -path '*/managed_components/*')

.PHONY: build flash monitor clean light-clean light-clean-tft fullclean compile-home-mrb compile-home-mpy flash-firmware save-firmware gendb format format-check run-emulator convert help

help:
	@echo "Targets:"
	@echo "  make build      - idf.py build"
	@echo "  make flash      - idf.py flash"
	@echo "  make monitor    - idf.py monitor"
	@echo "  make clean      - idf.py clean (light)"
	@echo "  make light-clean     - remove esp32-femtoruby libmruby.a (Cardputer)"
	@echo "  make light-clean-tft - remove esp32-femtoruby-captft libmruby.a (Cap TFT)"
	@echo "  make compile-home-mrb - recursively compile storage/home/**/*.rb to .mrb"
	@echo "  make compile-home-mpy - recursively compile storage/home/**/*.py to .mpy"
	@echo "                    (storage/ is the seed copied to the SD card's Area512_data/ on first boot)"
	@echo "  make flash-firmware   - flash committed firmware/ binaries (no rebuild)"
	@echo "  make save-firmware    - copy build/, build/v11/, build/captft7789/, build/captft9341/ artifacts into firmware/"
	@echo "  make gendb            - regenerate the built-in TI database"
	@echo "  make fullclean  - nuke everything: build/, picoruby/build/ (esp32-*, host, repos),"
	@echo "                    generated mrb/*.c. Use after editing build_config/*.rb."
	@echo "  make format     - clang-format -i over our own C/C++ (skips vendored trees)"
	@echo "  make format-check - check formatting without writing (CI; non-zero on diff)"
	@echo "  make run-emulator - run firmware/Area512.bin in the Cardputer ADV emulator"
	@echo "  make convert INPUT=background.png - convert to storage/share/backgrounds/background.rgb565 (320x240)"
	@echo "                    OUTPUT=path overrides the output file; requires Python 3 and Pillow"
	@echo "                    SIZE=240x135 converts for the Cardputer display (default 320x240)"

convert:
	python3 "$(ROOT)/tool/convert.py" "$(INPUT)" "$(OUTPUT)" --size "$(SIZE)"

build:
	idf.py build

flash:
	idf.py flash

flash-firmware:
	esptool.py $(if $(ESPPORT),-p $(ESPPORT)) -c esp32s3 -b 460800 write_flash \
	  --flash_mode dio --flash_size 8MB --flash_freq 80m \
	  0x0 $(FIRMWARE)/Area512.bin

define merge_firmware
	esptool.py --chip esp32s3 merge_bin \
	  --output $(FIRMWARE)/$(2) \
	  --flash_mode dio --flash_size 8MB --flash_freq 80m \
	  0x0 $(1)/bootloader/bootloader.bin \
	  0x8000 $(1)/partition_table/partition-table.bin \
	  0x10000 $(1)/Area512.bin
endef

save-firmware:
	$(call merge_firmware,$(ROOT)/build,Area512Adv.bin)
	$(call merge_firmware,$(ROOT)/build/v11,Area512V11.bin)
	$(call merge_firmware,$(ROOT)/build/captft7789,Area512CapTFT7789.bin)
	$(call merge_firmware,$(ROOT)/build/captft9341,Area512CapTFT9341.bin)
	@echo "firmware/ refreshed from build/"

monitor:
	idf.py monitor

run-emulator:
	$(ROOT)/emulator/run.sh

clean:
	idf.py clean

light-clean:
	rm -f $(LIBMRUBY)
	@echo "light-clean: removed $(LIBMRUBY)"

light-clean-tft:
	rm -f $(LIBMRUBY_TFT)
	@echo "light-clean-tft: removed $(LIBMRUBY_TFT)"

compile-home-mrb:
	test -x $(PICORBC)
	find $(HOME_DIR) -type f -name '*.rb' -exec $(PICORBC) {} \;

compile-home-mpy:
	$(MAKE) -C $(ROOT)/components/micropython/mpy-cross
	test -x $(MPY_CROSS)
	find $(HOME_DIR) -type f -name '*.py' -exec $(MPY_CROSS) {} \;

gendb:
	ruby ./components/area512/mrbgems/picoruby-ti/tidbgen/main.rb \
	  --sig-dir ./components/area512/sig/picoruby \
	  --out $(TI_GENERATED)
	$(MAKE) -C $(MICROPYTHON_TI) gendb

fullclean:
	rm -rf $(ROOT)/build
	rm -rf $(PICORB)/build
	@echo "fullclean: removed build/, $(PICORB)/build"

format:
	$(CLANG_FORMAT) -i $(FMT_FILES)
	@echo "format: clang-format applied to $(words $(FMT_FILES)) files"

format-check:
	$(CLANG_FORMAT) --dry-run --Werror $(FMT_FILES)
