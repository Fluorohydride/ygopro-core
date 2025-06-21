project "ocgcore"
    kind "StaticLib"

    files { "*.cpp", "*.h" }
    
    if BUILD_LUA then
        includedirs { "../lua/src" }
    else
        includedirs { LUA_INCLUDE_DIR }
    end

    filter "not action:vs*"
        cppdialect "C++14"

    filter "system:bsd"
        defines { "LUA_USE_POSIX" }

    filter "system:macosx"
        defines { "LUA_USE_MACOSX" }

    filter "system:linux"
        defines { "LUA_USE_LINUX" }
