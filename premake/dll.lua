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

    configurations { "Release", "Debug" }
    platforms { "x32", "x64", "arm64", "wasm" }
    
    filter "platforms:x32"
        architecture "x32"

    filter "platforms:x64"
        architecture "x64"

    filter "platforms:arm64"
        architecture "ARM64"

    filter "configurations:Release"
        optimize "Speed"

    filter "configurations:Debug"
        symbols "On"
        defines "_DEBUG"

    filter "system:windows"
        systemversion "latest"
        startproject "ocgcore"

    filter { "configurations:Release", "action:vs*" }
        linktimeoptimization "On"
        staticruntime "On"
        disablewarnings { "4334" }

    filter "action:vs*"
        cdialect "C11"
        conformancemode "On"
        buildoptions { "/utf-8" }
        defines { "_CRT_SECURE_NO_WARNINGS" }

    filter "system:bsd"
        defines { "LUA_USE_POSIX" }

    filter "system:macosx"
        defines { "LUA_USE_MACOSX" }

    filter "system:linux"
        defines { "LUA_USE_LINUX" }
        pic "On"

    filter "platforms:wasm"
        toolset "emcc"
        defines { "LUA_USE_C89", "LUA_USE_LONGJMP" }
        pic "On"

filter {}

include(LUA_DIR)

project "ocgcore"

    kind "SharedLib"

    files { "*.cpp", "*.h" }
    links { "lua" }
    
    includedirs { LUA_DIR .. "/src" }

    filter "platforms:wasm"
        targetextension ".wasm"
        linkoptions { "-s MODULARIZE=1", "-s EXPORT_NAME=\"createOcgcore\"", "--no-entry", "-s ENVIRONMENT=web,node", "-s EXPORTED_RUNTIME_METHODS=[\"ccall\",\"cwrap\",\"addFunction\",\"removeFunction\"]", "-s EXPORTED_FUNCTIONS=[\"_malloc\",\"_free\"]", "-s ALLOW_TABLE_GROWTH=1", "-s ALLOW_MEMORY_GROWTH=1", "-o ../build/bin/wasm/Release/libocgcore.js" }
