project "ocgcore"
    kind "StaticLib"

    files { "**.cc", "**.cpp", "**.c", "**.h" }
    configuration "windows"
        includedirs { "../lua" }
    configuration "not vs*"
        buildoptions { "-std=c++14" }
    configuration "not windows"
        includedirs { "/usr/include/lua5.3" }
