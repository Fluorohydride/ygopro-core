project "ocgcore"
    kind "StaticLib"

    files { "*.cpp", "*.h" }
    filter "system:windows"
        includedirs { "../lua" }
    filter "not action:vs*"
        buildoptions { "-std=c++14" }
    filter "not system:windows"
        includedirs { "/usr/include/lua5.3" }
