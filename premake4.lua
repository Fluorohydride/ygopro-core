project "ocgcore"
    kind "StaticLib"

    files { "**.cc", "**.cpp", "**.c", "**.h" }
    includedirs { "../lua" }
    configuration "not vs*"
        buildoptions { "-std=c++14" }
