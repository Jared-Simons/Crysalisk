%VULKAN_SDK%\Bin\glslc.exe -fshader-stage=vert shader.vert.glsl -o core.vert.spv
%VULKAN_SDK%\Bin\glslc.exe -fshader-stage=frag shader.frag.glsl -o core.frag.spv

%VULKAN_SDK%\Bin\glslc.exe -fshader-stage=vert UI.vert.glsl -o UI.vert.spv
%VULKAN_SDK%\Bin\glslc.exe -fshader-stage=frag UI.frag.glsl -o UI.frag.spv

%VULKAN_SDK%\Bin\glslc.exe -fshader-stage=vert AppUI.vert.glsl -o AppUI.vert.spv
%VULKAN_SDK%\Bin\glslc.exe -fshader-stage=frag AppUI.frag.glsl -o AppUI.frag.spv

%VULKAN_SDK%\Bin\glslc.exe -fshader-stage=vert Debug.Line.vert.glsl -o Debug.Line.vert.spv
%VULKAN_SDK%\Bin\glslc.exe -fshader-stage=frag Debug.Line.frag.glsl -o Debug.Line.frag.spv

%VULKAN_SDK%\Bin\glslc.exe -fshader-stage=vert Debug.Text.vert.glsl -o Debug.Text.vert.spv
%VULKAN_SDK%\Bin\glslc.exe -fshader-stage=frag Debug.Text.frag.glsl -o Debug.Text.frag.spv
pause