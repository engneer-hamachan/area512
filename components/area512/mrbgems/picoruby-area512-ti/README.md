# picoruby-area512-ti

The built-in database is generated directly from the RBS files listed in
`tools/tidbgen/manifest.rb`. Run `make gendb` from the Area512 project root
whenever one of those files changes, then review the generated
`area512_ti_builtin_db.c` and `area512_ti_builtin_db.h` files.

The generator does not create an intermediate JSON file. Types that RBS can
resolve but the current database fields cannot represent are stored as
`Untyped`.

Suggestion strings remain valid until the next `ti_fill_suggestions_at_cursor` call. User-class completion currently lists definitions appearing after the class declaration because v1 does not track definition ownership.
