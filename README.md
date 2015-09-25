#Ygopro script engine.

##Introduction
The core logic and lua script processor of YGOPro. this library can be made external of the project and used to power server technologies. It mantains a state engine that is manipulated by Lua scripts using manipulation functions it exposes.

##Compiling
###1.) Download Fluorohydride/ygopro
Start by downloading the most parent of the source code. The team developing this project are the defacto edge and experts in our community. The most upto date `ocgcore` is a compiled dll version of the `Fluorohydride/ygopro/ocgcore` folders project.

###2.) Install Premake4 and Visual Studio 2010 (or later).
Download premake4.exe, put it in `c:\windows` or a similar folder that is globally accessiable via `cmd` or PowerShell. Install Visual Studio 2010, it is the system used for the guide because other parts of the project use C# and most the development team are Windows users.

###3.) Download dependencies
Dependencies are absent from the main project. There is information on how to build them but the easist thing to do is to download the following folders from a `soarquin/ygopro` fork and simply copy them into the `Fluorohydride/ygopro` folder.

* event
* freetype
* irrklang
* irrlict
* lua
* sqlite3
    
###4.) Create the project files
Run the following commands from the command line in the `Fluorohydride/ygopro` folder.

` premake4 /help `
` premake4 vs2010 `

If you are not using Visual Studio 2010 or higher, make the needed adjustments. In the file system open `Fluorohydride/ygopro/build` folder open the `ygo` project.

### Build the system
Make sure the code actually compiles. Compile them in the following order one by one:

* lua
* sqlite3
* ocgcore

This should provide you with `ocgcore.lib` in the build output folder. `YGOCore` requires a `*.dll`; in `ocgcore` project properities change it to a dynamically linked library. Recompile, it should fail with an error indicating missing dependencies. Right click the project, add a existing file. Add `lua.lib` from the build folder to the project. It should now compile.