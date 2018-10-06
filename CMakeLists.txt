project (ocgcore)

set (AUTO_FILES_RESULT)
AutoFiles("." "src" "\\.(cpp|c|h)$")

include_directories ( ../lua )

add_library (ocgcore STATIC ${AUTO_FILES_RESULT})
