file(GLOB_RECURSE common_SOURCES "./*.cpp" "./*.h")

add_library (common ${common_SOURCES})
target_compile_features(common PRIVATE cxx_std_17)


