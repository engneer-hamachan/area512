ROOT       := $(CURDIR)
PICORB     := $(ROOT)/R2P2-ESP32/components/picoruby-esp32/picoruby
PICORB_ESP := $(ROOT)/components/area512
PICORBC    := $(PICORB)/bin/picorbc
HOME_DIR   := $(ROOT)/storage/home
TI_GENERATED := $(ROOT)/components/area512/mrbgems/picoruby-area512-ti/src/generated

FIRMWARE   := $(ROOT)/firmware

CLANG_FORMAT ?= clang-format

# Our own C/C++ only: skip vendored trees (R2P2-ESP32, m5gfx, M5Unified,
# managed_components), build output, and storage image headers.
FMT_FILES := $(shell find $(ROOT)/main $(ROOT)/components \
	-type f \( -name '*.c' -o -name '*.cpp' -o -name '*.h' -o -name '*.hpp' \) \
	-not -path '*/build/*' \
	-not -path '*/R2P2-ESP32/*' \
	-not -path '*/m5gfx/*' \
	-not -path '*/M5Unified/*' \
	-not -path '*/managed_components/*')

.PHONY: build flash monitor clean fullclean compile-home-mrb flash-firmware save-firmware gendb gendb-test format format-check help

help:
	@echo "Targets:"
	@echo "  make build      - idf.py build"
	@echo "  make flash      - idf.py flash"
	@echo "  make monitor    - idf.py monitor"
	@echo "  make clean      - idf.py clean (light)"
	@echo "  make compile-home-mrb - recursively compile storage/home/**/*.rb to .mrb"
	@echo "                    (storage/ is the seed copied to the SD card's Area512_data/ on first boot)"
	@echo "  make flash-firmware   - flash committed firmware/ binaries (no rebuild)"
	@echo "  make save-firmware    - copy build/ artifacts into firmware/ (refresh snapshot)"
	@echo "  make gendb            - regenerate the built-in TI database"
	@echo "  make gendb-test       - regenerate the built-in TI database and run its host tests"
	@echo "  make fullclean  - nuke everything: build/, picoruby/build/ (esp32-*, host, repos),"
	@echo "                    generated mrb/*.c. Use after editing build_config/*.rb."
	@echo "  make format     - clang-format -i over our own C/C++ (skips vendored trees)"
	@echo "  make format-check - check formatting without writing (CI; non-zero on diff)"

build:
	idf.py build

flash:
	idf.py flash

flash-firmware:
	esptool.py $(if $(ESPPORT),-p $(ESPPORT)) -c esp32s3 -b 460800 write_flash \
	  --flash_mode dio --flash_size 8MB --flash_freq 80m \
	  0x0 $(FIRMWARE)/Area512.bin

save-firmware:
	esptool.py --chip esp32s3 merge_bin \
	  --output $(FIRMWARE)/Area512.bin \
	  --flash_mode dio --flash_size 8MB --flash_freq 80m \
	  0x0 $(ROOT)/build/bootloader/bootloader.bin \
	  0x8000 $(ROOT)/build/partition_table/partition-table.bin \
	  0x10000 $(ROOT)/build/Area512.bin
	@echo "firmware/ refreshed from build/"

monitor:
	idf.py monitor

clean:
	idf.py clean

compile-home-mrb:
	test -x $(PICORBC)
	find $(HOME_DIR) -type f -name '*.rb' -exec $(PICORBC) {} \;

gendb:
	ruby ./tools/tidbgen/main.rb \
	  --manifest ./tools/tidbgen/manifest.rb \
	  --out $(TI_GENERATED)

gendb-test: gendb
	ruby ./tools/tidbgen/test/tidbgen_test.rb
	$(MAKE) -C ./components/area512/mrbgems/picoruby-area512-ti/host_test test

fullclean:
	rm -rf $(ROOT)/build
	rm -rf $(PICORB)/build
	@echo "fullclean: removed build/, $(PICORB)/build"

format:
	$(CLANG_FORMAT) -i $(FMT_FILES)
	@echo "format: clang-format applied to $(words $(FMT_FILES)) files"

format-check:
	$(CLANG_FORMAT) --dry-run --Werror $(FMT_FILES)
