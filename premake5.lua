include "Crysalisk/premake5.lua"
include "Vulkan-App-Framework/premake5.lua"

workspace "Crysalisk"
	architecture "x86_64"
	startproject "Testapp"

	configurations
	{
		"Debug",
		"Release",
		"Distribution"
	}

	flags
	{
		"MultiProcessorCompile"
	}

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

group "Engine"
	include "Crysalisk"
group ""

group "Tools"
	include "Vulkan-App-Framework"
group ""