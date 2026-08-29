MICROPYTHON_TOP := ../micropython
PACKAGE_DIR := micropython_embed

# Both must be set before embed.mk: it ends by including py/mkrules.mk, where
# the qstr rule's prerequisites and QSTR_GEN_CFLAGS are expanded immediately.
SRC_QSTR += $(wildcard modules/*.c)

CFLAGS += -I../area512_hal/include
CFLAGS += -I../../R2P2-ESP32/components/picoruby-esp32/picoruby/mrbgems/picoruby-gpio/include
CFLAGS += -I../../R2P2-ESP32/components/picoruby-esp32/picoruby/mrbgems/picoruby-io-console/include
CFLAGS += -I../area512/mrbgems/picoruby-area512-widget/src
CFLAGS += -I../area512/mrbgems/picoruby-area512-dot/src

include $(MICROPYTHON_TOP)/ports/embed/embed.mk
