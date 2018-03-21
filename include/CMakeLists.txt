add_library(behemoth INTERFACE)
target_include_directories(behemoth INTERFACE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(behemoth INTERFACE behemoth_fmt)
