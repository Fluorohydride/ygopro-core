project "ocgcore"
    kind "StaticLib"

    files { "*.cpp", "*.h" }
    links { "lua" }
    
    if BUILD_LUA then
        includedirs { "../lua/src" }
    end

    filter "not action:vs*"
        buildoptions { "-std=c++14" }

    filter "system:bsd"
        defines { "LUA_USE_POSIX" }

    filter "system:macosx"
        defines { "LUA_USE_MACOSX" }

    filter "system:linux"
        defines { "LUA_USE_LINUX" }
