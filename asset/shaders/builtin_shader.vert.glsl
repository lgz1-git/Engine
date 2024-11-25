#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 in_texcoord;

layout(set = 0 , binding = 0) uniform global_uniform_object{
	mat4 projection;
	mat4 view;
}g_ubo;

layout(push_constant) uniform push_constants{
	mat4 model;
}constants;

layout(location = 0) out int out_mode;
layout(location = 1) out struct texture_object{
	vec2 tex_coord;
}out_texture_object;


void main()
{	out_texture_object.tex_coord = in_texcoord;
	gl_Position = g_ubo.projection *g_ubo.view * constants.model * vec4(in_position , 1.0);
}