#pragma once

#include "py/obj.h"

void *area512_micropython_fetch_sprite_handle_or_raise(mp_obj_t sprite_object);
void *area512_micropython_fetch_quaternary_sprite_handle_or_raise(
  mp_obj_t sprite_object
);
