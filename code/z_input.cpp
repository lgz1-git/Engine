#include "h_inputs.h"
#include "h_clogger.h" 
#include "h_events.h"

struct keyboard_state {
	bool keys[256];
};

struct mouse_state {
	i16 x;
	i16 y;
	u8 button[BUTTON_MAX_BUTTON_COUNTS];
};

struct input_state {
	keyboard_state precious_key;
	keyboard_state current_key;
	mouse_state precious_mouse;
	mouse_state current_mouse;
};

static input_state state;

void input_update()
{
	memcpy(&state.precious_key, &state.current_key, sizeof(keyboard_state));
	memcpy(&state.precious_mouse, &state.current_mouse, sizeof(mouse_state));
}

bool input_key_is_up(keys key)
{
	return state.current_key.keys[key] == false; 
}
bool input_key_was_up(keys key)
{
	return state.precious_key.keys[key] == false;
}
bool input_key_is_down(keys key)
{
	return state.current_key.keys[key] == true;
}
bool input_key_was_down(keys key)
{
	return state.precious_key.keys[key] == true;
}

void input_process_key(keys key, bool pressed)
{
	if (state.current_key.keys[key] != pressed) {
		state.current_key.keys[key] = pressed;
	}
	event_context context;
	context.data.u32[0] = key;
	event_fire(pressed ? EVENT_CODE_KEY_PRESSED : EVENT_CODE_KEY_RELEASED, 0, context);
}

bool input_button_is_up(buttons button)
{
	return state.current_mouse.button[button] == false;
}
bool input_button_was_up(buttons button)
{
	return state.precious_mouse.button[button] == false;
}
bool input_button_is_down(buttons button)
{
	return state.current_mouse.button[button] == true;
}
bool input_button_was_down(buttons button)
{
	return state.precious_mouse.button[button] == true;
}

void input_get_mouse_pos(i32* x, i32* y)
{
	*x = state.current_mouse.x;
	*y = state.current_mouse.y;
}
void input_get_precious_mouse_pos(i32* x, i32* y)
{
	*x = state.precious_mouse.x;
	*y = state.precious_mouse.y;
}

void input_process_button(buttons button, bool pressed)
{
	if (state.current_key.keys[button] != pressed) {
		state.current_key.keys[button] = pressed;
	}
	event_context context;
	context.data.u32[0] = button;
	event_fire(pressed ? EVENT_CODE_BUTTON_PRESSED : EVENT_CODE_BUTTON_RELEASED, 0, context);
}
void input_process_mouse_move(i16 x, i16 y)
{
	if (state.current_mouse.x != x || state.current_mouse.y != y)
	{
		state.current_mouse.x = x;
		state.current_mouse.y = y;
		event_context context;
		context.data.u32[0] = x;
		context.data.u32[1] = y;
		event_fire(EVENT_CODE_MOUSE_MOVE, 0, context);
	}
}
void input_process_mouse_wheel(i8 z_delta)
{
	event_context context;
	context.data.u32[0] = z_delta;
	event_fire(EVENT_CODE_MOUSE_WHEEL, 0, context);
}