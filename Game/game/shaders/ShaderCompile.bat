%VULKAN_SDK%\Bin\glslc.exe -fshader-stage=vert shader.vert.glsl -o core.vert.spv
%VULKAN_SDK%\Bin\glslc.exe -fshader-stage=frag shader.frag.glsl -o core.frag.spv

%VULKAN_SDK%\Bin\glslc.exe -fshader-stage=vert UI.vert.glsl -o UI.vert.spv
%VULKAN_SDK%\Bin\glslc.exe -fshader-stage=frag UI.frag.glsl -o UI.frag.spv

%VULKAN_SDK%\Bin\glslc.exe -fshader-stage=vert Core.Unlit.vert.glsl -o Core.Unlit.vert.spv
%VULKAN_SDK%\Bin\glslc.exe -fshader-stage=frag Core.Unlit.frag.glsl -o Core.Unlit.frag.spv

%VULKAN_SDK%\Bin\glslc.exe -fshader-stage=vert AppUI.vert.glsl -o AppUI.vert.spv
%VULKAN_SDK%\Bin\glslc.exe -fshader-stage=frag AppUI.frag.glsl -o AppUI.frag.spv

%VULKAN_SDK%\Bin\glslc.exe -fshader-stage=vert AppUI_ImagePanel.vert.glsl -o AppUI_ImagePanel.vert.spv
%VULKAN_SDK%\Bin\glslc.exe -fshader-stage=frag AppUI_ImagePanel.frag.glsl -o AppUI_ImagePanel.frag.spv

%VULKAN_SDK%\Bin\glslc.exe -fshader-stage=vert Debug.Line.vert.glsl -o Debug.Line.vert.spv
%VULKAN_SDK%\Bin\glslc.exe -fshader-stage=vert Debug.Line.VertColor.vert.glsl -o Debug.Line.VertColor.vert.spv
%VULKAN_SDK%\Bin\glslc.exe -fshader-stage=frag Debug.Line.frag.glsl -o Debug.Line.frag.spv

%VULKAN_SDK%\Bin\glslc.exe -fshader-stage=vert Debug.Text.vert.glsl -o Debug.Text.vert.spv
%VULKAN_SDK%\Bin\glslc.exe -fshader-stage=frag Debug.Text.frag.glsl -o Debug.Text.frag.spv
pause