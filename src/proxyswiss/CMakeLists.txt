file(GLOB_RECURSE proxyswiss_SOURCES "./*.c" "./*.cpp" "./*.h")

add_executable (proxyswiss ${proxyswiss_SOURCES})
target_link_libraries(proxyswiss common proxy ${Boost_LIBRARIES})
link_directories(proxyswiss STATIC ${Boost_LIBRARY_DIRS})
target_compile_features(proxyswiss PRIVATE cxx_std_17)
