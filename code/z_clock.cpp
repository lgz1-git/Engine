#include "h_clock.h"

Clock::Clock() 
{ 
	QueryPerformanceFrequency(&frequency); 
	QueryPerformanceCounter(&last_counter);
	end_counter = last_counter;
	pointer_counts = 0;
}
void Clock::start()
{
	QueryPerformanceCounter(&last_counter);
}
void Clock::insert()
{
	pointer_counts++;
	QueryPerformanceCounter(&end_counter);
	f32 per_frame_time = 1000.f * (end_counter.QuadPart - last_counter.QuadPart) / frequency.QuadPart;
	std::cout << "\033[" << "1;32m" << "timer :" << pointer_counts << "\033[0m" << "    ";
	LTIME(per_frame_time);
}
void Clock::api()
{
	pointer_counts++;
	QueryPerformanceCounter(&end_counter);
	f32 per_frame_time = 1000.f * (end_counter.QuadPart - last_counter.QuadPart) / frequency.QuadPart;
}
