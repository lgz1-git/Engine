#pragma once
#include "h_win32_platform.h"
#include "h_type.h"


struct Clock {

private:
	LARGE_INTEGER last_counter;
	LARGE_INTEGER end_counter;
	LARGE_INTEGER frequency;
	u32 pointer_counts;
	
public:
	Clock();
	void start();
	void insert();
	void api();
};



