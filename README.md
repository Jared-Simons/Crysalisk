# Cryaslisk Game Engine
This project was started as a learning experience for building a complicated piece of software mostly from scratch. The goal for this project is to have a game engine that is mostly (if not completely), built using original code and to be performant enough to run on lower-end hardware yet flexible enough to shine in a variety of game-related situations.

![Screenshot of Crysalisk](Images/Crys_1.png?raw=true "Screenshot of Crysalisk")
![Screenshot of Crysalisk](Images/Crys_2.png?raw=true "Screenshot of Crysalisk")
![Screenshot of Crysalisk](Images/Crys_3.png?raw=true "Screenshot of Crysalisk")

# Platform Support
Currently Crysalisk only supports Win32 using Visual Studio and MSVC. However, support for Linux and potentially MacOS are planned for the future.

# Prerequisites for Windows
1.) Git for Windows: https://gitforwindows.org/ (Not required but recommended)  
2.) Visual Studio 2026 Community: https://visualstudio.microsoft.com/downloads/ (VS2022+ supported)  
3.) CMake: https://cmake.org/download/  
4.) Vulkan SDK: https://vulkan.lunarg.com/sdk/home  

# Important note for Vulkan
Crysalisk expects the Vulkan SDK to be located at the 'VULKAN_SDK' environment variable. Without this the project will fail to compile.  

To ensure the Vulkan SDK is setup properly after installation, open a Windows Command Line and enter 'echo %VULKAN_SDK%. This should print the path on your computer to the Vulkan SDK. If it prints another path or simply '%VULKAN_SDK%' then the Vulkan SDK was not properly setup.

# Building The Project
1.) Clone the repository using 'git clone https://github.com/Jared-Simons/Crysalisk'  
2.) From the project root directory, run 'GenerateProjects.bat', this will generate a Visual Studio solution.  
3.) There should be a new directory named 'Build', which contains a Crysalisk.sln solution file. Open this file.  
4.) Near the top, there is a dropdown menu with 'Debug' selected. Select either 'Debug' or 'Release', then go to Build -> Build Solution to build the project.  
5.)
5a.) The project can also be launched from either the command line or directly from the file directory. After the project

# Running The Project
1.) From within Visual Studio:  
* The default startup project is 'Testapp', which is the main testbed for Crysalisk. The other options are 'Editor' and 'UnitTest' (detailed below). To launch either, Open the Solution Explorer (View -> Solution Explorer), right-click on the desired project, and select 'Set as Startup Project'.   
* To launch the selected project, press either 'F5' or the green play button labelled 'Local Windows Debugger'.  

2.) From the command line/file directory.
* After the project builds, there should be a new directory relative to the project root: 'Game\bin', this is where all the built code is copied to.  
* Run either Testapp.exe or Editor.exe.

# Project Structure
This project structure layout is based on the folder structure located in "{Project Root}/Crysalisk", and the file structure mirrors the structure of the built code.
* AppUI - (Builds as a shared library): The UI framework of the engine, contains all of the code for controls, input handlers, and rendering of UI elements.
* Core - (Builds as a shared library): Core utilites shared across the codebase. Contains custom containers, memory allocators, math classes, platform-specific code, and more.
* Editor - (Builds as an executable): The main editor application. Exclusively used to setup the EditorHub project.
* EditorHub - (Builds as a shared library): The main heartbeat of the editor. Currently only serves to launch the Tools_LevelEditor project, but will eventually be expanded to the be center of Crysalisk's toolset.
* Engine - (Builds as a shared library): The main engine runtime. Contains the entry point and main loop, core systems such as the main input handler and event dispatcher, and other shared engine utilities.
* Renderer_Vulkan - (Builds as a shared library): The Vulkan renderer-backend. The renderer-frontend/interface lives in Engine.dll, and Renderer_Vulkan contains the Vulkan-specific implementations.
* Testapp - (Builds as an executable): Crysalisk's testbed application, used to quickly test new features and currently serves as a stand-in for the game layer.
* Tools_LevelEditor - (Builds as a shared library): Code for the Level-editing tool, also includes code for tools such at the interactive gizmo.
* UnitTest - (Builds as an executable): Unit test framework for Crysalisk, needs to be expanded upon in the future.