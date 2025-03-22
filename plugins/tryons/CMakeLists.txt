cmake_minimum_required(VERSION 3.28...3.30)
project(tryons_plugin)

# Add both subdirectories (optional, if you have separate CMakeLists.txt in audio/ and text/)
# or list all source files directly if you prefer a single file.
add_subdirectory(audio)
add_subdirectory(text)
add_subdirectory(network)
