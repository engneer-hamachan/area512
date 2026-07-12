# picoruby-area512-ti

Run `go run ./tools/tidbgen/main.go -config ./template/app/.ti-config -out ./components/area512/mrbgems/picoruby-area512-ti/src/generated` from the Area512 project root.
Review the generated `area512_ti_builtin_db.c` and `area512_ti_builtin_db.h` files.
Regenerate both files whenever the whitelisted `.ti-config` inputs change.

Suggestion strings remain valid until the next `ti_fill_suggestions_at_cursor` call. User-class completion currently lists definitions appearing after the class declaration because v1 does not track definition ownership.
