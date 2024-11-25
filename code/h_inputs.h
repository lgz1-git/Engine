#pragma once
#include "h_virtual_keycode.h"

void input_update();

bool input_key_is_up(keys key);
bool input_key_was_up(keys key);
bool input_key_is_down(keys key);
bool input_key_was_down(keys key);

void input_process_key(keys key, bool pressed);

bool input_button_is_up(buttons button);
bool input_button_was_up(buttons button);
bool input_button_is_down(buttons button);
bool input_button_was_down(buttons button);

void input_get_mouse_pos(i32* x, i32* y);
void input_get_precious_mouse_pos(i32* x, i32* y);

void input_process_button(buttons button , bool pressed);
void input_process_mouse_move(i16 x, i16 y);
void input_process_mouse_wheel(i8 z_delta);