project "ocgcore"
    kind "StaticLib"

    files { "*.cpp", "*.h" }
    if BUILD_LUA then
        links { "lua" }
        includedirs { "../lua/src" }
    end
    filter "not action:vs*"
        buildoptions { "-std=c++14" }
