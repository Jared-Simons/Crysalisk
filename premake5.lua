workspace "Engine"
	architecture "x64"
	
	configurations
	{
		"Debug",
		"Release",
		"Distribution"
	}
	
outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

project "Core"
	location "Core"
	kind "SharedLib"
	language "C++"
	
	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")
	
	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp",
	}

	characterset("MBCS")

	disablewarnings
	{
		C4302,	---'type-cast' truncation (primary for void* casts)
		C4244,	--- type conversion (possible loss of data)
	}
	
	includedirs
	{	
		"%{prj.name}/src",
		"Engine/src/",
		"Public/",
		"D:/coding/Vulkan/Include",
	}

	links
	{
		"D:/coding/Vulkan/Lib/Vulkan-1",
	}

	libdirs
	{
		"bin/Debug-windows-x86_64/VulkanRenderer",
		"D:/coding/Vulkan/Lib"
	}
	
	filter "system:windows"
		cppdialect "C++17"
		staticruntime "on"
		systemversion "latest"
		
		
		defines
		{
			PLAT_WINDOWS
		}
	
		postbuildcommands
		{
			("{COPY} %{cfg.buildtarget.relpath} ../bin/" .. outputdir .. "/Testapp")
		}
	
	filter "configurations:Debug"
		defines "CRYS_DEBUG;CORE_DLL"
		symbols "on"
		
	filter "configurations:Release"
		defines "CRYS_RELEASE;CORE_DLL"
		symbols "on"
		optimize "on"
	
	filter "configurations:Distribution"
		defines "CRYS_DIST;CORE_DLL"
		optimize "on"
	
project "Engine"
	location "engine"
	kind "SharedLib"
	language "C++"
	
	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")
	
	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp",
	}

	characterset("MBCS")

	disablewarnings
	{
		C4302,	---'type-cast' truncation (primary for void* casts)
		C4244,	--- type conversion (possible loss of data)
	}
	
	includedirs
	{	
		"%{prj.name}/src",
		"Core/src/",
		"Interfaces/src/",
		"Public/",
	}

	links
	{
		"Core",
		"Interfaces"
	}

	libdirs
	{
		"bin/Debug-windows-x86_64/VulkanRenderer",
	}
	
	filter "system:windows"
		cppdialect "C++17"
		staticruntime "on"
		systemversion "latest"
		
		
		defines
		{
			PLAT_WINDOWS
		}
	
		postbuildcommands
		{
			("{COPY} %{cfg.buildtarget.relpath} ../bin/" .. outputdir .. "/Testapp")
		}
	
	filter "configurations:Debug"
		defines "VK_USE_PLATFORM_WIN32_KHR;VK_PROTOTYPES;CRYS_DEBUG;ENGINE_DLL"
		symbols "on"
		
	filter "configurations:Release"
		defines "VK_USE_PLATFORM_WIN32_KHR;VK_PROTOTYPES;CRYS_RELEASE;ENGINE_RELEASE;ENGINE_DLL"
		symbols "on"
		optimize "on"
	
	filter "configurations:Distribution"
		defines "VK_USE_PLATFORM_WIN32_KHR;VK_PROTOTYPES;CRYS_DIST;ENGINE_DLL"
		optimize "on"

project "Interfaces"
	location "Interfaces"
	kind "StaticLib"
	language "C++"
	
	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")
	
	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp",
	}

	characterset("MBCS")

	disablewarnings
	{
		C4302,	---'type-cast' truncation (primary for void* casts)
		C4244,	--- type conversion (possible loss of data)
	}
	
	includedirs
	{	
		"%{prj.name}/src",
		"Core/src/",
		"Engine/src/",
		"Public/",
	}

	links
	{
	}

	libdirs
	{
	}
	
	filter "system:windows"
		cppdialect "C++17"
		staticruntime "on"
		systemversion "latest"
		
		
		defines
		{
			PLAT_WINDOWS
		}
	
		postbuildcommands
		{
			("{COPY} %{cfg.buildtarget.relpath} ../bin/" .. outputdir .. "/Testapp")
		}
	
	filter "configurations:Debug"
		defines "CRYS_DEBUG;"
		symbols "on"
		
	filter "configurations:Release"
		defines "CRYS_RELEASE;"
		symbols "on"
		optimize "on"
	
	filter "configurations:Distribution"
		defines "CRYS_DIST;"
		optimize "on"

