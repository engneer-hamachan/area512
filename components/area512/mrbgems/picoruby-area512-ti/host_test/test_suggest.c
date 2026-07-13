#include "area512_ti_eval.h"
#include <assert.h>
#include <string.h>

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

static const TiSuggestion *
find_suggestion(const TiSuggestionList *suggestions, const char *contents) {
  for (int index = 0; index < suggestions->count; index++) {
    if (strcmp(suggestions->items[index].contents, contents) == 0)
      return &suggestions->items[index];
  }

  return NULL;
}

static void
test_string_suggestion(void) {
  TiSuggestionList suggestions = suggest_source("s = \"abc\"\ns.");
  assert(suggestions.count > 0);

  for (int index = 1; index < suggestions.count; index++) {
    assert(
      strcmp(
        suggestions.items[index - 1].contents,
        suggestions.items[index].contents
      ) <= 0
    );
  }
}

static void
test_prefix_suggestion(void) {
  TiSuggestionList suggestions = suggest_source("s = \"abc\"\ns.su");
  assert(suggestions.count == 1);
  assert(strcmp(suggestions.items[0].contents, "sub") == 0);
}

static void
test_unknown_receiver(void) {
  TiSuggestionList suggestions = suggest_source("x.");
  assert(suggestions.count == 0);
}

static void
test_union_suggestion(void) {
  TiSuggestionList suggestions = suggest_source("v = 1\nv = \"s\"\nv.");
  const TiSuggestion *integer_suggestion = find_suggestion(&suggestions, "abs");
  const TiSuggestion *string_suggestion =
    find_suggestion(&suggestions, "bytes");

  assert(integer_suggestion);
  assert(string_suggestion);
  assert(strcmp(integer_suggestion->class_name, "Integer") == 0);
  assert(strcmp(string_suggestion->class_name, "String") == 0);

  for (int first = 0; first < suggestions.count; first++) {
    for (int second = first + 1; second < suggestions.count; second++) {
      int same_name = strcmp(
                        suggestions.items[first].contents,
                        suggestions.items[second].contents
                      ) == 0;
      int same_label = strcmp(
                         suggestions.items[first].detail,
                         suggestions.items[second].detail
                       ) == 0;

      assert(!(same_name && same_label));
    }
  }
}

static void
test_union_prefix_suggestion(void) {
  TiSuggestionList suggestions = suggest_source("v = 1\nv = \"s\"\nv.s");
  const TiSuggestion *integer_suggestion =
    find_suggestion(&suggestions, "sleep");
  const TiSuggestion *string_suggestion = find_suggestion(&suggestions, "sub");

  assert(integer_suggestion);
  assert(string_suggestion);
  assert(strcmp(integer_suggestion->class_name, "Integer") == 0);
  assert(strcmp(string_suggestion->class_name, "String") == 0);

  for (int index = 0; index < suggestions.count; index++)
    assert(suggestions.items[index].contents[0] == 's');
}

static void
test_union_skips_user_class(void) {
  TiSuggestionList suggestions = suggest_source("class Foo\n"
                                                "  def bar = 1\n"
                                                "end\n"
                                                "value = Foo.new\n"
                                                "value = 1\n"
                                                "value.");

  assert(find_suggestion(&suggestions, "abs"));
  assert(!find_suggestion(&suggestions, "bar"));
}

static void
test_static_method_suggestion(void) {
  TiSuggestionList suggestions = suggest_source("GPIO.ne");
  assert(find_suggestion(&suggestions, "new"));
}

static void
test_user_class_suggestion(void) {
  TiSuggestionList suggestions = suggest_source("class Foo\n"
                                                "  def bar(value) = value\n"
                                                "end\n"
                                                "foo = Foo.new\n"
                                                "foo.ba");
  const TiSuggestion *suggestion = find_suggestion(&suggestions, "bar");

  assert(suggestion);
  assert(strcmp(suggestion->detail, "bar(value)") == 0);
}

static void
test_user_class_only_suggests_its_methods(void) {
  TiSuggestionList suggestions = suggest_source("class Foo\n"
                                                "  def foo_method = 1\n"
                                                "end\n"
                                                "class Bar\n"
                                                "  def bar_method = 1\n"
                                                "end\n"
                                                "foo = Foo.new\n"
                                                "foo.");

  assert(find_suggestion(&suggestions, "foo_method"));
  assert(!find_suggestion(&suggestions, "bar_method"));
}

static void
test_same_method_name_in_different_classes(void) {
  TiSuggestionList suggestions = suggest_source("class Foo\n"
                                                "  def value(foo) = foo\n"
                                                "end\n"
                                                "class Bar\n"
                                                "  def value(bar) = bar\n"
                                                "end\n"
                                                "bar = Bar.new\n"
                                                "bar.val");
  const TiSuggestion *suggestion = find_suggestion(&suggestions, "value");

  assert(suggestion);
  assert(strcmp(suggestion->detail, "value(bar)") == 0);
}

static void
test_nested_class_methods_have_separate_owners(void) {
  TiSuggestionList suggestions = suggest_source("class Outer\n"
                                                "  def outer_method = 1\n"
                                                "  class Inner\n"
                                                "    def inner_method = 1\n"
                                                "  end\n"
                                                "end\n"
                                                "outer = Outer.new\n"
                                                "outer.");

  assert(find_suggestion(&suggestions, "outer_method"));
  assert(!find_suggestion(&suggestions, "inner_method"));
}

int
main(void) {
  test_string_suggestion();
  test_prefix_suggestion();
  test_unknown_receiver();
  test_union_suggestion();
  test_union_prefix_suggestion();
  test_union_skips_user_class();
  test_static_method_suggestion();
  test_user_class_suggestion();
  test_user_class_only_suggests_its_methods();
  test_same_method_name_in_different_classes();
  test_nested_class_methods_have_separate_owners();

  return 0;
}
