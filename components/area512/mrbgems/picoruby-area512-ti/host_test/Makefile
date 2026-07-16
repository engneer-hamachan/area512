PROJECT_ROOT := ../../../../..
PRISM_ROOT := $(PROJECT_ROOT)/R2P2-ESP32/components/picoruby-esp32/picoruby/mrbgems/mruby-compiler2/lib/prism
SOURCE_ROOT := ../src
SOURCE_DIRS := base builtin context eval eval/method_evaluator hover suggest
ENGINE_SOURCES := \
  $(foreach source_dir,$(SOURCE_DIRS),$(wildcard $(SOURCE_ROOT)/$(source_dir)/*.c))
GENERATED_SOURCE := $(SOURCE_ROOT)/generated/area512_ti_builtin_db.c
INCLUDE_FLAGS := \
  $(foreach source_dir,$(SOURCE_DIRS),-I$(SOURCE_ROOT)/$(source_dir)) \
  -I$(SOURCE_ROOT)/generated -I$(PRISM_ROOT)/include
CFLAGS := -std=c11 -Wall -Wextra -Werror $(INCLUDE_FLAGS)

.PHONY: all clean

all: test_builtin test_eval test_suggest

$(PRISM_ROOT)/build/libprism.a:
	$(MAKE) -C $(PRISM_ROOT) static

test_builtin: test_builtin.c $(SOURCE_ROOT)/builtin/area512_ti_builtin.c $(GENERATED_SOURCE)
	$(CC) $(CFLAGS) $^ -o $@

test_eval: test_eval.c $(ENGINE_SOURCES) $(GENERATED_SOURCE) $(PRISM_ROOT)/build/libprism.a
	$(CC) $(CFLAGS) $^ -o $@

test_suggest: test_suggest.c $(ENGINE_SOURCES) $(GENERATED_SOURCE) $(PRISM_ROOT)/build/libprism.a
	$(CC) $(CFLAGS) $^ -o $@

clean:
	$(RM) test_builtin test_eval test_suggest
