project (ocgcore)

set (AUTO_FILES_RESULT)
AutoFiles("." "src" "\\.(cpp|c|h)$")

if (MSVC)
    include_directories ( ../lua )
else ()
    include_directories ( ${LUA_INCLUDE_DIR} )
endif ()

add_library (ocgcore STATIC ${AUTO_FILES_RESULT})
