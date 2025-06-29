%VULKAN_SDK%\Bin\glslc.exe -fshader-stage=vert shader.vert.glsl -o core.vert.spv
%VULKAN_SDK%\Bin\glslc.exe -fshader-stage=frag shader.frag.glsl -o core.frag.spv

%VULKAN_SDK%\Bin\glslc.exe -fshader-stage=vert UI.vert.glsl -o UI.vert.spv
%VULKAN_SDK%\Bin\glslc.exe -fshader-stage=frag UI.frag.glsl -o UI.frag.spv
pause