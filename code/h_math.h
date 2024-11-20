#pragma once
#include "h_type.h"

constexpr auto PI = 3.14159265358979323846f;
constexpr auto PI_2 = 2.0f * PI;
constexpr auto f_epsilon = 1.192092896e-07f;

typedef struct vec2
{
	union {
		f32 elements[2];
		struct
		{
			union {
				f32 x, r, s, u;
			};
			union {
				f32 y, g, t, v;
			};
		};
	};
};

typedef struct vec3 {
	union {
		f32 elements[3];
		struct {
			union {
				f32 x, r, s, u;
			};
			union {
				f32 y, g, t, v;
			};
			union {
				f32 z, b, p, w;
			};
		};
	};
};


typedef struct vec4 {
#if defined(L_SIMD)
	alignas(16) __m128 data
#endif // 
	union {
	alignas(16)	f32 elements[4];
		union {
			f32 x, r, s;
		};
		union {
			f32 y, g, t;
		};
		union {
			f32 z, b, p;
		};
		union {
			f32 w, a, q;
		};
	};
};

struct mat4 {
	vec4 a;
	vec4 b;
	vec4 c;
	vec4 d;
};

struct vert_3D {
	vec3 position;
};


namespace vkm {
	f32 sin(f32 x);
	f32 cos(f32 x);
	f32 tan(f32 x);
	f32 acos(f32 x);
	f32 sqrt(f32 x);
	f32 abs(f32 x);
};