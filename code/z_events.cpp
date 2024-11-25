#include "h_events.h" 
#include "h_clogger.h"
#include <vector>//TODO:memory manage

struct registered_event {
	void* listener;
	FP_on_event callback;
};

struct event_code_entry {
	std::vector<registered_event> events;
};

#define MAX_MESSAGE_CODES 16384

struct event_system_state {
	event_code_entry registered[MAX_MESSAGE_CODES];
};

static event_system_state state;

bool event_register(u32 code, void* listener, FP_on_event on_event)
{
	size_t register_count = state.registered[code].events.size();
	for (u64 i = 0; i < register_count; i++) {
		if (state.registered[code].events[i].listener == listener)
			LWARN("the listener repeatly!");
			return false;
	}
	registered_event event;
	event.listener = listener;
	event.callback = on_event;
	state.registered[code].events.emplace_back(event);
	return true;
}
bool event_fire(u32 code, void* sender, event_context context)
{
	for (size_t i = 0; i < state.registered[code].events.size(); i++) {
		registered_event e = state.registered[code].events[i];
		if (e.callback(code, sender, e.listener, context))
		{
			return true;
		}
	}
	return false;
}