@echo off

mkdir ..\asset\shaders\shader_code


echo "compiling shaders..."
%VULKAN_SDK%\bin\glslc.exe -fshader-stage=vert ..\asset/shaders/builtin_shader.vert.glsl -o ..\asset/shaders/shader_code/builtin_shader.vert.spv
%VULKAN_SDK%\bin\glslc.exe -fshader-stage=frag ..\asset/shaders/builtin_shader.frag.glsl -o ..\asset/shaders/shader_code/builtin_shader.frag.spv

echo "Done."