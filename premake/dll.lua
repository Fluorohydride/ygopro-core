newoption { trigger = "lua-dir", description = "", value = "PATH", default = "./lua" }

function GetParam(param)
    return _OPTIONS[param] or os.getenv(string.upper(string.gsub(param,"-","_")))
end

LUA_DIR=GetParam("lua-dir")
if not os.isdir(LUA_DIR) then
    LUA_DIR="../lua"
end

workspace "ocgcoredll"
    location "build"
    language "C++"
    cppdialect "C++14"

    configurations { "Release", "Debug" }
    platforms { "x32", "x64" }
    
    filter "platforms:x32"
        architecture "x32"

    filter "platforms:x64"
        architecture "x64"

    filter "configurations:Release"
        optimize "Speed"

    filter "configurations:Debug"
        symbols "On"
        defines "_DEBUG"

    filter "system:windows"
        defines { "WIN32", "_WIN32" }
        systemversion "latest"
        startproject "ocgcore"

    filter { "configurations:Release", "action:vs*" }
        if linktimeoptimization then
            linktimeoptimization "On"
        else
            flags { "LinkTimeOptimization" }
        end
        staticruntime "On"
        disablewarnings { "4334" }

    filter "action:vs*"
        buildoptions { "/utf-8" }
        defines { "_CRT_SECURE_NO_WARNINGS" }

    filter "not action:vs*"
        buildoptions { }

    filter "system:bsd"
        defines { "LUA_USE_POSIX" }

    filter "system:macosx"
        defines { "LUA_USE_MACOSX" }

    filter "system:linux"
        defines { "LUA_USE_LINUX" }
        buildoptions { "-fPIC" }

filter {}

include(LUA_DIR)

project "ocgcore"

    kind "SharedLib"
    cppdialect "C++14"

    files { "*.cpp", "*.h" }
    links { "lua" }
    
    includedirs { LUA_DIR .. "/src" }
