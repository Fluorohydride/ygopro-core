project "ocgcore"
    kind "StaticLib"
    cppdialect "C++14"

    files { "*.cpp", "*.h" }
    links { LUA_LIB_NAME }
    
    if BUILD_LUA then
        includedirs { "../lua/src" }
    else
        includedirs { LUA_INCLUDE_DIR }
        libdirs { LUA_LIB_DIR }
    end

    filter "not action:vs*"
        buildoptions { }

    filter "system:bsd"
        defines { "LUA_USE_POSIX" }

    filter "system:macosx"
        defines { "LUA_USE_MACOSX" }

    filter "system:linux"
        defines { "LUA_USE_LINUX" }
