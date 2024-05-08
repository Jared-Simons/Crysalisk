workspace "Engine"
	architecture "x64"
	
	configurations
	{
		"Debug",
		"Release",
		"Distribution"
	}
	
outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"
	
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
	
	includedirs
	{	
		"%{prj.name}/src",
		"D:/coding/Vulkan/Include",
		"UtilityLib/src/"
	}

	links
	{
		"D:/coding/Vulkan/Lib/Vulkan-1",
		"UtilityLib"
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
		defines "VK_USE_PLATFORM_WIN32_KHR;VK_PROTOTYPES;ENGINE_DEBUG;ENGINE_DLL"
		symbols "on"
		
	filter "configurations:Release"
		defines "VK_USE_PLATFORM_WIN32_KHR;VK_PROTOTYPES;ENGINE_DIST;ENGINE_RELEASE;ENGINE_DLL"
		symbols "on"
		optimize "on"
	
	filter "configurations:Distribution"
		defines "VK_USE_PLATFORM_WIN32_KHR;VK_PROTOTYPES;ENGINE_DIST;ENGINE_DLL"
		optimize "on"

project "Testapp"
	location "Testapp"
	kind "ConsoleApp"
	language "C++"
	
	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")
	characterset("MBCS")
	
	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp",
	}
	
	includedirs
	{	
		"Engine/src/"
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
			"UtilityLib"
		}
	
	filter "configurations:Debug"
		defines "GAME_API;GAME_DEBUG;"
		symbols "on"
		
	filter "configurations:Release"
		defines "GAME_API;GAME_RELEASE;"
		symbols "on"
		optimize "on"
	
	filter "configurations:Distribution"
		defines "GAME_API;GAME_DIST;"
		optimize "on"


project "UtilityLib"
	location "UtilityLib"
	kind "StaticLib"
	language "C++"
	
	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")
	characterset("MBCS")
	
	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp",
	}
	
	includedirs
	{	
	}
	
	links
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
		defines "UTILITY_DLL;UTILITY_API;UTILITY_DEBUG;"
		symbols "on"
		
	filter "configurations:Release"
		defines "UTILITY_DLL;UTILITY_API;UTILITY_RELEASE;"
		symbols "on"
		optimize "on"
	
	filter "configurations:Distribution"
		defines "UTILITY_DLL;UTILITY_API;UTILITY_DIST;"
		optimize "on"

