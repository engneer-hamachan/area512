PROJECT_ROOT := ../../../../..
PRISM_ROOT := $(PROJECT_ROOT)/R2P2-ESP32/components/picoruby-esp32/picoruby/mrbgems/mruby-compiler2/lib/prism
SOURCE_ROOT := ../src
SOURCE_DIRS := base builtin context eval eval/method_evaluator hover suggest
ENGINE_SOURCES := \
  $(foreach source_dir,$(SOURCE_DIRS),$(wildcard $(SOURCE_ROOT)/$(source_dir)/*.c))
GENERATED_SOURCE := $(SOURCE_ROOT)/generated/area512_ti_builtin_database.c
OBJECT_ROOT := build
ENGINE_OBJECTS := \
  $(patsubst $(SOURCE_ROOT)/%.c,$(OBJECT_ROOT)/%.o,$(ENGINE_SOURCES))
GENERATED_OBJECT := \
  $(patsubst $(SOURCE_ROOT)/%.c,$(OBJECT_ROOT)/%.o,$(GENERATED_SOURCE))
TEST_OBJECTS := \
  $(OBJECT_ROOT)/test_builtin.o \
  $(OBJECT_ROOT)/test_eval.o \
  $(OBJECT_ROOT)/test_suggest.o
DEPENDENCIES := \
  $(ENGINE_OBJECTS:.o=.d) \
  $(GENERATED_OBJECT:.o=.d) \
  $(TEST_OBJECTS:.o=.d)
INCLUDE_FLAGS := \
  $(foreach source_dir,$(SOURCE_DIRS),-I$(SOURCE_ROOT)/$(source_dir)) \
  -I$(SOURCE_ROOT)/generated -I$(PRISM_ROOT)/include
CFLAGS := -std=c11 -Wall -Wextra -Werror -MMD -MP $(INCLUDE_FLAGS)

.PHONY: all test clean

all: test_builtin test_eval test_suggest

test:
	$(MAKE) all
	./test_builtin
	./test_eval
	./test_suggest

$(PRISM_ROOT)/build/libprism.a:
	$(MAKE) -C $(PRISM_ROOT) static

$(OBJECT_ROOT)/%.o: $(SOURCE_ROOT)/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJECT_ROOT)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

test_builtin: \
  $(OBJECT_ROOT)/test_builtin.o \
  $(OBJECT_ROOT)/builtin/area512_ti_builtin.o \
  $(GENERATED_OBJECT)
	$(CC) $^ -o $@

test_eval: \
  $(OBJECT_ROOT)/test_eval.o \
  $(ENGINE_OBJECTS) \
  $(GENERATED_OBJECT) \
  $(PRISM_ROOT)/build/libprism.a
	$(CC) $^ -o $@

test_suggest: \
  $(OBJECT_ROOT)/test_suggest.o \
  $(ENGINE_OBJECTS) \
  $(GENERATED_OBJECT) \
  $(PRISM_ROOT)/build/libprism.a
	$(CC) $^ -o $@

clean:
	$(RM) -r $(OBJECT_ROOT) test_builtin test_eval test_suggest

-include $(DEPENDENCIES)
