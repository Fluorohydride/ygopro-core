project "ocgcore"
    kind "StaticLib"

    files { "*.cpp", "*.h" }
    filter "not action:vs*"
        buildoptions { "-std=c++14" }
    if BUILD_LUA then
        links { "lua" }
        includedirs { "../lua" }
    end
