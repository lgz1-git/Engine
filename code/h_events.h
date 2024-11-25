#pragma once
#include "h_type.h"
struct event_context {
	union {
		i64 i64[2];
		u64 u64[2];
		f64 f64[2];

		i32 i32[4];
		u32 u32[4];
		f32 f32[4];

		i16 i16[8];
		u16 u16[8];

		i8  i8[16];
		u8  u8[16];

		char c[16];
	}data;
};
typedef bool (*FP_on_event)(u32 code, void* sender, void* listener_inst, event_context data);

bool event_register(u32 code, void* listener, FP_on_event on_event);
bool event_unregister(u32 code, void* listener, FP_on_event on_event);
bool event_fire(u32 code, void* sender, event_context context);

enum sys_event_code {
	EVENT_CODE_KEY_PRESSED = 0X01,
	EVENT_CODE_KEY_RELEASED ,
	EVENT_CODE_BUTTON_PRESSED ,
	EVENT_CODE_BUTTON_RELEASED ,
	EVENT_CODE_MOUSE_MOVE ,
	EVENT_CODE_MOUSE_WHEEL ,
	EVENT_CODE_RESIZE,
	MAX_EVENT_CODE
};