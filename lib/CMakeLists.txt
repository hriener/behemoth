add_library(behemoth_fmt INTERFACE)
target_include_directories(behemoth_fmt INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/fmt)
target_compile_definitions(behemoth_fmt INTERFACE FMT_HEADER_ONLY)

add_library(behemoth_cli11 INTERFACE)
target_include_directories(behemoth_cli11 INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/cli11)

#add_library(behemoth_any INTERFACE)
#target_include_directories(behemoth_any INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/any)
