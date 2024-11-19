#pragma once
#include "h_type.h"

struct vec3
{
	f32 x, y, z;
};

struct vec4 {
	f32 x, y, z, w;
};

struct mat4 {
	vec4 a;
	vec4 b;
	vec4 c;
	vec4 d;	
};

struct vert_3D{
	vec3 position;
};