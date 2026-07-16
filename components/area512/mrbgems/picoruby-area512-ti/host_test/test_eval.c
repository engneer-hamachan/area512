#include "area512_ti_hover.h"
#include "area512_ti_suggest.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int
has_suggestion(const TiSuggestionList *suggestions, const char *contents) {
  for (int index = 0; index < suggestions->count; index++) {
    if (strcmp(suggestions->items[index].contents, contents) == 0)
      return 1;
  }

  return 0;
}

static TiSuggestionList
suggest_source(const char *source) {
  TiSuggestionList suggestions;
  int source_length = (int)strlen(source);

  ti_fill_suggestions_at_cursor(
    source,
    source_length,
    source_length,
    &suggestions
  );

  return suggestions;
}

static void
test_literal_bindings(void) {
  TiSuggestionList integer_suggestions = suggest_source("a = 1\na.");
  assert(has_suggestion(&integer_suggestions, "abs"));

  TiSuggestionList string_suggestions = suggest_source("s = \"x\"\ns.sp");
  assert(has_suggestion(&string_suggestions, "split"));

  TiSuggestionList hash_suggestions = suggest_source("h = {}\nh.ke");
  assert(has_suggestion(&hash_suggestions, "key"));
}

static void
test_binding_lookup(void) {
  TiSuggestionList suggestions = suggest_source("a = 1\nb = a\nb.ab");
  assert(has_suggestion(&suggestions, "abs"));
}

static void
test_method_chain(void) {
  TiSuggestionList suggestions =
    suggest_source("s = \"abc\".sub(\"a\", \"b\")\ns.le");
  assert(has_suggestion(&suggestions, "length"));
}

static void
test_instance_variable(void) {
  TiSuggestionList suggestions = suggest_source("@x = 1\n@x.ab");
  assert(has_suggestion(&suggestions, "abs"));
}

static void
test_definition_return(void) {
  TiSuggestionList suggestions =
    suggest_source("a = 1\ndef plus_one(a) = a + 1\nplus_one().ab");
  assert(has_suggestion(&suggestions, "abs"));
}

static void
test_definition_binding_return(void) {
  TiSuggestionList suggestions = suggest_source(
    "def message\n"
    "  value = \"x\"\n"
    "  value\n"
    "end\n"
    "message().sp"
  );
  assert(has_suggestion(&suggestions, "split"));
}

static void
test_if_return(void) {
  TiSuggestionList suggestions = suggest_source(
    "def mixed\n"
    "  if condition\n"
    "    1\n"
    "  else\n"
    "    \"x\"\n"
    "  end\n"
    "end\n"
    "mixed()."
  );
  assert(has_suggestion(&suggestions, "abs"));
  assert(has_suggestion(&suggestions, "bytes"));
}

static void
test_explicit_return(void) {
  TiSuggestionList suggestions = suggest_source(
    "def mixed\n"
    "  return 1\n"
    "  \"x\"\n"
    "end\n"
    "mixed()."
  );
  assert(has_suggestion(&suggestions, "abs"));
  assert(has_suggestion(&suggestions, "bytes"));
}

static void
test_type_at_cursor(void) {
  const char *source = "value = 1\nvalue\n";
  const char *target = strrchr(source, 'v');
  assert(target);

  TiTypeInfo type_info;
  int found = ti_find_type_at_cursor(
    source,
    (int)strlen(source),
    (int)(target - source),
    &type_info
  );

  assert(found);
  assert(strcmp(type_info.variable_name, "value") == 0);
  assert(strcmp(type_info.type_name, "Integer") == 0);
}

static void
test_union_type_at_cursor(void) {
  const char *source = "value = 1\nvalue = \"x\"\nvalue\n";
  const char *target = strrchr(source, 'v');
  assert(target);

  TiTypeInfo type_info;
  assert(ti_find_type_at_cursor(
    source,
    (int)strlen(source),
    (int)(target - source),
    &type_info
  ));
  assert(strcmp(type_info.type_name, "Union<Integer String>") == 0);
}

static void
test_forward_definition(void) {
  TiSuggestionList suggestions =
    suggest_source("x = my_method()\ndef my_method() = \"x\"\nx.sp");
  assert(has_suggestion(&suggestions, "split"));
}

static void
test_union_binding(void) {
  TiSuggestionList suggestions = suggest_source("v = 1\nv = \"s\"\nv.");
  assert(has_suggestion(&suggestions, "abs"));
  assert(has_suggestion(&suggestions, "bytes"));
}

static void
test_unknown_return(void) {
  TiSuggestionList suggestions =
    suggest_source("def orphan(x) = x.foo\norphan().");
  assert(suggestions.count == 0);
}

static void
test_binding_overflow(void) {
  size_t capacity = 24000;
  char *source = malloc(capacity);
  assert(source);

  size_t offset = 0;
  for (int index = 0; index < 700; index++) {
    int written = snprintf(
      source + offset,
      capacity - offset,
      "value_%d = %d\n",
      index,
      index
    );
    assert(written > 0);
    offset += (size_t)written;
    assert(offset < capacity - 16);
  }

  memcpy(source + offset, "value_0.\0", 9);
  offset += 8;

  TiSuggestionList suggestions;
  int count = ti_fill_suggestions_at_cursor(
    source,
    (int)offset,
    (int)offset,
    &suggestions
  );
  assert(count == 0);

  free(source);
}

int
main(void) {
  test_literal_bindings();
  test_binding_lookup();
  test_method_chain();
  test_instance_variable();
  test_definition_return();
  test_definition_binding_return();
  test_if_return();
  test_explicit_return();
  test_type_at_cursor();
  test_union_type_at_cursor();
  test_forward_definition();
  test_union_binding();
  test_unknown_return();
  test_binding_overflow();

  return 0;
}
