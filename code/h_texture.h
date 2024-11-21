#pragma once
#include "h_type.h"
struct texture {
	u32 id;
	u32 w, h;
	u8 channel_counts;
	bool has_transparency;
	u32 generation;
	void* internal_data;
};