project "Testapp"
	location "Testapp"
	kind "ConsoleApp"
	language "C++"
	
	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")
	characterset("MBCS")

	disablewarnings
	{
		C4302	---'type-cast' truncation (primary for void* casts).
	}
	
	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp",
	}
	
	includedirs
	{	
		"Engine/src/",
		"Public/",
		"Core/src/",
	}
	
	links
	{
		"Engine"
	}
	
	filter "system:windows"
		cppdialect "C++17"
		staticruntime "on"
		systemversion "latest"
		
		defines
		{
			PLAT_WINDOWS
		}

		links
		{

		}
	
	filter "configurations:Debug"
		defines "GAME_API;CRYS_DEBUG;"
		symbols "on"
		
	filter "configurations:Release"
		defines "GAME_API;CRYS_RELEASE;"
		symbols "on"
		optimize "on"
	
	filter "configurations:Distribution"
		defines "GAME_API;CRYS_DIST;"
		optimize "on"
		
project "Renderer_Vulkan"
	location "Renderer_Vulkan"
	kind "SharedLib"
	language "C++"
	
	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")
	
	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp",
	}

	characterset("MBCS")

	disablewarnings
	{
		C4302,	---'type-cast' truncation (primary for void* casts)
		C4244,	--- type conversion (possible loss of data)
		C4190,  --- stop warning about C-compatible interfaces, we need this now for our interface registry, but this should be fixed in the future.
	}
	
	includedirs
	{	
		"%{prj.name}/src",
		"Core/src/",
		"Engine/src",
		"Interfaces/src",
		"D:/coding/Vulkan/Include",
		"Public/",
	}

	links
	{
		"D:/coding/Vulkan/Lib/Vulkan-1",
		"Core",
		"Engine",
		"Interfaces",
	}

	libdirs
	{
		"bin/Debug-windows-x86_64/VulkanRenderer",
		"D:/coding/Vulkan/Lib"
	}
	
	filter "system:windows"
		cppdialect "C++17"
		staticruntime "on"
		systemversion "latest"
		
		
		defines
		{
			PLAT_WINDOWS
		}
	
		postbuildcommands
		{
			("{COPY} %{cfg.buildtarget.relpath} ../bin/" .. outputdir .. "/Testapp")
		}
	
	filter "configurations:Debug"
		defines "VK_USE_PLATFORM_WIN32_KHR;VK_PROTOTYPES;CRYS_DEBUG;VULKAN_DLL"
		symbols "on"
		
	filter "configurations:Release"
		defines "VK_USE_PLATFORM_WIN32_KHR;VK_PROTOTYPES;CRYS_RELEASE;VULKAN_DLL"
		symbols "on"
		optimize "on"
	
	filter "configurations:Distribution"
		defines "VK_USE_PLATFORM_WIN32_KHR;VK_PROTOTYPES;CRYS_DIST;VULKAN_DLL"
		optimize "on"

project "UnitTest"
	location "UnitTest"
	kind "ConsoleApp"
	language "C++"
	
	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")
	characterset("MBCS")

	disablewarnings
	{
		C4302	---'type-cast' truncation (primary for void* casts).
	}
	
	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp",
	}
	
	includedirs
	{	
		"Engine/src/",
		"Core/src/",
		"Public/",
		"UnitTest/src/",
	}
	
	links
	{
		"Core"
	}
	
	filter "system:windows"
		cppdialect "C++17"
		staticruntime "on"
		systemversion "latest"
		
		defines
		{
			PLAT_WINDOWS
		}
		
		postbuildcommands
		{
			("{COPY} %{cfg.buildtarget.relpath} ../bin/" .. outputdir .. "/Testapp")
		}
	
	filter "configurations:Debug"
		defines "GAME_API;CRYS_DEBUG;"
		symbols "on"
		debugdir = "bin/Debug-windows-x86_64/Testapp"
		
	filter "configurations:Release"
		defines "GAME_API;CRYS_RELEASE;"
		symbols "on"
		optimize "on"
	
	filter "configurations:Distribution"
		defines "GAME_API;CRYS_DIST;"
		optimize "on"
