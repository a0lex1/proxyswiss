file(GLOB_RECURSE proxy_SOURCES "./*.cpp" "./*.h")

add_library (proxy ${proxy_SOURCES})
target_compile_features(proxy PRIVATE cxx_std_17)
target_link_libraries(proxy common)

